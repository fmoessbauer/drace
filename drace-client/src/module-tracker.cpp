#include "globals.h"
#include "module-tracker.h"
#include "function-wrapper.h"
#include "statistics.h"
#include "symbols.h"
#include "util.h"

#include <drmgr.h>

#include <set>
#include <vector>
#include <string>
#include <algorithm>

/* Global operator to compare module_data_t regarding logic equivalence */
static bool operator==(const module_data_t & d1, const module_data_t & d2)
{
	if (&d1 == &d2) { return true; }

	if (d1.start == d2.start &&
		d1.end == d2.end   &&
		d1.entry_point == d2.entry_point &&
#ifdef WINDOWS
		d1.checksum == d2.checksum  &&
		d1.timestamp == d2.timestamp &&
#endif
		/* treat two modules without name (there are some) as different */
		dr_module_preferred_name(&d1) != NULL &&
		dr_module_preferred_name(&d2) != NULL &&
		strcmp(dr_module_preferred_name(&d1),
			dr_module_preferred_name(&d2)) == 0)
		return true;
	return false;
}
bool operator!=(const module_data_t & d1, const module_data_t & d2) {
	return !(d1 == d2);
}

void ModuleTracker::print_modules() {
	for (const auto & current : modules) {
		const char * mod_name = dr_module_preferred_name(current.info);
		dr_printf("<< [%.5i] Track module: %20s, beg: %p, end: %p, instrument: %s, debug info: %s, full path: %s\n",
			0, mod_name, current.base, current.end,
			current.debug_info ? "YES" : " NO",
			current.instrument ? "YES" : " NO",
			current.info->full_path);
	}
}

ModuleTracker::ModuleTracker(const std::shared_ptr<Symbols> & symbols)
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

ModuleTracker::~ModuleTracker() {
	if (!drmgr_unregister_module_load_event(event_module_load) ||
		!drmgr_unregister_module_unload_event(event_module_unload)) {
		DR_ASSERT(false);
	}

	dr_rwlock_destroy(mod_lock);
}

/* Module load event implementation. As this function is passed
*  as a callback to a c API, we cannot use std::bind
*/
void event_module_load(void *drcontext, const module_data_t *mod, bool loaded) {
	auto start = std::chrono::system_clock::now();

	per_thread_t * data = (per_thread_t*) drmgr_get_tls_field(drcontext, tls_idx);
	thread_id_t tid = data->tid;
	std::string mod_name(dr_module_preferred_name(mod));

	// create shadow struct of current module
	// for easier comparison
	ModuleData current(mod->start, mod->end);
	current.instrument = true;

	module_tracker->lock_read();
	auto & modules = module_tracker->modules;
	auto modit = modules.find(current);
	if (modit != modules.end()) {
		if (!modit->loaded && (modit->info == mod)) {
			// requires mutable, does not modify key
			modit->loaded = true;
		}
		module_tracker->unlock_read();
	}
	else {
		// Module not already registered
		current.set_info(mod);

		std::string mod_path(mod->full_path);
		std::transform(mod_path.begin(), mod_path.end(), mod_path.begin(), ::tolower);

		for (auto prefix : module_tracker->excluded_path_prefix) {
			// check if mod path is excluded
			if (util::common_prefix(prefix, mod_path)) {
				current.instrument = false;
				break;
			}
		}
		if (current.instrument) {
			// check if mod name is excluded
			const auto & excluded_mods = module_tracker->excluded_mods;
			if (std::binary_search(excluded_mods.begin(), excluded_mods.end(), mod_name)) {
				current.instrument = false;
			}
		}
		if (current.instrument) {
			// check if debug info is available
			current.debug_info = symbol_table->debug_info_available(mod);
		}

		dr_printf("< [%.5i] Track module: %20s, beg: %p, end: %p, instrument: %s, debug info: %s, full path: %s\n",
			tid, mod_name.c_str(), current.base, current.end,
			current.instrument ? "YES" : " NO",
			current.debug_info ? "YES" : " NO",
			current.info->full_path);

		module_tracker->unlock_read();
		// These locks cannot be upgraded, hence unlock and lock
		// as nobody can change the container, dispatching is no issue
		module_tracker->lock_write();
		modit = (modules.insert(std::move(current))).first;
		module_tracker->unlock_write();
	}

	DR_ASSERT(!dr_using_app_state(drcontext));
	// wrap functions
	if (util::common_prefix(mod_name, "MSVCP") ||
		util::common_prefix(mod_name, "KERNELBASE"))
	{
		funwrap::wrap_mutexes(mod, true);
	}
	if (util::common_prefix(mod_name, "KERNEL"))
	{
		funwrap::wrap_allocations(mod);
		funwrap::wrap_thread_start_sys(mod);
	}
	if (modit->instrument) {
		funwrap::wrap_excludes(mod);
		// This requires debug symbols, but avoids false positives during
		// C++11 thread construction and startup
		if (modit->debug_info) {
			funwrap::wrap_thread_start(mod);
			funwrap::wrap_mutexes(mod, false);
		}
	}

	// Free symbol information. A later access re-reads them, so its safe to do it here
	drsym_free_resources(mod->full_path);

	data->stats->module_load_duration += std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start);
	data->stats->module_loads++;
}

/* Module unload event implementation. As this function is passed
*  as a callback to a c API, we cannot use std::bind
*/
void event_module_unload(void *drcontext, const module_data_t *mod) {
	dr_printf("< [%.5i] Unload module: %20s, beg: %p, end: %p, full path: %s\n",
		0, dr_module_preferred_name(mod), mod->start, mod->end, mod->full_path);

	module_tracker->lock_read();
	auto modc = module_tracker->get_module_containing(mod->start);
	if(modc.first){
		modc.second->loaded = false;
	}
	module_tracker->unlock_read();
}
