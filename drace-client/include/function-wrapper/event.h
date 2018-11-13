#pragma once

#include <dr_api.h>

namespace drace {
	struct per_thread_t;

	namespace funwrap {
		class event {
			/* Arguments of a WaitForMultipleObjects call */
			struct wfmo_args_t {
				DWORD          ncount;
				BOOL           waitall;
				const HANDLE*  handles;
			};

		private:
			static void prepare_and_aquire(
				void* wrapctx,
				void* mutex,
				bool write,
				bool trylock);

			static inline void prepare_and_release(void* wrapctx, bool write);

		public:

			static void beg_excl_region(per_thread_t * data);
			static void end_excl_region(per_thread_t * data);

			static void alloc_pre(void *wrapctx, void **user_data);
			static void alloc_post(void *wrapctx, void *user_data);

			static void realloc_pre(void *wrapctx, void **user_data);
			// realloc_post = alloc_post

			static void free_pre(void *wrapctx, void **user_data);
			static void free_post(void *wrapctx, void *user_data);

			static void thread_creation(void *wrapctx, void **user_data);
			static void thread_handover(void *wrapctx, void *user_data);

			static void thread_pre_sys(void *wrapctx, void **user_data);
			static void thread_post_sys(void *wrapctx, void *user_data);

			static void begin_excl(void *wrapctx, void **user_data);
			static void end_excl(void *wrapctx, void *user_data);

			static void dotnet_enter(void *wrapctx, void **user_data);
			static void dotnet_leave(void *wrapctx, void *user_data);

			// ------------
			// Mutex Events
			// ------------

			/** Get addr of mutex */
			static void get_arg(void *wrapctx, OUT void **user_data);
			static void get_arg_dotnet(void *wrapctx, OUT void **user_data);

			static void mutex_lock(void *wrapctx, void *user_data);
			static void mutex_trylock(void *wrapctx, void *user_data);
			static void mutex_unlock(void *wrapctx, OUT void **user_data);

			static void recmutex_lock(void *wrapctx, void *user_data);
			static void recmutex_trylock(void *wrapctx, void *user_data);
			static void recmutex_unlock(void *wrapctx, OUT void **user_data);

			static void mutex_read_lock(void *wrapctx, void *user_data);
			static void mutex_read_trylock(void *wrapctx, void *user_data);
			static void mutex_read_unlock(void *wrapctx, OUT void **user_data);

#ifdef WINDOWS
			/** WaitForSingleObject Windows API call (experimental)
			* \warning the mutex parameter is the Handle ID of the mutex and not
			*          a memory location
			* \warning the return value is a DWORD (aka 32bit unsigned integer)
			* \warning A handle does not have to be a mutex,
			*          however distinction is not possible here
			*/
			static void wait_for_single_obj(void *wrapctx, void *mutex);

			static void wait_for_mo_getargs(void *wrapctx, OUT void **user_data);
			/** WaitForMultipleObjects Windows API call (experimental) */
			static void wait_for_mult_obj(void *wrapctx, void *user_data);
#endif
			/** Call this function before a thread-barrier is entered */
			static void barrier_enter(void *wrapctx, void **user_data);
			/** Call this function after a thread-barrier is passed */
			static void barrier_leave(void *wrapctx, void *user_data);
			/** Call this function after a thread-barrier is passed or the waiting was cancelled */
			static void barrier_leave_or_cancel(void *wrapctx, void *user_data);

			/** Custom annotated happens before */
			static void happens_before(void *wrapctx, void *identifier);
			/** Custom annotated happens after */
			static void happens_after(void *wrapctx, void *identifier);
		};
	}
} // namespace drace