#include "globals.h"
#include "function-wrapper.h"
#include "memory-tracker.h"
#include "config.h"
#include "MSR.h"

#include <vector>
#include <string>
#include <functional>

#include <detector_if.h>
#include <dr_api.h>
#include <drwrap.h>
#include <drutil.h>
#include <drsyms.h>

#include <unordered_map>

bool funwrap::init() {
	bool state = drwrap_init();

	// performance tuning
	drwrap_set_global_flags((drwrap_global_flags_t)(DRWRAP_NO_FRILLS | DRWRAP_FAST_CLEANCALLS));
	return state;
}

void funwrap::finalize() {
	drwrap_exit();
}

void funwrap::wrap_functions(
	const module_data_t *mod,
	const std::vector<std::string> & syms,
	bool full_search,
	funwrap::Method method,
	wrapcb_pre_t pre,
	wrapcb_post_t post)
{
	std::string modname(dr_module_preferred_name(mod));
	for (const auto & name : syms) {
		LOG_INFO(-1, "Search for %s", name.c_str());
		if (method == Method::EXTERNAL_MPCR) {
			std::string symname = (modname + '!') + name;
			auto sr = MSR::search_symbol(mod, symname.c_str(), full_search);
			for (int i = 0; i < sr.size; ++i) {
				wrap_info_t info{ mod, pre, post };
				internal::wrap_function_clbck(
					"<unknown>",
					sr.adresses[i] - (size_t)mod->start,
					(void*)(&info));
			}
		}
		else if(method == Method::DBGSYMS)
		{
			wrap_info_t info{ mod, pre, post };
			drsym_error_t err = drsym_search_symbols(
				mod->full_path,
				name.c_str(),
				false,
				(drsym_enumerate_cb)internal::wrap_function_clbck,
				(void*)&info);
		}
		else if(method == Method::EXPORTS)
		{
			app_pc towrap = (app_pc)dr_get_proc_address(mod->handle, name.c_str());
			if (drwrap_wrap(towrap, pre, post)) {
				LOG_INFO(0, "Wrapped %s at %p", name.c_str(), towrap);
			}
		}
	}
}

void funwrap::wrap_allocations(const module_data_t *mod) {
	wrap_functions(mod, config.get_multi("functions", "allocators"), false, Method::EXPORTS, event::alloc_pre, event::alloc_post);
	wrap_functions(mod, config.get_multi("functions", "deallocators"), false, Method::EXPORTS, event::free_pre, nullptr);
}

bool starters_wrap_callback(const char *name, size_t modoffs, void *data) {
	module_data_t * mod = (module_data_t*)data;
	bool ok = drwrap_wrap_ex(
		mod->start + modoffs,
		funwrap::event::thread_creation,
		funwrap::event::thread_handover,
		(void*)name,
		DRWRAP_CALLCONV_FASTCALL);
	if (ok)
		LOG_INFO(0, "wrapped thread-start function %s", name);
	return true;
}

bool starters_sys_callback(const char *name, size_t modoffs, void *data) {
	module_data_t * mod = (module_data_t*)data;
	bool ok = drwrap_wrap_ex(
		mod->start + modoffs,
		funwrap::event::thread_pre_sys,
		funwrap::event::thread_post_sys,
		(void*)name,
		DRWRAP_CALLCONV_FASTCALL);
	if (ok)
		LOG_INFO(0, "wrapped system thread-start function %s", name);
	return true;
}

void funwrap::wrap_thread_start(const module_data_t *mod) {
	for (const auto & name : config.get_multi("functions", "thread_starters")) {
		drsym_error_t err = drsym_search_symbols(
			mod->full_path,
			name.c_str(),
			false,
			(drsym_enumerate_cb)starters_wrap_callback,
			(void*)mod);
	}
}

void funwrap::wrap_thread_start_sys(const module_data_t *mod) {
	for (const auto & name : config.get_multi("functions", "thread_starters_sys")) {
		drsym_error_t err = drsym_search_symbols(
			mod->full_path,
			name.c_str(),
			false,
			(drsym_enumerate_cb)starters_sys_callback,
			(void*)mod);
	}
}

