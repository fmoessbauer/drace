#include "globals.h"
#include "util.h"
#include "function-wrapper.h"
#include "memory-tracker.h"
#include "symbols.h"
#include "statistics.h"
#include "ThreadState.h"
#include <detector/detector_if.h>

#include <dr_api.h>
#include <drmgr.h>
#include <drwrap.h>

namespace drace {
	namespace funwrap {

		void event::beg_excl_region(ThreadState * data) {
			// We do not flush here, as in disabled state no
			// refs are recorded
			//memory_tracker->process_buffer();
			LOG_TRACE(data->tid, "Begin excluded region");
			data->mtrack.disable_scope();
		}

		void event::end_excl_region(ThreadState * data) {
			LOG_TRACE(data->tid, "End excluded region");
			data->mtrack.enable_scope();
		}

		// TODO: On Linux size is arg 0
		void event::alloc_pre(void *wrapctx, void **user_data) {
			app_pc drcontext = drwrap_get_drcontext(wrapctx);
			ThreadState * data = (ThreadState*)drmgr_get_tls_field(drcontext, tls_idx);

			MemoryTracker::flush_all_threads(data->mtrack);
			// Save allocate size to user_data
			// we use the pointer directly to avoid an allocation
			//per_thread_t * data = (per_thread_t*)drmgr_get_tls_field(drcontext, tls_idx);
			*user_data = drwrap_get_arg(wrapctx, 2);
		}

		void event::alloc_post(void *wrapctx, void *user_data) {
			app_pc drcontext = drwrap_get_drcontext(wrapctx);
			void * retval = drwrap_get_retval(wrapctx);
			void * pc = drwrap_get_func(wrapctx);
			size_t size = reinterpret_cast<size_t>(user_data);

			ThreadState * data = (ThreadState*)drmgr_get_tls_field(drcontext, tls_idx);
			DR_ASSERT(nullptr != data);

			//MemoryTracker::flush_all_threads(data);

			// allocations with size 0 are valid if they come from
			// reallocate (in fact, that's a free)
			if (size != 0) {
				// to avoid high pressure on the internal spinlock,
				// we lock externally using a os lock
				// TODO: optimize tsan wrapper internally
				dr_mutex_lock(th_mutex);
				//detector::happens_after(data->tid, retval);
				detector::allocate(data->detector_data, pc, retval, size);
				dr_mutex_unlock(th_mutex);
			}
		}

		void event::realloc_pre(void *wrapctx, void **user_data) {
			app_pc drcontext = drwrap_get_drcontext(wrapctx);
			ThreadState * data = (ThreadState*)drmgr_get_tls_field(drcontext, tls_idx);

			MemoryTracker::flush_all_threads(data->mtrack);

			// first deallocate, then allocate again
			void* old_addr = drwrap_get_arg(wrapctx, 2);

			// to avoid high pressure on the internal spinlock,
			// we lock externally using a os lock
			// TODO: optimize tsan wrapper internally
			dr_mutex_lock(th_mutex);
			detector::deallocate(data->detector_data, old_addr);
			//detector::happens_before(data->tid, old_addr);
			dr_mutex_unlock(th_mutex);

			*user_data = drwrap_get_arg(wrapctx, 3);
			//LOG_INFO(data->tid, "reallocate, new blocksize %u at %p", (SIZE_T)*user_data, old_addr);
		}

		// TODO: On Linux addr is arg 0
		void event::free_pre(void *wrapctx, void **user_data) {
			app_pc drcontext = drwrap_get_drcontext(wrapctx);
			ThreadState * data = (ThreadState*)drmgr_get_tls_field(drcontext, tls_idx);
			DR_ASSERT(nullptr != data);

			void * addr = drwrap_get_arg(wrapctx, 2);

			MemoryTracker::flush_all_threads(data->mtrack);

			// TODO: optimize tsan wrapper internally (see comment in alloc_post)
			dr_mutex_lock(th_mutex);
			detector::deallocate(data->detector_data, addr);
			//detector::happens_before(data->tid, addr);
			dr_mutex_unlock(th_mutex);
		}

		void event::free_post(void *wrapctx, void *user_data) {
			//app_pc drcontext = drwrap_get_drcontext(wrapctx);
			//per_thread_t * data = (per_thread_t*)drmgr_get_tls_field(drcontext, tls_idx);

			//end_excl_region(data);
			//dr_fprintf(STDERR, "<< [%i] post free\n", data->tid);
		}

