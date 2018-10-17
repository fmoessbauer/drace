#pragma once

#include <dr_api.h>
#include <string>
#include <ipc/SMData.h>

#include "function-wrapper/internal.h"
#include "function-wrapper/event.h"

namespace funwrap {
	using wrapcb_pre_t = void(void *, void **);
	using wrapcb_post_t = void(void *, void *);

	struct wrap_info_t {
		const module_data_t * mod;
		wrapcb_pre_t        * pre;
		wrapcb_post_t       * post;
	};

	enum class Method : uint8_t {
		EXPORTS = 1,
		DBGSYMS = 2,
		EXTERNAL_MPCR = 3
	};

	bool init();
	void finalize();

	void wrap_functions(
		/*Module to inspect for symbols */
		const module_data_t *mod,
		/* Vector of symbol names or patterns */
		const std::vector<std::string> & syms,
		/* Perform a full search (set to true for managed symbols) */
		bool full_search,
		/* method to use for symbol lookup */
		Method method,
		/* Pre-function callback for each symbol found */
		wrapcb_pre_t pre,
		/* Post-function callback for each symbol found */
		wrapcb_post_t post);

	/* Wrap mutex aquire and release */
	void wrap_mutexes(const module_data_t *mod, bool sys);
	/* Wrap heap alloc and free */
	void wrap_allocations(const module_data_t *mod);
	/* Wrap excluded functions */
	void wrap_excludes(const module_data_t *mod, std::string section = "exclude");
	/* Wrap annotations */
	void wrap_annotations(const module_data_t *mod);
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
