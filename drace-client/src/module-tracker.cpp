#include "module-tracker.h"
#include "function-wrapper.h"
#include <drmgr.h>

#include <set>

std::set<module_tracker::module_info_t> modules;

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
	mod_lock = dr_mutex_create();
}

void module_tracker::finalize() {

	if (!drmgr_unregister_module_load_event(event_module_load) ||
		!drmgr_unregister_module_unload_event(event_module_unload)) {
		DR_ASSERT(false);
	}

	dr_mutex_destroy(mod_lock);
}

bool module_tracker::register_events() {
	return (
		drmgr_register_module_load_event(event_module_load) &&
		drmgr_register_module_unload_event(event_module_unload));
}

static void module_tracker::event_module_load(void *drcontext, const module_data_t *mod, bool loaded) {
	thread_id_t tid = dr_get_thread_id(drcontext);
	dr_printf("<< [%i] Loaded module: %s\n", tid, dr_module_preferred_name(mod));
	// create shadow struct of current module
	// for easier comparison
	module_info_t current(mod->start, mod->end);

	dr_mutex_lock(mod_lock);

	auto modit = modules.find(current);
	if (modit != modules.end()) {
		if (!modit->loaded && (modit->info == mod)) {
			// requires mutable, does not modify key
			modit->loaded = true;
		}
	}
	else {
		// Module not already registered
		current.set_info(mod);
		modules.insert(std::move(current));
	}
	dr_mutex_unlock(mod_lock);

	// wrap functions
	funwrap::wrap_mutex_acquire(mod);
	funwrap::wrap_mutex_release(mod);
	funwrap::wrap_allocators(mod);
	funwrap::wrap_deallocs(mod);
	//funwrap::wrap_main(mod);
}

static void module_tracker::event_module_unload(void *drcontext, const module_data_t *mod) {
	module_info_t current(mod->start, mod->end);

	dr_mutex_lock(mod_lock);

	auto modit = modules.find(current);
	if (modit != modules.end()) {
		modit->loaded = false;
	}

	dr_mutex_unlock(mod_lock);
}
