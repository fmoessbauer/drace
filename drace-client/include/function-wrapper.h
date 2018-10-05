#pragma once

#include <dr_api.h>
#include <string>
#include <ipc/SMData.h>

namespace funwrap {
	namespace internal {
		void wrap_dotnet_helper(uint64_t addr);

		static void beg_excl_region(per_thread_t * data);
		static void end_excl_region(per_thread_t * data);

		static void alloc_pre(void *wrapctx, void **user_data);
		static void alloc_post(void *wrapctx, void *user_data);

		static void free_pre(void *wrapctx, void **user_data);
		static void free_post(void *wrapctx, void *user_data);

		static void thread_creation(void *wrapctx, void **user_data);
		static void thread_handover(void *wrapctx, void *user_data);
		static void thread_pre_sys(void *wrapctx, void **user_data);
		static void thread_post_sys(void *wrapctx, void *user_data);

		void begin_excl(void *wrapctx, void **user_data);
		void end_excl(void *wrapctx, void *user_data);

		static void dotnet_enter(void *wrapctx, void **user_data);
		static void dotnet_leave(void *wrapctx, void *user_data);
	}

	bool init();
	void finalize();

	/* Wrap mutex aquire and release */
	void wrap_mutexes(const module_data_t *mod, bool sys);
	/* Wrap heap alloc and free */
	void wrap_allocations(const module_data_t *mod);
	/* Wrap excluded functions */
	void wrap_excludes(const module_data_t *mod, std::string section = "exclude");
	/* Wrap C++11 thread starters */
	void wrap_thread_start(const module_data_t *mod);
	/* Wrap System thread starters */
	void wrap_thread_start_sys(const module_data_t *mod);

	template<typename InputIt>
	void wrap_dotnet(InputIt a, InputIt b) {
		std::for_each(a, b, [](uint64_t addr) {
			internal::wrap_dotnet_helper(addr);
		});
	}
	/* Wraps dotnet sync primitives */
	void wrap_sync_dotnet(const module_data_t *mod, bool native);
}