		void event::thread_creation(void *wrapctx, void **user_data) {
			app_pc drcontext = drwrap_get_drcontext(wrapctx);
			ThreadState * data = (ThreadState*)drmgr_get_tls_field(drcontext, tls_idx);
			DR_ASSERT(nullptr != data);

			beg_excl_region(data);

			th_start_pending.store(true);
			LOG_INFO(data->tid, "setup new thread");
		}
		void event::thread_handover(void *wrapctx, void *user_data) {
			app_pc drcontext = drwrap_get_drcontext(wrapctx);
			ThreadState * data = (ThreadState*)drmgr_get_tls_field(drcontext, tls_idx);
			DR_ASSERT(nullptr != data);

			end_excl_region(data);
			// Enable recently started thread
			auto last_th = last_th_start.load(std::memory_order_relaxed);
			// TLS is already updated, hence read lock is sufficient
			dr_rwlock_read_lock(tls_rw_mutex);
			if (TLS_buckets.count(last_th) == 1) {
				auto & other_tls = TLS_buckets[last_th];
				other_tls->mtrack.enable_scope();
			}
			dr_rwlock_read_unlock(tls_rw_mutex);
			LOG_INFO(data->tid, "new thread created: %i", last_th_start.load());
		}

		void event::thread_pre_sys(void *wrapctx, void **user_data) {
		}