bool exclude_wrap_callback(const char *name, size_t modoffs, void *data) {
		module_data_t * mod = (module_data_t*) data;
	bool ok = drwrap_wrap_ex(
		mod->start + modoffs,
		funwrap::event::begin_excl,
		funwrap::event::end_excl,
		(void*) name,
		DRWRAP_CALLCONV_FASTCALL);
	if (ok)
		LOG_INFO(0, "wrapped excluded function %s @ %p", name, mod->start + modoffs);
	return true;
}

void funwrap::wrap_excludes(const module_data_t *mod, std::string section) {
	wrap_functions(mod, config.get_multi(section, "exclude"), false, Method::DBGSYMS, event::begin_excl, event::end_excl);
}

void funwrap::wrap_mutexes(const module_data_t *mod, bool sys) {
	using namespace internal;

	if (sys) {
		// mutex lock
		wrap_functions(mod, config.get_multi("sync", "acquire_excl"), false, Method::EXPORTS, event::get_arg, event::mutex_lock);
		// mutex trylock
		wrap_functions(mod, config.get_multi("sync", "acquire_excl_try"), false, Method::EXPORTS, event::get_arg, event::mutex_trylock);
		// mutex unlock
		wrap_functions(mod, config.get_multi("sync", "release_excl"), false, Method::EXPORTS, event::mutex_unlock, NULL);
		// rec-mutex lock
		wrap_functions(mod, config.get_multi("sync", "acquire_rec"), false, Method::EXPORTS, event::get_arg, event::recmutex_lock);
		// rec-mutex trylock
		wrap_functions(mod, config.get_multi("sync", "acquire_rec_try"), false, Method::EXPORTS, event::get_arg, event::recmutex_trylock);
		// rec-mutex unlock
		wrap_functions(mod, config.get_multi("sync", "release_rec"), false, Method::EXPORTS, event::recmutex_unlock, NULL);
		// read-mutex lock
		wrap_functions(mod, config.get_multi("sync", "acquire_shared"), false, Method::EXPORTS, event::get_arg, event::mutex_read_lock);
		// read-mutex trylock
		wrap_functions(mod, config.get_multi("sync", "acquire_shared_try"), false, Method::EXPORTS, event::get_arg, event::mutex_read_trylock);
		// read-mutex unlock
		wrap_functions(mod, config.get_multi("sync", "release_shared"), false, Method::EXPORTS, event::mutex_read_unlock, NULL);
#ifdef WINDOWS
		// waitForSingleObject
		wrap_functions(mod, config.get_multi("sync", "wait_for_single_object"), false, Method::EXPORTS, event::get_arg, event::wait_for_single_obj);
		// waitForMultipleObjects
		wrap_functions(mod, config.get_multi("sync", "wait_for_multiple_objects"), false, Method::EXPORTS, event::wait_for_mo_getargs, event::wait_for_mult_obj);
#endif
	}
	else {
		LOG_INFO(0, "try to wrap non-system mutexes");
		// Std mutexes
		wrap_functions(mod, config.get_multi("stdsync", "acquire_excl"), false, Method::DBGSYMS, event::get_arg, event::mutex_lock);
		wrap_functions(mod, config.get_multi("stdsync", "acquire_excl_try"), false, Method::DBGSYMS, event::get_arg, event::mutex_trylock);
		wrap_functions(mod, config.get_multi("stdsync", "release_excl"), false, Method::DBGSYMS, event::mutex_unlock, NULL);

		// Qt Mutexes
		wrap_functions(mod, config.get_multi("qtsync", "acquire_excl"), false, Method::DBGSYMS, event::get_arg, event::recmutex_lock);
		wrap_functions(mod, config.get_multi("qtsync", "acquire_excl_try"), false, Method::DBGSYMS, event::get_arg, event::recmutex_trylock);
		wrap_functions(mod, config.get_multi("qtsync", "release_excl"), false, Method::DBGSYMS, event::recmutex_unlock, NULL);

		wrap_functions(mod, config.get_multi("qtsync", "acquire_shared"), false, Method::DBGSYMS, event::get_arg, event::mutex_read_lock);
		wrap_functions(mod, config.get_multi("qtsync", "acquire_shared_try"), false, Method::DBGSYMS, event::get_arg, event::mutex_read_trylock);
	}
}

