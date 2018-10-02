#include "globals.h"
#include "function-wrapper.h"
#include "memory-tracker.h"
#include "config.h"

#include <vector>
#include <string>
#include <functional>

#include <detector_if.h>
#include <dr_api.h>
#include <drwrap.h>
#include <drutil.h>
#include <drsyms.h>

#include <unordered_map>

namespace funwrap {
	namespace internal {
		static void beg_excl_region(per_thread_t * data) {
			// We do not flush here, as in disabled state no
			// refs are recorded
			//memory_tracker->process_buffer();
			data->enabled = false;
			data->event_cnt++;
		}

		static void end_excl_region(per_thread_t * data) {
			if (data->event_cnt == 1) {
				//memory_tracker->clear_buffer();
				data->enabled = true;
			}
			data->event_cnt--;
		}

		// TODO: On Linux size is arg 0
		static void alloc_pre(void *wrapctx, void **user_data) {
			app_pc drcontext = drwrap_get_drcontext(wrapctx);

			// Save allocate size to user_data
			// we use the pointer directly to avoid an allocation
			//per_thread_t * data = (per_thread_t*)drmgr_get_tls_field(drcontext, tls_idx);
			*user_data = drwrap_get_arg(wrapctx, 2);

			//beg_excl_region(data);

			// spams logs
			//dr_fprintf(STDERR, "<< [%i] pre allocate %i\n", data->tid, data->last_alloc_size);
		}

		static void alloc_post(void *wrapctx, void *user_data) {
			app_pc drcontext = drwrap_get_drcontext(wrapctx);
			void * retval    = drwrap_get_retval(wrapctx);
			void * pc = drwrap_get_func(wrapctx);

			per_thread_t * data = (per_thread_t*)drmgr_get_tls_field(drcontext, tls_idx);
			DR_ASSERT(nullptr != data);

			MemoryTracker::flush_all_threads(data);
			detector::allocate(data->tid, pc, retval, reinterpret_cast<size_t>(user_data), data->detector_data);

			// spams logs
			//dr_fprintf(STDERR, "<< [%i] post allocate: size %i, addr %p\n", data->tid, user_data, retval);
		}

		// TODO: On Linux addr is arg 0
		static void free_pre(void *wrapctx, void **user_data) {
			app_pc drcontext = drwrap_get_drcontext(wrapctx);
			per_thread_t * data = (per_thread_t*)drmgr_get_tls_field(drcontext, tls_idx);
			DR_ASSERT(nullptr != data);

			void * addr = drwrap_get_arg(wrapctx, 2);

			MemoryTracker::flush_all_threads(data);
			detector::deallocate(data->tid, addr, data->detector_data);

			// spams logs
			//dr_fprintf(STDERR, "<< [%i] pre free %p\n", data->tid, addr);
		}

		static void free_post(void *wrapctx, void *user_data) {
			//app_pc drcontext = drwrap_get_drcontext(wrapctx);
			//per_thread_t * data = (per_thread_t*)drmgr_get_tls_field(drcontext, tls_idx);
			
			//end_excl_region(data);
			//dr_fprintf(STDERR, "<< [%i] post free\n", data->tid);
		}

		static void thread_creation(void *wrapctx, void **user_data) {
			app_pc drcontext = drwrap_get_drcontext(wrapctx);
			per_thread_t * data = (per_thread_t*)drmgr_get_tls_field(drcontext, tls_idx);
			DR_ASSERT(nullptr != data);
			
			beg_excl_region(data);

			th_start_pending.store(true);
			LOG_INFO(data->tid, "setup new thread");
		}
		static void thread_handover(void *wrapctx, void *user_data) {
			app_pc drcontext = drwrap_get_drcontext(wrapctx);
			per_thread_t * data = (per_thread_t*)drmgr_get_tls_field(drcontext, tls_idx);
			DR_ASSERT(nullptr != data);
			
			end_excl_region(data);
			// Enable recently started thread
			auto last_th = last_th_start.load(std::memory_order_relaxed);
			// TLS is already updated, hence read lock is sufficient
			dr_rwlock_read_lock(tls_rw_mutex);
			if (TLS_buckets.count(last_th) == 1) {
				auto & other_tls = TLS_buckets[last_th];
				if (other_tls->event_cnt == 0)
					TLS_buckets[last_th]->enabled = true;
			}
			dr_rwlock_read_unlock(tls_rw_mutex);
			LOG_INFO(data->tid, "new thread created: %i", last_th_start.load());
		}

