#include "globals.h"
#include "function-wrapper.h"
#include "memory-tracker.h"
#include "config.h"

#include <vector>
#include <string>

#include <detector_if.h>
#include <dr_api.h>
#include <drwrap.h>
#include <drutil.h>
#include <drsyms.h>

#include <unordered_map>

namespace funwrap {
	namespace internal {
		static void beg_excl_region(per_thread_t * data) {
			memory_tracker->process_buffer();
			data->enabled = false;
			data->event_cnt++;
		}

		static void end_excl_region(per_thread_t * data) {
			if (data->event_cnt == 1) {
				memory_tracker->clear_buffer();
				data->enabled = true;
			}
			data->event_cnt--;
		}

		// TODO: On Linux size is arg 0
		static void alloc_pre(void *wrapctx, void **user_data) {
			app_pc drcontext = drwrap_get_drcontext(wrapctx);

			// Save allocate size to TLS, disable detector
			per_thread_t * data = (per_thread_t*)drmgr_get_tls_field(drcontext, tls_idx);
			*user_data = drwrap_get_arg(wrapctx, 2);

			//beg_excl_region(data);

			// spams logs
			//dr_fprintf(STDERR, "<< [%i] pre allocate %i\n", data->tid, data->last_alloc_size);
		}

		static void alloc_post(void *wrapctx, void *user_data) {
			app_pc drcontext = drwrap_get_drcontext(wrapctx);
			void * retval    = drwrap_get_retval(wrapctx);
			void * pc = drwrap_get_func(wrapctx);

			// Read allocate size from TLS
			per_thread_t * data = (per_thread_t*)drmgr_get_tls_field(drcontext, tls_idx);

			// TODO: Validate if this has to be synchronized
			//dr_mutex_lock(th_mutex);
			//flush_all_threads(data);
			detector::allocate((detector::tid_t)data->tid, pc, retval, reinterpret_cast<size_t>(user_data), data->detector_data);
			//dr_mutex_unlock(th_mutex);

			// spams logs
			//dr_fprintf(STDERR, "<< [%i] post allocate: size %i, addr %p\n", data->tid, data->last_alloc_size, retval);
		}

		// TODO: On Linux addr is arg 0
		static void free_pre(void *wrapctx, void **user_data) {
			app_pc drcontext = drwrap_get_drcontext(wrapctx);
			per_thread_t * data = (per_thread_t*)drmgr_get_tls_field(drcontext, tls_idx);
			void * addr = drwrap_get_arg(wrapctx, 2);

			// TODO: Validate if this has to be synchronized
			//dr_mutex_lock(th_mutex);
			//flush_all_threads(data);
			detector::deallocate((detector::tid_t)data->tid, addr, data->detector_data);
			//dr_mutex_unlock(th_mutex);

			// spams logs
			//dr_fprintf(STDERR, "<< [%i] pre free %p\n", data->tid, addr);
		}

		static void free_post(void *wrapctx, void *user_data) {
			app_pc drcontext = drwrap_get_drcontext(wrapctx);
			per_thread_t * data = (per_thread_t*)drmgr_get_tls_field(drcontext, tls_idx);
			
			//end_excl_region(data);
			//dr_fprintf(STDERR, "<< [%i] post free\n", data->tid);
		}

		static void thread_creation(void *wrapctx, void **user_data) {
			app_pc drcontext = drwrap_get_drcontext(wrapctx);
			per_thread_t * data = (per_thread_t*)drmgr_get_tls_field(drcontext, tls_idx);
			
			beg_excl_region(data);
			// do not start threads concurrently as otherwise parent -> child relation
			// cannot be calculated
			dr_mutex_lock(th_start_mutex);
			th_start_pending.store(true);
			dr_printf("<< [%.5i] Setup New Thread\n", data->tid);
		}
		static void thread_handover(void *wrapctx, void *user_data) {
			app_pc drcontext = drwrap_get_drcontext(wrapctx);
			per_thread_t * data = (per_thread_t*)drmgr_get_tls_field(drcontext, tls_idx);
			
			end_excl_region(data);
			// Enable recently started thread
			auto last_th = last_th_start.load(std::memory_order_relaxed);
			auto & other_tls = TLS_buckets[last_th];
			if(other_tls->event_cnt == 0)
				TLS_buckets[last_th]->enabled = true;
			dr_mutex_unlock(th_start_mutex);
			dr_printf("<< [%.5i] New Thread Created: %.5i\n", data->tid, last_th_start.load());
		}