void funwrap::wrap_sync_dotnet(const module_data_t *mod, bool native) {
	using namespace internal;

	std::string modname = util::basename(dr_module_preferred_name(mod));
	// Managed IPs
	if (!native) {
		wrap_functions(mod, config.get_multi("dotnetsync_rwlock", "acquire_excl"), true, Method::EXTERNAL_MPCR, event::get_arg, event::mutex_lock);
		wrap_functions(mod, config.get_multi("dotnetsync_rwlock", "acquire_excl_try"), true, Method::EXTERNAL_MPCR, event::get_arg, event::mutex_trylock);
		wrap_functions(mod, config.get_multi("dotnetsync_rwlock", "acquire_shared"), true, Method::EXTERNAL_MPCR, event::get_arg, event::mutex_read_lock);
		wrap_functions(mod, config.get_multi("dotnetsync_rwlock", "acquire_shared_try"), true, Method::EXTERNAL_MPCR, event::get_arg, event::mutex_read_trylock);
		// upgradable locks are semantically read-locks. However it is valid to upgrade it to a write lock without relinquishing the ressource
		wrap_functions(mod, config.get_multi("dotnetsync_rwlock", "acquire_upgrade"), true, Method::EXTERNAL_MPCR, event::get_arg, event::mutex_read_lock);
		wrap_functions(mod, config.get_multi("dotnetsync_rwlock", "acquire_upgrade_try"), true, Method::EXTERNAL_MPCR, event::get_arg, event::mutex_read_trylock);

		wrap_functions(mod, config.get_multi("dotnetsync_rwlock", "release_excl"), true, Method::EXTERNAL_MPCR, event::mutex_unlock, NULL);
		wrap_functions(mod, config.get_multi("dotnetsync_rwlock", "release_shared"), true, Method::EXTERNAL_MPCR, event::mutex_read_unlock, NULL);
		wrap_functions(mod, config.get_multi("dotnetexclude", "exclude"), true, Method::EXTERNAL_MPCR, event::begin_excl, event::end_excl);

		wrap_functions(mod, config.get_multi("dotnetsync_barrier", "wait_blocking"), true, Method::EXTERNAL_MPCR, event::barrier_enter, event::barrier_leave);
		wrap_functions(mod, config.get_multi("dotnetsync_barrier", "wait_nonblocking"), true, Method::EXTERNAL_MPCR, event::barrier_enter, event::barrier_leave_or_cancel);
	}
	else {
		// [native] JIT_MonExit
		// We also use MPCR here, as DR syms has problems finding the correct debug
		// information if multiple versions of a dll are in the cache
		LOG_INFO(-1, "try to wrap dotnetsync native");
		wrap_functions(mod, config.get_multi("dotnetsync_monitor", "monitor_enter"), false, Method::EXTERNAL_MPCR, event::get_arg, event::mutex_lock);
		wrap_functions(mod, config.get_multi("dotnetsync_monitor", "monitor_exit"), false, Method::EXTERNAL_MPCR, event::mutex_unlock, NULL);
	}
}

void funwrap::wrap_annotations(const module_data_t *mod) {
	LOG_INFO(0, "try to wrap annotations");
	// wrap happens before
	wrap_functions(mod, config.get_multi("sync", "happens_before"), false, Method::EXPORTS, event::get_arg, event::happens_before);
	wrap_functions(mod, config.get_multi("sync", "happens_after"), false, Method::EXPORTS, event::get_arg, event::happens_after);

	// wrap excludes
	wrap_functions(mod, config.get_multi("functions", "exclude_enter"), false, Method::EXPORTS, event::begin_excl, NULL);
	wrap_functions(mod, config.get_multi("functions", "exclude_leave"), false, Method::EXPORTS, NULL, event::end_excl);
}