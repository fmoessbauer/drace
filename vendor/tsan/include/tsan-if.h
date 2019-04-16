#pragma once

const int kSizeLog1 = 0;
const int kSizeLog2 = 1;
const int kSizeLog4 = 2;
const int kSizeLog8 = 3;


extern "C" {
	typedef struct {
		void* accessed_memory;
		int tid;
		unsigned long long user_id;
		int size;
		int write;
		int atomic;
		void* stack_trace;
		int stack_trace_size;
		int type;
        bool cli;
	} __tsan_race_info_access;

	typedef struct {
		__tsan_race_info_access* access1;
		__tsan_race_info_access* access2;
	} __tsan_race_info;

	void __tsan_init(void **thr, void **proc, void(*cb)(long, void*), void(*r_cb)(__tsan_race_info* raceInfo, void* callback_parameter), void* callback_parameter);
	void __tsan_init_simple(void(*r_cb)(__tsan_race_info* raceInfo, void* callback_parameter), void* callback_parameter);
	void __tsan_fini();

	void __tsan_map_shadow(void *addr, unsigned long size);
	void __tsan_go_start(void *thr, void **chthr, void *pc);
	void __tsan_go_end(void *thr);
	void __tsan_proc_create(void **pproc);
	void __tsan_proc_destroy(void *proc);
	void __tsan_proc_wire(void *proc, void *thr);
	void __tsan_proc_unwire(void *proc, void *thr);
	void __tsan_reset_range(unsigned int p, unsigned int sz);

	void __tsan_read(void *thr, void *addr, void *pc);
	void __tsan_write(void *thr, void *addr, void *pc);

	void __tsan_happens_before(void* thr, void* addr);
	void __tsan_happens_after(void* thr, void* addr);

	void __tsan_func_enter(void *thr, void *pc);
	void __tsan_func_exit(void *thr);
	void __tsan_free(void *p, unsigned long sz);

	void __tsan_acquire(void *thr, void *addr);
	void __tsan_release(void *thr, void *addr);

	void __tsan_mutex_after_lock(void *thr, void *addr, void* write);
	void __tsan_mutex_before_unlock(void *thr, void *addr, void* write);

	void __tsan_release_merge(void *thr, void *addr);

	void __tsan_end_use_user_tid(unsigned long thr);

	// C wrapper around native tsan interface
	void* __tsan_create_thread(unsigned long user_tid);
	void __tsan_malloc(void *thr, void *pc, void *p, unsigned long sz);
	void __tsan_ThreadFinish(void *thr);
	void __tsan_ThreadDetach(void *thr, uint64_t pc, int tid);
	void __tsan_ThreadJoin(void* thr, uint64_t pc, int tid);
}
