/* Responsible for handling various mutex types 
* - Regular Mutexes (Non-Recursive)
* - Recursive Mutexes
* - R/W Locks
*
* All mutexe-types can be try-locked
*/

#include "globals.h"
#include "function-wrapper.h"
#include "module-tracker.h"
#include "memory-tracker.h"
#include "statistics.h"

#include <detector_if.h>
#include <dr_api.h>
#include <drwrap.h>
#include <drsyms.h>

using wrapcb_pre_t = void(void *, void **);
using wrapcb_post_t = void(void *, void *);

struct wrap_info_t {
	const module_data_t * mod;
	wrapcb_pre_t        * pre;
	wrapcb_post_t       * post;
};

/* Arguments of a WaitForMultipleObjects call */
struct wfmo_args_t {
	DWORD          ncount;
	BOOL           waitall;
	const HANDLE*  handles;
};

static inline void prepare_and_aquire(
	void* wrapctx,
	void* mutex,
	bool write,
	bool trylock)
{
	app_pc drcontext = drwrap_get_drcontext(wrapctx);
	per_thread_t * data = (per_thread_t*)drmgr_get_tls_field(drcontext, tls_idx);
	DR_ASSERT(nullptr != data);

	if (params.exclude_master && data->tid == runtime_tid)
		return;

	if (trylock) {
		int retval = (int)drwrap_get_retval(wrapctx);
		//dr_printf("[%.5i] Try Aquire %p, ret %i\n", data->tid, mutex, retval);
		// If retval == 0, mtx acquired
		// skip otherwise
		if (retval != 0)
			return;
	}
	LOG_TRACE(data->tid, "Aquire %p", mutex);

	// To avoid deadlock in flush-waiting spinlock,
	// acquire / release must not occur concurrently
	uint64_t cnt = ++(data->mutex_book[(uint64_t)mutex]);

	LOG_TRACE(data->tid, "Mutex book size: %i, count: %i, mutex: %p\n", data->mutex_book.size(), cnt, mutex);

	//dr_mutex_lock(th_mutex);
	MemoryTracker::flush_all_threads(data);
	detector::acquire(data->tid, mutex, cnt, write, trylock, data->detector_data);
	//dr_mutex_unlock(th_mutex);

	data->stats->mutex_ops++;
}

static inline void prepare_and_release(
	void* wrapctx,
	bool write)
{
	app_pc drcontext = drwrap_get_drcontext(wrapctx);
	per_thread_t * data = (per_thread_t*)drmgr_get_tls_field(drcontext, tls_idx);
	DR_ASSERT(nullptr != data);

	if (params.exclude_master && data->tid == runtime_tid)
		return;

	void* mutex = drwrap_get_arg(wrapctx, 0);
	LOG_TRACE(data->tid, "Release %p", mutex);

	if (!data->mutex_book.count((uint64_t)mutex)) {
		// mutex not in book
		return;
	}
	auto & cnt = --(data->mutex_book[(uint64_t)mutex]);
	if (cnt == 0) {
		data->mutex_book.erase((uint64_t)mutex);
	}

	// To avoid deadlock in flush-waiting spinlock,
	// acquire / release must not occur concurrently
	//dr_mutex_lock(th_mutex);
	MemoryTracker::flush_all_threads(data);
	detector::release(data->tid, mutex, write, data->detector_data);
	//dr_mutex_unlock(th_mutex);
}

namespace mutex_clb {
	/* Get addr of mutex */
	void get_arg(void *wrapctx, OUT void **user_data) {
		*user_data = drwrap_get_arg(wrapctx, 0);
	}

	void mutex_lock(void *wrapctx, void *user_data) {
		prepare_and_aquire(wrapctx, user_data, true, false);
	}

	void mutex_trylock(void *wrapctx, void *user_data) {
		prepare_and_aquire(wrapctx, user_data, true, true);
	}

	void mutex_unlock(void *wrapctx, OUT void **user_data) {
		prepare_and_release(wrapctx, true);
	}

	void recmutex_lock(void *wrapctx, void *user_data) {
		// TODO: Check recursive parameter
		prepare_and_aquire(wrapctx, user_data, true, false);
	}

	void recmutex_trylock(void *wrapctx, void *user_data) {
		prepare_and_aquire(wrapctx, user_data, true, true);
	}

	void recmutex_unlock(void *wrapctx, OUT void **user_data) {
		prepare_and_release(wrapctx, true);
	}

