/* Responsible for handling various mutex types 
* - Regular Mutexes (Non-Recursive)
* - Recursive Mutexes
* - R/W Locks
*
* All mutexe-types can be try-locked
*/

#include "drace-client.h"
#include "function-wrapper.h"
#include "module-tracker.h"
#include <detector_if.h>
#include <dr_api.h>
#include <drwrap.h>

static inline void prepare_and_aquire(
	void* wrapctx,
	void* mutex,
	bool write,
	bool trylock)
{
	app_pc drcontext = drwrap_get_drcontext(wrapctx);
	per_thread_t * data = (per_thread_t*)drmgr_get_tls_field(drcontext, tls_idx);

	if (params.exclude_master && data->tid == runtime_tid)
		return;

	if (trylock) {
		bool aquired = (bool) drwrap_get_retval(wrapctx);
		//dr_printf("[%.5i] Try Aquire %p, success %i, rec: %i\n", data->tid, mutex, aquired, data->mutex_rec);
		if (!aquired)
			return;
	}

	// To avoid deadlock in flush-waiting spinlock,
	// acquire / release must not occur concurrently
	mutex_cntr_t & book = *(data->mutex_book);
	auto & cnt = book[(uint64_t)mutex];

	dr_mutex_lock(th_mutex);
	flush_all_threads(data);
	detector::acquire(data->tid, mutex, ++cnt, write, false, data->detector_data);
	dr_mutex_unlock(th_mutex);

	data->mutex_ops++;
}

static inline void prepare_and_release(
	void* wrapctx,
	bool write)
{
	app_pc drcontext = drwrap_get_drcontext(wrapctx);
	per_thread_t * data = (per_thread_t*)drmgr_get_tls_field(drcontext, tls_idx);

	if (params.exclude_master && data->tid == runtime_tid)
		return;

	void* mutex = drwrap_get_arg(wrapctx, 0);

	mutex_cntr_t & book = *(data->mutex_book);
	// mutex not in book
	if (book.count((uint64_t)mutex) == 0) {
		// This should not be necessary, but as early threads cannot be tracked
		// mutexes might be locked without book keeping.
		return;
	}
	auto & cnt = book[(uint64_t)mutex];

	if (cnt == 1) {
		book.erase((uint64_t)mutex);
	}
	else {
		--cnt;
	}

	// To avoid deadlock in flush-waiting spinlock,
	// acquire / release must not occur concurrently
	dr_mutex_lock(th_mutex);
	flush_all_threads(data);
	detector::release(data->tid, mutex, write, data->detector_data);
	dr_mutex_unlock(th_mutex);
}

namespace mutex_clb {
	/* Get addr of mutex */
	void get_arg(void *wrapctx, OUT void **user_data) {
		*user_data = drwrap_get_arg(wrapctx, 0);
	}

	void mutex_lock(void *wrapctx, OUT void *user_data) {
		prepare_and_aquire(wrapctx, user_data, true, false);
	}

	void mutex_trylock(void *wrapctx, OUT void *user_data) {
		prepare_and_aquire(wrapctx, user_data, true, true);
	}

	void mutex_unlock(void *wrapctx, OUT void **user_data) {
		prepare_and_release(wrapctx, true);
	}

	void recmutex_lock(void *wrapctx, OUT void *user_data) {
		// TODO: Check recursive parameter
		prepare_and_aquire(wrapctx, user_data, true, false);
	}

	void recmutex_trylock(void *wrapctx, OUT void *user_data) {
		prepare_and_aquire(wrapctx, user_data, true, true);
	}

	void recmutex_unlock(void *wrapctx, OUT void **user_data) {
		prepare_and_release(wrapctx, true);
	}

	void mutex_read_lock(void *wrapctx, OUT void *user_data) {
		prepare_and_aquire(wrapctx, user_data, false, false);
	}

	void mutex_read_trylock(void *wrapctx, OUT void *user_data) {
		prepare_and_aquire(wrapctx, user_data, false, true);
	}

	void mutex_read_unlock(void *wrapctx, OUT void **user_data) {
		prepare_and_release(wrapctx, false);
	}
}

namespace mutex_wrap {
	using wrapcb_pre_t  = void(void *, void **);
	using wrapcb_post_t = void(void *, void *);

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
					dr_fprintf(STDERR, "< wrapped mutex %s\n", name.c_str());
			}
		}
	}

}

void funwrap::wrap_mutexes(const module_data_t *mod) {
	using namespace mutex_wrap;
	using namespace mutex_clb;

	// mutex lock
	wrap_mtx(mod, config.get_multi("functions", "acquire_excl"), get_arg, mutex_lock);
	// mutex trylock
	wrap_mtx(mod, config.get_multi("functions", "acquire_excl_try"), get_arg, mutex_trylock);
	// mutex unlock
	wrap_mtx(mod, config.get_multi("functions", "release_excl"), mutex_unlock, NULL);
	// rec-mutex lock
	wrap_mtx(mod, config.get_multi("functions", "acquire_rec"), get_arg, recmutex_lock);
	// rec-mutex trylock
	wrap_mtx(mod, config.get_multi("functions", "acquire_rec_try"), get_arg, recmutex_trylock);
	// rec-mutex unlock
	wrap_mtx(mod, config.get_multi("functions", "release_rec"), recmutex_unlock, NULL);
	// read-mutex lock
	wrap_mtx(mod, config.get_multi("functions", "acquire_shared"), get_arg, mutex_read_lock);
	// read-mutex trylock
	wrap_mtx(mod, config.get_multi("functions", "acquire_shared_try"), get_arg, mutex_read_trylock);
	// read-mutex unlock
	wrap_mtx(mod, config.get_multi("functions", "release_shared"), mutex_read_unlock, NULL);
}