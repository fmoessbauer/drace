#pragma once

#include "function-wrapper.h"

#include <vector>
#include <string>

namespace funwrap {
	// forward declare to avoid circular dependency
	using wrapcb_pre_t = void(void *, void **);
	using wrapcb_post_t = void(void *, void *);

	namespace internal {
		void wrap_dotnet_helper(uint64_t addr);
		bool wrap_function_clbck(const char *name, size_t modoffs, void *data);

		bool mutex_wrap_callback(const char *name, size_t modoffs, void *data);
	} // namespace internal
} // namespace funwrap
