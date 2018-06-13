#include "function-wrapper.h"

#include <vector>
#include <string>

#include <drwrap.h>

static std::vector<std::string> acquire_symbols{ "_Mtx_lock",   "__gthrw_pthread_mutex_lock"   };
static std::vector<std::string> release_symbols{ "_Mtx_unlock", "__gthrw_pthread_mutex_unlock" };

static void mutex_acquire_pre(void *wrapctx, OUT void **user_data) {
	void *      mutex = drwrap_get_arg(wrapctx, 0);
	thread_id_t tid   = dr_get_thread_id(drwrap_get_drcontext(wrapctx));
	dr_fprintf(STDERR, "<< [%i] pre mutex acquire %p\n", tid, mutex);
}

static void mutex_acquire_post(void *wrapctx, OUT void *user_data) {
	thread_id_t tid = dr_get_thread_id(drwrap_get_drcontext(wrapctx));
	dr_fprintf(STDERR, "<< [%i] post mutex acquire\n", tid);
}

static void mutex_release_pre(void *wrapctx, OUT void **user_data) {
	void *      mutex = drwrap_get_arg(wrapctx, 0);
	thread_id_t tid   = dr_get_thread_id(drwrap_get_drcontext(wrapctx));
	dr_fprintf(STDERR, "<< [%i] pre mutex release %p\n", tid, mutex);
}

static void mutex_release_post(void *wrapctx, OUT void *user_data) {
	thread_id_t tid = dr_get_thread_id(drwrap_get_drcontext(wrapctx));
	dr_fprintf(STDERR, "<< [%i] post mutex release\n", tid);
}

void wrap_mutex_acquire(const module_data_t *mod) {
	for (const auto & name : acquire_symbols) {
		app_pc towrap = (app_pc) dr_get_proc_address(mod->handle, name.c_str());
		if (towrap != NULL) {

			bool ok = drwrap_wrap(towrap, mutex_acquire_pre, mutex_acquire_post);
			if (ok) {
				std::string msg{ "< wrapped acq "};
				msg += name + "\n";
				dr_fprintf(STDERR, msg.c_str());
			}
		}
	}
}

void wrap_mutex_release(const module_data_t *mod) {
	for (const auto & name : release_symbols) {
		app_pc towrap = (app_pc)dr_get_proc_address(mod->handle, name.c_str());
		if (towrap != NULL) {
			bool ok = drwrap_wrap(towrap, mutex_release_pre, mutex_release_post);
			if (ok) {
				std::string msg{ "< wrapped rel "};
				msg += name + "\n";
				dr_fprintf(STDERR, msg.c_str());
			}
		}
	}
}
