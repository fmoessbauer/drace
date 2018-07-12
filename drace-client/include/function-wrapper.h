#pragma once

#include <dr_api.h>

namespace funwrap {
	bool init();
	void finalize();

	/* Wrap mutex aquire and release */
	void wrap_mutexes(const module_data_t *mod);
	/* Wrap heap alloc and free */
	void wrap_allocations(const module_data_t *mod);
	/* Wrap excluded functions */
	void wrap_excludes(const module_data_t *mod);
	/* Wrap C++11 thread starters */
	void wrap_thread_start(const module_data_t *mod);
	/* Wrap System thread starters */
	void wrap_thread_start_sys(const module_data_t *mod);
}