	void mutex_read_lock(void *wrapctx, void *user_data) {
		prepare_and_aquire(wrapctx, user_data, false, false);
	}

	void mutex_read_trylock(void *wrapctx, void *user_data) {
		prepare_and_aquire(wrapctx, user_data, false, true);
	}

	void mutex_read_unlock(void *wrapctx, OUT void **user_data) {
		prepare_and_release(wrapctx, false);
	}

#ifdef WINDOWS
	/* WaitForSingleObject Windows API call (experimental)
	* \warning the mutex parameter is the Handle ID of the mutex and not
	*          a memory location
	* \warning the return value is a DWORD (aka 32bit unsigned integer)
	* \warning A handle does not have to be a mutex,
	*          however distinction is not possible here
	*/
	void wait_for_single_obj(void *wrapctx, void *mutex) {
		// https://docs.microsoft.com/en-us/windows/desktop/api/synchapi/nf-synchapi-waitforsingleobject

		app_pc drcontext = drwrap_get_drcontext(wrapctx);
		per_thread_t * data = (per_thread_t*)drmgr_get_tls_field(drcontext, tls_idx);
		DR_ASSERT(nullptr != data);

		LOG_TRACE(data->tid, "waitForSingleObject: %p\n", mutex);

		if (params.exclude_master && data->tid == runtime_tid)
			return;

		DWORD retval = (DWORD)drwrap_get_retval(wrapctx);
		if (retval != WAIT_OBJECT_0) return;

		LOG_TRACE(data->tid, "waitForSingleObject: %p (Success)", mutex);

		uint64_t cnt = ++(data->mutex_book[(uint64_t)mutex]);
		MemoryTracker::flush_all_threads(data);
		detector::acquire(data->tid, mutex, cnt, 1, false, data->detector_data);
		data->stats->mutex_ops++;
	}

	void wait_for_mo_getargs(void *wrapctx, OUT void **user_data) {
		app_pc drcontext = drwrap_get_drcontext(wrapctx);
		per_thread_t * data = (per_thread_t*)drmgr_get_tls_field(drcontext, tls_idx);
		DR_ASSERT(nullptr != data);

		wfmo_args_t * args = (wfmo_args_t*)dr_thread_alloc(drcontext, sizeof(wfmo_args_t));

		args->ncount  = (DWORD)drwrap_get_arg(wrapctx, 0);
		args->handles = (const HANDLE*)drwrap_get_arg(wrapctx, 1);
		args->waitall = (BOOL)drwrap_get_arg(wrapctx, 2);

		LOG_TRACE(data->tid, "waitForMultipleObjects: %u, %i", args->ncount, args->waitall);

		*user_data = (void*)args;
	}

	/* WaitForMultipleObjects Windows API call (experimental) */
	void wait_for_mult_obj(void *wrapctx, void *user_data) {
		app_pc drcontext = drwrap_get_drcontext(wrapctx);
		per_thread_t * data = (per_thread_t*)drmgr_get_tls_field(drcontext, tls_idx);
		DWORD retval = (DWORD)drwrap_get_retval(wrapctx);

		wfmo_args_t * info = (wfmo_args_t*)user_data;

		MemoryTracker::flush_all_threads(data);
		if (info->waitall && (retval == WAIT_OBJECT_0)) {
			LOG_TRACE(data->tid, "waitForMultipleObjects:finished all");
			for (DWORD i = 0; i < info->ncount; ++i) {
				HANDLE mutex = info->handles[i];
				uint64_t cnt = ++(data->mutex_book[(uint64_t)mutex]);
				detector::acquire(data->tid, (void*)mutex, cnt, true, false, data->detector_data);
				data->stats->mutex_ops++;
			}
		}
		if (!info->waitall) {
			if (retval <= (WAIT_OBJECT_0 + info->ncount - 1)) {
				LOG_TRACE(data->tid, "waitForMultipleObjects:finished one");
				HANDLE mutex = info->handles[retval - WAIT_OBJECT_0];
				uint64_t cnt = ++(data->mutex_book[(uint64_t)mutex]);
				detector::acquire(data->tid, (void*)mutex, cnt, true, false, data->detector_data);
				data->stats->mutex_ops++;
			}
		}

		dr_thread_free(drcontext, user_data, sizeof(wfmo_args_t));
	}
#endif
}

