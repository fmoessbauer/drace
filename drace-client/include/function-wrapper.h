#pragma once

#include <dr_api.h>

namespace funwrap {
	void wrap_mutex_acquire(const module_data_t *mod);
	void wrap_mutex_release(const module_data_t *mod);
	void wrap_allocators(const module_data_t *mod);
}