		/* Deprecated as this did never really work */
		void event::thread_post_sys(void *wrapctx, void *user_data) {
			return; // TODO: Fix or remove this code

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

		void event::begin_excl(void *wrapctx, void **user_data) {
			app_pc drcontext = drwrap_get_drcontext(wrapctx);
			ThreadState * data = (ThreadState*)drmgr_get_tls_field(drcontext, tls_idx);
			DR_ASSERT(nullptr != data);

			beg_excl_region(data);
		}

		void event::end_excl(void *wrapctx, void *user_data) {
			app_pc drcontext = drwrap_get_drcontext(wrapctx);
			ThreadState * data = (ThreadState*)drmgr_get_tls_field(drcontext, tls_idx);
			DR_ASSERT(nullptr != data);

			end_excl_region(data);
		}

		void event::dotnet_enter(void *wrapctx, void **user_data) { }
		void event::dotnet_leave(void *wrapctx, void *user_data) { }

		// --------------------------------------------------------------------------------------
		// ------------------------------------- Mutex Events -----------------------------------
		// --------------------------------------------------------------------------------------

		void event::prepare_and_aquire(
			void* wrapctx,
			void* mutex,
			bool write,
			bool trylock)
		{
			app_pc drcontext = drwrap_get_drcontext(wrapctx);
			ThreadState * data = (ThreadState*)drmgr_get_tls_field(drcontext, tls_idx);
			DR_ASSERT(nullptr != data);

			if (params.exclude_master && data->tid == runtime_tid)
				return;

			if (trylock) {
				int retval = (int)drwrap_get_retval(wrapctx);
				LOG_TRACE(data->tid, "Try Aquire %p, res: %i", mutex, retval);
				//dr_printf("[%.5i] Try Aquire %p, ret %i\n", data->tid, mutex, retval);
				// If retval == 0, mtx acquired
				// skip otherwise
				if (retval != 0)
					return;
			}
			LOG_TRACE(data->tid, "Aquire  %p : %s", mutex, module_tracker->_syms->get_symbol_info(drwrap_get_func(wrapctx)).sym_name.c_str());

			// To avoid deadlock in flush-waiting spinlock,
			// acquire / release must not occur concurrently
			uint64_t cnt = ++(data->mutex_book[(uint64_t)mutex]);

			LOG_TRACE(data->tid, "Mutex book size: %i, count: %i, mutex: %p\n", data->mutex_book.size(), cnt, mutex);

			detector::acquire(data->detector_data, mutex, cnt, write);
			//detector::happens_after(data->tid, mutex);

			data->stats.mutex_ops++;
		}

		void event::prepare_and_release(
			void* wrapctx,
			bool write)
		{
			app_pc drcontext = drwrap_get_drcontext(wrapctx);
			ThreadState * data = (ThreadState*)drmgr_get_tls_field(drcontext, tls_idx);
			DR_ASSERT(nullptr != data);

			if (params.exclude_master && data->tid == runtime_tid)
				return;

			void* mutex = drwrap_get_arg(wrapctx, 0);
			//detector::happens_before(data->tid, mutex);

			if (!data->mutex_book.count((uint64_t)mutex)) {
				LOG_TRACE(data->tid, "Mutex Error %p at : %s", mutex, module_tracker->_syms->get_symbol_info(drwrap_get_func(wrapctx)).sym_name.c_str());
				// mutex not in book
				return;
			}
			auto & cnt = --(data->mutex_book[(uint64_t)mutex]);
			if (cnt == 0) {
				data->mutex_book.erase((uint64_t)mutex);
			}

			// To avoid deadlock in flush-waiting spinlock,
			// acquire / release must not occur concurrently

			MemoryTracker::flush_all_threads(data->mtrack);
			LOG_TRACE(data->tid, "Release %p : %s", mutex, module_tracker->_syms->get_symbol_info(drwrap_get_func(wrapctx)).sym_name.c_str());
			detector::release(data->detector_data, mutex, write);
		}

		void event::get_arg(void *wrapctx, OUT void **user_data) {
			*user_data = drwrap_get_arg(wrapctx, 0);

			app_pc drcontext = drwrap_get_drcontext(wrapctx);
			ThreadState * data = (ThreadState*)drmgr_get_tls_field(drcontext, tls_idx);
			// we flush here to avoid tracking sync-function itself
			MemoryTracker::flush_all_threads(data->mtrack);
		}

		void event::get_arg_dotnet(void *wrapctx, OUT void **user_data) {
			LOG_INFO(-1, "Hit Whatever with arg %p", drwrap_get_arg(wrapctx, 0));
			*user_data = drwrap_get_arg(wrapctx, 0);
		}

		void event::mutex_lock(void *wrapctx, void *user_data) {
			prepare_and_aquire(wrapctx, user_data, true, false);
		}

		void event::mutex_trylock(void *wrapctx, void *user_data) {
			prepare_and_aquire(wrapctx, user_data, true, true);
		}

		void event::mutex_unlock(void *wrapctx, OUT void **user_data) {
			prepare_and_release(wrapctx, true);
		}

		void event::recmutex_lock(void *wrapctx, void *user_data) {
			// TODO: Check recursive parameter
			event::prepare_and_aquire(wrapctx, user_data, true, false);
		}

		void event::recmutex_trylock(void *wrapctx, void *user_data) {
			prepare_and_aquire(wrapctx, user_data, true, true);
		}

		void event::recmutex_unlock(void *wrapctx, OUT void **user_data) {
			prepare_and_release(wrapctx, true);
		}

		void event::mutex_read_lock(void *wrapctx, void *user_data) {
			prepare_and_aquire(wrapctx, user_data, false, false);
		}

		void event::mutex_read_trylock(void *wrapctx, void *user_data) {
			prepare_and_aquire(wrapctx, user_data, false, true);
		}

		void event::mutex_read_unlock(void *wrapctx, OUT void **user_data) {
			prepare_and_release(wrapctx, false);
		}

#ifdef WINDOWS
		void event::wait_for_single_obj(void *wrapctx, void *mutex) {
			// https://docs.microsoft.com/en-us/windows/desktop/api/synchapi/nf-synchapi-waitforsingleobject

			app_pc drcontext = drwrap_get_drcontext(wrapctx);
			ThreadState * data = (ThreadState*)drmgr_get_tls_field(drcontext, tls_idx);
			DR_ASSERT(nullptr != data);

			LOG_TRACE(data->tid, "waitForSingleObject: %p\n", mutex);

			if (params.exclude_master && data->tid == runtime_tid)
				return;

			DWORD retval = (DWORD)drwrap_get_retval(wrapctx);
			if (retval != WAIT_OBJECT_0) return;

			LOG_TRACE(data->tid, "waitForSingleObject: %p (Success)", mutex);

			uint64_t cnt = ++(data->mutex_book[(uint64_t)mutex]);
			MemoryTracker::flush_all_threads(data->mtrack);
			detector::acquire(data->detector_data, mutex, cnt, 1);
			data->stats.mutex_ops++;
		}

		void event::wait_for_mo_getargs(void *wrapctx, OUT void **user_data) {
			app_pc drcontext = drwrap_get_drcontext(wrapctx);
			ThreadState * data = (ThreadState*)drmgr_get_tls_field(drcontext, tls_idx);
			DR_ASSERT(nullptr != data);

			wfmo_args_t * args = (wfmo_args_t*)dr_thread_alloc(drcontext, sizeof(wfmo_args_t));

			args->ncount = (DWORD)drwrap_get_arg(wrapctx, 0);
			args->handles = (const HANDLE*)drwrap_get_arg(wrapctx, 1);
			args->waitall = (BOOL)drwrap_get_arg(wrapctx, 2);

			LOG_TRACE(data->tid, "waitForMultipleObjects: %u, %i", args->ncount, args->waitall);

			*user_data = (void*)args;
		}

		void event::wait_for_mult_obj(void *wrapctx, void *user_data) {
			app_pc drcontext = drwrap_get_drcontext(wrapctx);
			ThreadState * data = (ThreadState*)drmgr_get_tls_field(drcontext, tls_idx);
			DWORD retval = (DWORD)drwrap_get_retval(wrapctx);

			wfmo_args_t * info = (wfmo_args_t*)user_data;

			MemoryTracker::flush_all_threads(data->mtrack);
			if (info->waitall && (retval == WAIT_OBJECT_0)) {
				LOG_TRACE(data->tid, "waitForMultipleObjects:finished all");
				for (DWORD i = 0; i < info->ncount; ++i) {
					HANDLE mutex = info->handles[i];
					uint64_t cnt = ++(data->mutex_book[(uint64_t)mutex]);
					detector::acquire(data->detector_data, (void*)mutex, cnt, true);
					data->stats.mutex_ops++;
				}
			}
			if (!info->waitall) {
				if (retval <= (WAIT_OBJECT_0 + info->ncount - 1)) {
					HANDLE mutex = info->handles[retval - WAIT_OBJECT_0];
					LOG_TRACE(data->tid, "waitForMultipleObjects:finished one: %p", mutex);
					uint64_t cnt = ++(data->mutex_book[(uint64_t)mutex]);
					detector::acquire(data->detector_data, (void*)mutex, cnt, true);
					data->stats.mutex_ops++;
				}
			}

			dr_thread_free(drcontext, user_data, sizeof(wfmo_args_t));
		}

		void event::barrier_enter(void *wrapctx, void** addr) {
			app_pc drcontext = drwrap_get_drcontext(wrapctx);
			ThreadState * data = (ThreadState*)drmgr_get_tls_field(drcontext, tls_idx);
			DR_ASSERT(nullptr != data);
			*addr = drwrap_get_arg(wrapctx, 0);
			LOG_TRACE(data->tid, "barrier enter %p", *addr);
			// each thread enters the barrier individually
			detector::happens_before(data->tid, *addr);
		}

		void event::barrier_leave(void *wrapctx, void *addr) {
			app_pc drcontext = drwrap_get_drcontext(wrapctx);
			ThreadState * data = (ThreadState*)drmgr_get_tls_field(drcontext, tls_idx);
			DR_ASSERT(nullptr != data);

			LOG_TRACE(data->tid, "barrier passed");

			// each thread leaves individually, but only after all barrier_enters have been called
			detector::happens_after(data->tid, addr);
		}

		void event::barrier_leave_or_cancel(void *wrapctx, void *addr) {
			app_pc drcontext = drwrap_get_drcontext(wrapctx);
			ThreadState * data = (ThreadState*)drmgr_get_tls_field(drcontext, tls_idx);
			DR_ASSERT(nullptr != data);

			bool passed = (bool)drwrap_get_retval(wrapctx);
			LOG_TRACE(data->tid, "barrier left with status %i", passed);

			// TODO: Validate cancellation path, where happens_before will be called again
			if (passed) {
				// each thread leaves individually, but only after all barrier_enters have been called
				detector::happens_after(data->tid, addr);
			}
		}

		void event::happens_before(void *wrapctx, void *identifier) {
			app_pc drcontext = drwrap_get_drcontext(wrapctx);
			ThreadState * data = (ThreadState*)drmgr_get_tls_field(drcontext, tls_idx);
			DR_ASSERT(nullptr != data);

			detector::happens_before(data->tid, identifier);
			LOG_TRACE(data->tid, "happens-before @ %p", identifier);
		}

		void event::happens_after(void *wrapctx, void *identifier) {
			app_pc drcontext = drwrap_get_drcontext(wrapctx);
			ThreadState * data = (ThreadState*)drmgr_get_tls_field(drcontext, tls_idx);
			DR_ASSERT(nullptr != data);

			detector::happens_after(data->tid, identifier);
			LOG_TRACE(data->tid, "happens-after  @ %p", identifier);
		}
#endif
	} // namespace funwrap
} // namespace drace
