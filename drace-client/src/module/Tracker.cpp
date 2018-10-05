#include "globals.h"
#include "Module.h"

#include "function-wrapper.h"
#include "statistics.h"
#include "symbols.h"
#include "util.h"

#include "ipc/SyncSHMDriver.h"
#include "ipc/SharedMemory.h"
#include "ipc/SMData.h"
#include "MSR.h"

#include <drmgr.h>
#include <drutil.h>

#include <map>
#include <vector>
#include <string>
#include <algorithm>

namespace module {

	Tracker::Tracker(const std::shared_ptr<Symbols> & symbols)
		: _syms(symbols)
	{
		mod_lock = dr_rwlock_create();

		excluded_mods = config.get_multi("modules", "exclude_mods");
		excluded_path_prefix = config.get_multi("modules", "exclude_path");

		std::sort(excluded_mods.begin(), excluded_mods.end());

		// convert pathes to lowercase for case-insensitive matching
		for (auto & prefix : excluded_path_prefix) {
			std::transform(prefix.begin(), prefix.end(), prefix.begin(), ::tolower);
		}

		if (!drmgr_register_module_load_event(event_module_load) ||
			!drmgr_register_module_unload_event(event_module_unload)) {
			DR_ASSERT(false);
		}
	}

	Tracker::~Tracker() {
		if (!drmgr_unregister_module_load_event(event_module_load) ||
			!drmgr_unregister_module_unload_event(event_module_unload)) {
			DR_ASSERT(false);
		}

		dr_rwlock_destroy(mod_lock);
	}

	Tracker::PMetadata Tracker::get_module_containing(const app_pc pc) const
	{
		auto m_it = _modules_idx.lower_bound(pc);
		if (m_it != _modules_idx.end() && pc < m_it->second->end) {
			return m_it->second;
		}
		else {
			return nullptr;
		}
	}

	Tracker::PMetadata Tracker::register_module(const module_data_t * mod, bool loaded)
	{
		// first check if module is already registered
		module_tracker->lock_read();
		auto modptr = module_tracker->get_module_containing(mod->start);
		module_tracker->unlock_read();

		if (modptr) {
			if (!modptr->loaded && (modptr->info == mod)) {
				modptr->loaded = true;
				return modptr;
			}
		}

		// Module is not registered (new)
		INSTR_FLAGS def_instr_flags = (INSTR_FLAGS)(
			INSTR_FLAGS::MEMORY | INSTR_FLAGS::STACK | INSTR_FLAGS::SYMBOLS);

		module_tracker->lock_write();
		modptr = module_tracker->add_emplace(mod->start, mod->end);
		module_tracker->unlock_write();

		// Module not already registered
		modptr->set_info(mod);
		modptr->instrument = def_instr_flags;

		std::string mod_path(mod->full_path);
		std::string mod_name(dr_module_preferred_name(mod));
		std::transform(mod_path.begin(), mod_path.end(), mod_path.begin(), ::tolower);

		/* We must ensure that no symbol lookup using MPCR is done until this dll is loaded.
		*  Otherwise, Jitted IPs are always resolved to unknown */
		if (util::common_prefix("System.Threading.Thread.dll", mod_name)) {
			_dotnet_rt_ready = true;
		}

		for (auto prefix : module_tracker->excluded_path_prefix) {
			// check if mod path is excluded
			if (util::common_prefix(prefix, mod_path)) {
				modptr->instrument = INSTR_FLAGS::STACK;
				break;
			}
		}
		// if not in excluded path
		if (modptr->instrument != INSTR_FLAGS::STACK) {
			// check if mod name is excluded
			// in this case, we check for syms but only instrument stack
			const auto & excluded_mods = module_tracker->excluded_mods;
			if (std::binary_search(excluded_mods.begin(), excluded_mods.end(), mod_name)) {
				modptr->instrument = (INSTR_FLAGS)(INSTR_FLAGS::SYMBOLS | INSTR_FLAGS::STACK);
			}
		}
		if (modptr->instrument & INSTR_FLAGS::SYMBOLS) {
			// check if debug info is available
			modptr->debug_info = symbol_table->debug_info_available(mod);
		}

		return modptr;
	}

