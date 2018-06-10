#include "function-wrapper.h"

#include <vector>
#include <string>

#include <drwrap.h>

static std::vector<std::string> acquire_symbols{ "_Mtx_lock" };
static std::vector<std::string> release_symbols{ "_Mtx_unlock" };

static void mutex_acquire_pre(void *wrapcxt, OUT void **user_data) {
	dr_fprintf(STDERR, "<< pre mutex acquire\n");
}

static void mutex_acquire_post(void *wrapcxt, OUT void *user_data) {
	dr_fprintf(STDERR, "<< post mutex acquire\n");
}

static void mutex_release_pre(void *wrapcxt, OUT void **user_data) {
	dr_fprintf(STDERR, "<< pre mutex release\n");
}

static void mutex_release_post(void *wrapcxt, OUT void *user_data) {
	dr_fprintf(STDERR, "<< post mutex release\n");
}

void wrap_mutex_acquire(const module_data_t *mod) {
	for (const auto & name : acquire_symbols) {
		app_pc towrap = (app_pc) dr_get_proc_address(mod->handle, name.c_str());
		if (towrap != NULL) {
			bool ok = drwrap_wrap(towrap, mutex_acquire_pre, mutex_acquire_post);
			if (ok) {
				std::string msg{ "<wrapped acq " };
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
				std::string msg{ "<wrapped rel " };
				msg += name + "\n";
				dr_fprintf(STDERR, msg.c_str());
			}
		}
	}
}