namespace mutex_wrap {
	static bool mutex_wrap_callback(const char *name, size_t modoffs, void *data) {
		wrap_info_t * info = (wrap_info_t*)data;
		bool ok = drwrap_wrap_ex(
			info->mod->start + modoffs,
			info->pre,
			info->post,
			(void*)name,
			DRWRAP_CALLCONV_FASTCALL);
		if (ok)
			LOG_INFO(0, "wrapped custom mutex %s", name);

		delete info;
		// Exact matches only, hence quit after each symbol
		return false;
	}

	static void wrap_mtx_dbg(
		const module_data_t *mod,
		const std::vector<std::string> & syms,
		wrapcb_pre_t pre,
		wrapcb_post_t post)
	{
		for (const auto & name : syms) {
			wrap_info_t * info = new wrap_info_t;
			info->mod = mod;
			info->pre = pre;
			info->post = post;

			drsym_error_t err = drsym_search_symbols(
				mod->full_path,
				name.c_str(),
				false,
				(drsym_enumerate_cb)mutex_wrap_callback,
				(void*)info);
		}
	}

	static void wrap_mtx(
		const module_data_t *mod,
		const std::vector<std::string> & syms,
		wrapcb_pre_t pre,
		wrapcb_post_t post)
	{
		for (const auto & name : syms) {
			app_pc towrap = (app_pc)dr_get_proc_address(mod->handle, name.c_str());
			if (towrap != NULL) {
				bool ok = drwrap_wrap(towrap, pre, post);
				if (ok)
					LOG_INFO(0, "wrapped exported function %s", name.c_str());
			}
		}
	}
}

void funwrap::wrap_mutexes(const module_data_t *mod, bool sys) {
	using namespace mutex_wrap;
	using namespace mutex_clb;
	if (sys) {
		// mutex lock
		wrap_mtx(mod, config.get_multi("sync", "acquire_excl"), get_arg, mutex_lock);
		// mutex trylock
		wrap_mtx(mod, config.get_multi("sync", "acquire_excl_try"), get_arg, mutex_trylock);
		// mutex unlock
		wrap_mtx(mod, config.get_multi("sync", "release_excl"), mutex_unlock, NULL);
		// rec-mutex lock
		wrap_mtx(mod, config.get_multi("sync", "acquire_rec"), get_arg, recmutex_lock);
		// rec-mutex trylock
		wrap_mtx(mod, config.get_multi("sync", "acquire_rec_try"), get_arg, recmutex_trylock);
		// rec-mutex unlock
		wrap_mtx(mod, config.get_multi("sync", "release_rec"), recmutex_unlock, NULL);
		// read-mutex lock
		wrap_mtx(mod, config.get_multi("sync", "acquire_shared"), get_arg, mutex_read_lock);
		// read-mutex trylock
		wrap_mtx(mod, config.get_multi("sync", "acquire_shared_try"), get_arg, mutex_read_trylock);
		// read-mutex unlock
		wrap_mtx(mod, config.get_multi("sync", "release_shared"), mutex_read_unlock, NULL);
#ifdef WINDOWS
		// waitForSingleObject
		wrap_mtx(mod, config.get_multi("sync", "wait_for_single_object"), get_arg, wait_for_single_obj);
		// waitForMultipleObjects
		wrap_mtx(mod, config.get_multi("sync", "wait_for_multiple_objects"), wait_for_mo_getargs, wait_for_mult_obj);
#endif
	}
	else {
		LOG_INFO(0, "Try to wrap non-system mutexes");
		// Std mutexes
		wrap_mtx_dbg(mod, config.get_multi("stdsync", "acquire_excl"), get_arg, mutex_lock);
		wrap_mtx_dbg(mod, config.get_multi("stdsync", "acquire_excl_try"), get_arg, mutex_trylock);
		wrap_mtx_dbg(mod, config.get_multi("stdsync", "release_excl"), mutex_unlock, NULL);

		// Qt Mutexes
		wrap_mtx_dbg(mod, config.get_multi("qtsync", "acquire_excl"), get_arg, recmutex_lock);
		wrap_mtx_dbg(mod, config.get_multi("qtsync", "acquire_excl_try"), get_arg, recmutex_trylock);
		wrap_mtx_dbg(mod, config.get_multi("qtsync", "release_excl"), recmutex_unlock, NULL);

		wrap_mtx_dbg(mod, config.get_multi("qtsync", "acquire_shared"), get_arg, mutex_read_lock);
		wrap_mtx_dbg(mod, config.get_multi("qtsync", "acquire_shared_try"), get_arg, mutex_read_trylock);
	}
}