	/* Module load event implementation.
	* To get clean call-stacks, we add the shadow-stack instrumentation
	* to all modules (even the excluded ones).
	* \note As this function is passed as a callback to a c API, we cannot use std::bind
	*
	*/
	void event_module_load(void *drcontext, const module_data_t *mod, bool loaded) {
		auto start = std::chrono::system_clock::now();

		per_thread_t * data = (per_thread_t*)drmgr_get_tls_field(drcontext, tls_idx);
		DR_ASSERT(nullptr != data);
		thread_id_t tid = data->tid;

		auto modptr = module_tracker->register_module(mod, loaded);

		std::string mod_name = dr_module_preferred_name(mod);

		DR_ASSERT(!dr_using_app_state(drcontext));
		// wrap functions
		if (util::common_prefix(mod_name, "MSVCP") ||
			util::common_prefix(mod_name, "KERNELBASE"))
		{
			funwrap::wrap_mutexes(mod, true);
		}
		else if (util::common_prefix(mod_name, "KERNEL"))
		{
			funwrap::wrap_allocations(mod);
			funwrap::wrap_thread_start_sys(mod);
		}
		else if (util::common_prefix(mod_name, "clr.dll") ||
			util::common_prefix(mod_name, "coreclr.dll"))
		{
			bool m_ok = MSR::attach(mod);

			if (m_ok) {
				MSR::request_symbols(mod);
				funwrap::wrap_sync_dotnet(mod, true);
			}
		}
		else if (modptr->modtype == Metadata::MOD_TYPE_FLAGS::MANAGED)
		{
			if (shmdriver) {
				MSR::request_symbols(mod);
			}
			// Name of .Net modules often contains a full path
			std::string basename = util::basename(mod_name);

			if (util::common_prefix(basename, "System.Private.CoreLib.dll")) {
				if (shmdriver) {
					funwrap::wrap_sync_dotnet(mod, false);
				}
			}
			if (util::common_prefix(basename, "System.Console.dll")) {
				if (shmdriver) {
					funwrap::wrap_sync_dotnet(mod, false);
				}
			}
			if (util::common_prefix(basename, "System.")) {
				// TODO: This is highly experimental
				// Check impact on correctness
				LOG_NOTICE(data->tid, "Detected %s", basename.c_str());
				modptr->instrument = INSTR_FLAGS::STACK;
				if (!util::common_prefix(basename, "System.Private")) {
					modptr->modtype = (Metadata::MOD_TYPE_FLAGS)(modptr->modtype | Metadata::MOD_TYPE_FLAGS::SYNC);
				}
			}
			else {
				// Not a System DLL
				LOG_NOTICE(data->tid, "Detected %s", basename.c_str());
				modptr->instrument = INSTR_FLAGS::STACK;
				modptr->modtype = (Metadata::MOD_TYPE_FLAGS)(modptr->modtype | Metadata::MOD_TYPE_FLAGS::SYNC);
			}
		}
		else if (modptr->instrument != INSTR_FLAGS::NONE) {
			// no special handling of this module

			funwrap::wrap_excludes(mod);
			// This requires debug symbols, but avoids false positives during
			// C++11 thread construction and startup
			if (modptr->debug_info) {
				funwrap::wrap_thread_start(mod);
				funwrap::wrap_mutexes(mod, false);
			}
		}

		LOG_INFO(tid,
			"Track module: % 20s, beg : %p, end : %p, instrument : %s, debug info : %s, full path : %s",
			mod_name.c_str(), modptr->base, modptr->end,
			util::instr_flags_to_str(modptr->instrument).c_str(),
			modptr->debug_info ? "YES" : " NO",
			modptr->info->full_path);

		// Free symbol information. A later access re-creates them, so its safe to do it here
		drsym_free_resources(mod->full_path);
		// free symbols on MPCR side
		if (modptr->modtype == Metadata::MOD_TYPE_FLAGS::MANAGED && shmdriver) {
			MSR::unload_symbols(mod->start);
		}

		data->stats->module_load_duration += std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start);
		data->stats->module_loads++;
	}

	/* Module unload event implementation. As this function is passed
	*  as a callback to a C api, we cannot use std::bind
	*/
	void event_module_unload(void *drcontext, const module_data_t *mod) {
		LOG_INFO(-1, "Unload module: % 20s, beg : %p, end : %p, full path : %s",
			dr_module_preferred_name(mod), mod->start, mod->end, mod->full_path);

		module_tracker->lock_read();
		auto modptr = module_tracker->get_module_containing(mod->start);
		module_tracker->unlock_read();
		if (modptr) {
			modptr->loaded = false;
		}
	}
} // namespace module
