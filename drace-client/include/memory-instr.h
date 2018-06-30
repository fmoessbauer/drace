#pragma once

#include <dr_api.h>
#include <drmgr.h>
#include <drreg.h>
#include <drutil.h>
#include <dr_tools.h>

#include <atomic>

#define TLS_SLOT(tls_base) (void **)((byte *)(tls_base)+tls_offs)
#define BUF_PTR(tls_base) *(mem_ref_t **)TLS_SLOT(tls_base)

namespace memory_inst {

	/* Single memory reference */
	typedef struct _mem_ref_t {
		bool  write;
		void *addr;
		size_t size;
		app_pc pc;
	} mem_ref_t;

	/* Maximum number of references between clean calls */
	constexpr int MAX_NUM_MEM_REFS = 256;
	constexpr int MEM_BUF_SIZE = sizeof(mem_ref_t) * MAX_NUM_MEM_REFS;

	static size_t page_size;
	static app_pc code_cache;

	static std::atomic<int> refs;

	bool register_events();

	void init();

	void finalize();

	void process_buffer(void);

	void clear_buffer(void);

	static void analyze_access(void* drcontext);

	static void code_cache_init(void);

	static void code_cache_exit(void);

	// Instrumentation
	/* Instrument all memory accessing instructions */
	static void instrument_mem(void *drcontext, instrlist_t *ilist, instr_t *where, opnd_t ref, bool write);

	// Events
	static void event_thread_init(void *drcontext);

	static void event_thread_exit(void *drcontext);

	/* We transform string loops into regular loops so we can more easily
	* monitor every memory reference they make.
	*/
	static dr_emit_flags_t event_bb_app2app(void *drcontext, void *tag, instrlist_t *bb, bool for_trace, bool translating);

	static dr_emit_flags_t event_app_analysis(void * drcontext, void * tag, instrlist_t * bb, bool for_trace, bool translating, OUT void ** user_data);
	/* Instrument application instructions */
	static dr_emit_flags_t event_app_instruction(void *drcontext, void *tag, instrlist_t *bb,
		instr_t *instr, bool for_trace,
		bool translating, void *user_data);
}
