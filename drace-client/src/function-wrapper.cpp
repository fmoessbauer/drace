#include "drace-client.h"
#include "function-wrapper.h"
#include "memory-instr.h"

#include <vector>
#include <string>

#include <detector_if.h>
#include <drwrap.h>

#include <unordered_map>

namespace funwrap {
	namespace internal {

		// TODO: get from config file
		static std::vector<std::string> acquire_symbols{ "_Mtx_lock",   "__gthrw_pthread_mutex_lock" };
		static std::vector<std::string> release_symbols{ "_Mtx_unlock", "__gthrw_pthread_mutex_unlock" };
#ifdef WINDOWS
		static std::vector<std::string> allocators{ "HeapAlloc" };
		static std::vector<std::string> deallocs{ "HeapFree" };
#else
		static std::vector<std::string> allocators{ "malloc" };
		static std::vector<std::string> deallocs{ "free" };
#endif

		static void mutex_acquire_pre(void *wrapctx, OUT void **user_data) {
			void *      mutex = drwrap_get_arg(wrapctx, 0);
			thread_id_t tid = dr_get_thread_id(drwrap_get_drcontext(wrapctx));

			memory_inst::process_buffer();

			detector::acquire(tid, mutex);

			dr_fprintf(STDERR, "<< [%i] pre mutex acquire %p\n", tid, mutex);
		}

		// Currently not bound as not necessary
		static void mutex_acquire_post(void *wrapctx, OUT void *user_data) {
			thread_id_t tid = dr_get_thread_id(drwrap_get_drcontext(wrapctx));
			dr_fprintf(STDERR, "<< [%i] post mutex acquire\n", tid);
		}

		static void mutex_release_pre(void *wrapctx, OUT void **user_data) {
			void *      mutex = drwrap_get_arg(wrapctx, 0);
			thread_id_t tid = dr_get_thread_id(drwrap_get_drcontext(wrapctx));

			memory_inst::process_buffer();

			detector::release(tid, mutex);

			dr_fprintf(STDERR, "<< [%i] pre mutex release %p\n", tid, mutex);
		}

		// Currently not bound as not necessary
		static void mutex_release_post(void *wrapctx, OUT void *user_data) {
			thread_id_t tid = dr_get_thread_id(drwrap_get_drcontext(wrapctx));
			dr_fprintf(STDERR, "<< [%i] post mutex release\n", tid);
		}

		// TODO: On Linux size is arg 0
		static void alloc_pre(void *wrapctx, void **user_data) {
			app_pc drcontext = drwrap_get_drcontext(wrapctx);

			memory_inst::process_buffer();

			// Save alloc size to TLS
			per_thread_t * data = (per_thread_t*)drmgr_get_tls_field(drcontext, tls_idx);
			data->last_alloc_size = (size_t)drwrap_get_arg(wrapctx, 2);

			// spams logs
			// dr_fprintf(STDERR, "<< [%i] pre alloc %i\n", tid, last_alloc_size);
		}

		static void alloc_post(void *wrapctx, void *user_data) {
			app_pc drcontext = drwrap_get_drcontext(wrapctx);
			void * retval = drwrap_get_retval(wrapctx);

			// Read alloc size from TLS
			per_thread_t * data = (per_thread_t*)drmgr_get_tls_field(drcontext, tls_idx);

			detector::alloc(data->tid, retval, data->last_alloc_size);

			// spams logs
			//dr_fprintf(STDERR, "<< [%i] post alloc %p\n", data->tid, retval);
		}

		// TODO: On Linux addr is arg 0
		static void free_pre(void *wrapctx, void **user_data) {
			void * addr = drwrap_get_arg(wrapctx, 2);
			thread_id_t tid = dr_get_thread_id(drwrap_get_drcontext(wrapctx));

			memory_inst::process_buffer();

			detector::free(tid, addr);

			// spams logs
			//dr_fprintf(STDERR, "<< [%i] pre free %p\n", tid, addr);
		}

		// Currently not bound as not necessary
		static void free_post(void *wrapctx, void *user_data) {
			thread_id_t tid = dr_get_thread_id(drwrap_get_drcontext(wrapctx));
			dr_fprintf(STDERR, "<< [%i] post free\n", tid);
		}

	} // namespace internal
} // namespace funwrap

void funwrap::wrap_mutex_acquire(const module_data_t *mod) {
	for (const auto & name : internal::acquire_symbols) {
		app_pc towrap = (app_pc)dr_get_proc_address(mod->handle, name.c_str());
		if (towrap != NULL) {
			bool ok = drwrap_wrap(towrap, internal::mutex_acquire_pre, NULL);
			if (ok) {
				std::string msg{ "< wrapped acq " };
				msg += name + "\n";
				dr_fprintf(STDERR, msg.c_str());
			}
		}
	}
}

void funwrap::wrap_mutex_release(const module_data_t *mod) {
	for (const auto & name : internal::release_symbols) {
		app_pc towrap = (app_pc)dr_get_proc_address(mod->handle, name.c_str());
		if (towrap != NULL) {
			bool ok = drwrap_wrap(towrap, internal::mutex_release_pre, NULL);
			if (ok) {
				std::string msg{ "< wrapped rel " };
				msg += name + "\n";
				dr_fprintf(STDERR, msg.c_str());
			}
		}
	}
}

void funwrap::wrap_allocators(const module_data_t *mod) {
	for (const auto & name : internal::allocators) {
		app_pc towrap = (app_pc)dr_get_proc_address(mod->handle, name.c_str());
		if (towrap != NULL) {
			bool ok = drwrap_wrap(towrap, internal::alloc_pre, internal::alloc_post);
			if (ok) {
				std::string msg{ "< wrapped alloc " };
				msg += name + "\n";
				dr_fprintf(STDERR, msg.c_str());
			}
		}
	}
}

void funwrap::wrap_deallocs(const module_data_t *mod) {
	for (const auto & name : internal::deallocs) {
		app_pc towrap = (app_pc)dr_get_proc_address(mod->handle, name.c_str());
		if (towrap != NULL) {
			bool ok = drwrap_wrap(towrap, internal::free_pre, NULL);
			if (ok) {
				std::string msg{ "< wrapped deallocs " };
				msg += name + "\n";
				dr_fprintf(STDERR, msg.c_str());
			}
		}
	}
}

//TODO: Currently not working
void funwrap::wrap_main(const module_data_t *mod) {
	app_pc towrap = (app_pc)dr_get_proc_address(mod->handle, "main");
	if (towrap != NULL) {
		dr_fprintf(STDERR, "< wrapped main\n");
	}
}
