#include "drace-client.h"
#include "module-tracker.h"
#include "function-wrapper.h"
#include "util.h"

#include <drmgr.h>

#include <set>
#include <vector>
#include <string>
#include <algorithm>

std::set<ModuleData, std::greater<ModuleData>> modules;

void *module_tracker::mod_lock;

std::vector<std::string> excluded_mods;
std::vector<std::string> excluded_path_prefix;

void print_modules() {
	for (const auto & current : modules) {
		const char * mod_name = dr_module_preferred_name(current.info);
		dr_printf("<< [%.5i] Track module: %20s, beg: %p, end: %p, instrument: %s, full path: %s\n",
			0, mod_name, current.base, current.end,
			current.instrument ? "YES" : "NO",
			current.info->full_path);
	}
}

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
		/* treat two modules w/ no name (there are some) as different */
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

void module_tracker::init() {
	mod_lock = dr_rwlock_create();

	excluded_mods = config.get_multi("modules", "exclude_mods");
	excluded_path_prefix = config.get_multi("modules", "exclude_path");

	std::sort(excluded_mods.begin(), excluded_mods.end());

	// convert pathes to lowercase for case-insensitive matching
	for (auto & prefix : excluded_path_prefix) {
		std::transform(prefix.begin(), prefix.end(), prefix.begin(), ::tolower);
	}
}

void module_tracker::finalize() {
	//print_modules();

	if (!drmgr_unregister_module_load_event(event_module_load) ||
		!drmgr_unregister_module_unload_event(event_module_unload)) {
		DR_ASSERT(false);
	}

	dr_rwlock_destroy(mod_lock);
}

bool module_tracker::register_events() {
	return (
		drmgr_register_module_load_event(event_module_load) &&
		drmgr_register_module_unload_event(event_module_unload));
}

static void module_tracker::event_module_load(void *drcontext, const module_data_t *mod, bool loaded) {
	bool instrument = true;

	thread_id_t tid = dr_get_thread_id(drcontext);
	std::string mod_name(dr_module_preferred_name(mod));

	// create shadow struct of current module
	// for easier comparison
	ModuleData current(mod->start, mod->end);

	dr_rwlock_read_lock(mod_lock);
	auto modit = modules.find(current);
	if (modit != modules.end()) {
		if (!modit->loaded && (modit->info == mod)) {
			// requires mutable, does not modify key
			modit->loaded = true;
			instrument = modit->instrument;
		}
		dr_rwlock_read_unlock(mod_lock);
	}
	else {
		// Module not already registered
		current.set_info(mod);

		std::string mod_path(mod->full_path);
		std::transform(mod_path.begin(), mod_path.end(), mod_path.begin(), ::tolower);

		for (auto prefix : excluded_path_prefix) {
			if (util::common_prefix(prefix, mod_path)) {
				instrument = false;
				break;
			}
		}
		if (instrument) {
			if (std::binary_search(excluded_mods.begin(), excluded_mods.end(), mod_name)) {
				instrument = false;
			}
		}

		current.instrument = instrument;
		dr_printf("< [%.5i] Track module: %20s, beg: %p, end: %p, instrument: %s, full path: %s\n",
			tid, mod_name.c_str(), current.base, current.end,
			current.instrument ? "YES" : "NO",
			current.info->full_path);

		dr_rwlock_read_unlock(mod_lock);
		// These locks cannot be upgraded, hence unlock and lock
		// as nobody can change the container, dispatching is no issue
		dr_rwlock_write_lock(mod_lock);
		modules.insert(std::move(current));
		dr_rwlock_write_unlock(mod_lock);
	}
	// wrap functions
	// TODO: get prefixes from config file
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
	if (instrument) {
		funwrap::wrap_excludes(mod);
		// This requires debug symbols, but avoids false positives during
		// C++11 thread construction and startup
		funwrap::wrap_thread_start(mod);
		funwrap::wrap_mutexes(mod, false);
	}
}

static void module_tracker::event_module_unload(void *drcontext, const module_data_t *mod) {
	ModuleData current(mod->start, mod->end);

	dr_rwlock_read_lock(mod_lock);

	dr_printf("< [%.5i] Unload module: %20s, beg: %p, end: %p, instrument: %s, full path: %s\n",
		0, dr_module_preferred_name(mod), current.base, current.end, current.instrument ? "YES" : "NO",
		mod->full_path);

	auto modit = modules.find(current);
	if (modit != modules.end()) {
		modit->loaded = false;
	}

	dr_rwlock_read_unlock(mod_lock);
}
