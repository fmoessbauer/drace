#pragma once
/*
 * DRace, a dynamic data race detector
 *
 * Copyright 2018 Siemens AG
 *
 * Authors:
 *   Felix Moessbauer <felix.moessbauer@siemens.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <dr_api.h>
#include <string>
#include <ipc/SMData.h>

#include "function-wrapper/internal.h"
#include "function-wrapper/event.h"

namespace drace {
	/// application function wrapping
	namespace funwrap {

        /// type of the pre-call callback
		using wrapcb_pre_t = void(void *, void **);

        /// type of the post-call callback
		using wrapcb_post_t = void(void *, void *);

        /// information that is passed to a function wrapper
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

		bool wrap_functions(
			/** Module to inspect for symbols */
			const module_data_t *mod,
			/** Vector of symbol names or patterns */
			const std::vector<std::string> & syms,
			/** Perform a full search (set to true for managed symbols) */
			bool full_search,
			/** method to use for symbol lookup */
			Method method,
			/** Pre-function callback for each symbol found */
			wrapcb_pre_t pre,
			/** Post-function callback for each symbol found */
			wrapcb_post_t post);

		/** Wrap mutex aquire and release */
		void wrap_mutexes(const module_data_t *mod, bool sys);
		/** Wrap heap alloc and free */
		void wrap_allocations(const module_data_t *mod);
		/** Wrap excluded functions */
		void wrap_excludes(const module_data_t *mod, std::string section = "functions");
		/** Wrap annotations */
		void wrap_annotations(const module_data_t *mod);
		/** Wrap C++11 thread starters */
		void wrap_thread_start(const module_data_t *mod);
		/** Wrap System thread starters */
		void wrap_thread_start_sys(const module_data_t *mod);

		template<typename InputIt>
		void wrap_dotnet(InputIt a, InputIt b) {
			std::for_each(a, b, [](uintptr_t addr) {
				internal::wrap_dotnet_helper(addr);
			});
		}
		/** Wraps dotnet sync primitives */
		void wrap_sync_dotnet(const module_data_t *mod, bool native);

        /**
         * \brief Manually instrument a call / ret construct
         * \note: mainly for the shadow stack, but also for some fast
         *        switching of the instrumentation logic
         */
        bool wrap_generic_call(void *drcontext, void *tag, instrlist_t *bb,
			instr_t *instr, bool for_trace,
			bool translating, void *user_data);
	}
}