		static void thread_pre_sys(void *wrapctx, void **user_data) {
		}
		static void thread_post_sys(void *wrapctx, void *user_data) {
			app_pc drcontext = drwrap_get_drcontext(wrapctx);
			per_thread_t * data = (per_thread_t*)drmgr_get_tls_field(drcontext, tls_idx);

			detector::fork((detector::tid_t)runtime_tid.load(std::memory_order_relaxed), dr_get_thread_id(drcontext), data->detector_data);
		}

		static void begin_excl(void *wrapctx, void **user_data) {
			app_pc drcontext = drwrap_get_drcontext(wrapctx);
			per_thread_t * data = (per_thread_t*)drmgr_get_tls_field(drcontext, tls_idx);

			beg_excl_region(data);

			//dr_printf("<< [%.5i] Begin EXCLUDED REGION, stack: %i\n",
			//	data->tid, data->event_cnt);
		}

		static void end_excl(void *wrapctx, void *user_data) {
			app_pc drcontext = drwrap_get_drcontext(wrapctx);
			per_thread_t * data = (per_thread_t*)drmgr_get_tls_field(drcontext, tls_idx);

			end_excl_region(data);

			//dr_printf("<< [%.5i] End   EXCLUDED REGION: stack: %i\n",
			//	data->tid, data->event_cnt);
		}

	} // namespace internal
} // namespace funwrap

bool funwrap::init() {
	bool state = drwrap_init();

	// performance tuning
	drwrap_set_global_flags((drwrap_global_flags_t)(DRWRAP_NO_FRILLS | DRWRAP_FAST_CLEANCALLS));
	return state;
}

void funwrap::finalize() {
	drwrap_exit();
}

void funwrap::wrap_allocations(const module_data_t *mod) {
	for (const auto & name : config.get_multi("functions", "allocators")) {
		app_pc towrap = (app_pc)dr_get_proc_address(mod->handle, name.c_str());
		if (towrap != NULL) {
			bool ok = drwrap_wrap(towrap, internal::alloc_pre, internal::alloc_post);
			if (ok)
				dr_fprintf(STDERR, "< wrapped alloc %s\n", name.c_str());
		}
	}
	for (const auto & name : config.get_multi("functions", "deallocators")) {
		app_pc towrap = (app_pc)dr_get_proc_address(mod->handle, name.c_str());
		if (towrap != NULL) {
			bool ok = drwrap_wrap(towrap, internal::free_pre, NULL);
			if (ok)
				dr_fprintf(STDERR, "< wrapped deallocs %s\n", name.c_str());
		}
	}
}

bool starters_wrap_callback(const char *name, size_t modoffs, void *data) {
	module_data_t * mod = (module_data_t*)data;
	bool ok = drwrap_wrap_ex(
		mod->start + modoffs,
		funwrap::internal::thread_creation,
		funwrap::internal::thread_handover,
		(void*)name,
		DRWRAP_CALLCONV_FASTCALL);
	if (ok) {
		dr_fprintf(STDERR, "< wrapped thread-start function %s\n", name);
	}
	return true;
}

bool starters_sys_callback(const char *name, size_t modoffs, void *data) {
	module_data_t * mod = (module_data_t*)data;
	bool ok = drwrap_wrap_ex(
		mod->start + modoffs,
		funwrap::internal::thread_pre_sys,
		funwrap::internal::thread_post_sys,
		(void*)name,
		DRWRAP_CALLCONV_FASTCALL);
	if (ok) {
		dr_fprintf(STDERR, "< wrapped system thread-start function %s\n", name);
	}
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
		funwrap::internal::begin_excl,
		funwrap::internal::end_excl,
		(void*) name,
		DRWRAP_CALLCONV_FASTCALL);
	if (ok) {
		dr_fprintf(STDERR, "< wrapped excluded function %s\n", name);
	}
	return true;
}

void funwrap::wrap_excludes(const module_data_t *mod) {
	for (const auto & name : config.get_multi("functions", "exclude")) {
		drsym_error_t err = drsym_search_symbols(
			mod->full_path,
			name.c_str(),
			false,
			(drsym_enumerate_cb)exclude_wrap_callback,
			(void*)mod);
	}
}