		static void thread_pre_sys(void *wrapctx, void **user_data) {
		}
		static void thread_post_sys(void *wrapctx, void *user_data) {
			//app_pc drcontext = drwrap_get_drcontext(wrapctx);
			//per_thread_t * data = (per_thread_t*)drmgr_get_tls_field(drcontext, tls_idx);
			dr_rwlock_read_lock(tls_rw_mutex);
			auto other_th = last_th_start.load(std::memory_order_acquire);
			// There are some spurious failures where the thread init event
			// is not called but the system call has already returned
			// Hence, skip the fork here and rely on fallback-fork in
			// analyze_access
			if (TLS_buckets.count(other_th) == 1) {
				auto other_tls = TLS_buckets[other_th];
				//MemoryTracker::flush_all_threads(data, false);
				//detector::fork(dr_get_thread_id(drcontext), other_tls->tid, &(other_tls->detector_data));
			}
			dr_rwlock_read_unlock(tls_rw_mutex);
		}

		static void begin_excl(void *wrapctx, void **user_data) {
			app_pc drcontext = drwrap_get_drcontext(wrapctx);
			per_thread_t * data = (per_thread_t*)drmgr_get_tls_field(drcontext, tls_idx);
			DR_ASSERT(nullptr != data);

			beg_excl_region(data);
		}

		static void end_excl(void *wrapctx, void *user_data) {
			app_pc drcontext = drwrap_get_drcontext(wrapctx);
			per_thread_t * data = (per_thread_t*)drmgr_get_tls_field(drcontext, tls_idx);
			DR_ASSERT(nullptr != data);

			end_excl_region(data);
		}

		static void dotnet_enter(void *wrapctx, void **user_data) {
			LOG_NOTICE(0, "enter dotnet function");
		}
		static void dotnet_leave(void *wrapctx, void *user_data) {
			LOG_NOTICE(0, "leave dotnet function");
		}

		void wrap_dotnet_helper(uint64_t addr) {
			bool ok = drwrap_wrap_ex(
				(app_pc)addr,
				funwrap::internal::dotnet_enter,
				funwrap::internal::dotnet_leave,
				nullptr,
				DRWRAP_CALLCONV_FASTCALL);
			if (ok)
				LOG_INFO(0, "- wrapped dotnet function @ %p", addr);
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
			if (ok) {
				LOG_INFO(0, "wrapped alloc %s", name.c_str());
			}
		}
	}
	for (const auto & name : config.get_multi("functions", "deallocators")) {
		app_pc towrap = (app_pc)dr_get_proc_address(mod->handle, name.c_str());
		if (towrap != NULL) {
			bool ok = drwrap_wrap(towrap, internal::free_pre, NULL);
			if (ok)
				LOG_INFO(0, "wrapped deallocs %s", name.c_str());
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
	if (ok)
		LOG_INFO(0, "wrapped thread-start function %s", name);
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
		funwrap::internal::begin_excl,
		funwrap::internal::end_excl,
		(void*) name,
		DRWRAP_CALLCONV_FASTCALL);
	if (ok)
		LOG_INFO(0, "wrapped excluded function %s @ %p", name, mod->start + modoffs);
	return true;
}

void funwrap::wrap_excludes(const module_data_t *mod, std::string section) {
	for (const auto & name : config.get_multi(section, "exclude")) {
		drsym_error_t err = drsym_search_symbols(
			mod->full_path,
			name.c_str(),
			false,
			(drsym_enumerate_cb)exclude_wrap_callback,
			(void*)mod);
	}
}

// TODO: Obsolete Code
bool dotnet_wrap_callback(const char *name, size_t modoffs, void *data) {
	module_data_t * mod = (module_data_t*)data;
	LOG_INFO(0, "Hit DotNet function %s @ %p", name, mod->start + modoffs);
	return true;
}

bool dotnet_wrap_callback2(const char *name, size_t modoffs, void *data) {
	module_data_t * mod = (module_data_t*)data;
	LOG_INFO(0, "Hit DotNet function %s @ %p", name, mod->start + modoffs);
	uint64_t addr = (uint64_t)mod->start + modoffs;

	bool ok = drwrap_wrap_ex(
		(app_pc)addr,
		funwrap::internal::dotnet_enter,
		NULL,
		nullptr,
		DRWRAP_CALLCONV_FASTCALL);
	if (ok)
		LOG_INFO(0, "- wrapped dotnet enter function @ %p", addr);

	return true;
}

bool dotnet_wrap_callback3(const char *name, size_t modoffs, void *data) {
	module_data_t * mod = (module_data_t*)data;
	LOG_INFO(0, "Hit DotNet function %s @ %p", name, mod->start + modoffs);
	uint64_t addr = (uint64_t)mod->start + modoffs;

	bool ok = drwrap_wrap_ex(
		(app_pc)addr,
		NULL,
		funwrap::internal::dotnet_leave,
		nullptr,
		DRWRAP_CALLCONV_FASTCALL);
	if (ok)
		LOG_INFO(0, "- wrapped dotnet leave function @ %p", addr);
	return true;
}