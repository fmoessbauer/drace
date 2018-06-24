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

	enum {
		REF_TYPE_READ  = 0,
		REF_TYPE_WRITE = 1,
	};

	typedef struct _mem_ref_t {
		bool  write;
		void *addr;
		size_t size;
		app_pc pc;
	} mem_ref_t;

	constexpr int MAX_NUM_MEM_REFS = 4096;
	constexpr int MEM_BUF_SIZE = sizeof(mem_ref_t) * MAX_NUM_MEM_REFS;

	/* thread private log file and counter */
	typedef struct {
		byte       *buf_ptr;
		byte       *buf_base;
		ptr_int_t   buf_end;
		void       *cache;
		uint64      last_alloc_size;
		uint64      num_refs;
		thread_id_t tid;
		ushort      test;
	} per_thread_t;

	static size_t page_size;
	static app_pc code_cache;

	static std::atomic<int> refs;

	bool register_events();

	void register_tls();

	void finalize();

	static void analyze_access(void* drcontext);

	void clean_call(void);

	static void code_cache_init(void);

	static void code_cache_exit(void);

	// Events
	static void event_thread_init(void *drcontext);

	static void event_thread_exit(void *drcontext);

	static void instrument_instr(void *drcontext, instrlist_t *ilist, instr_t *where);

	static void instrument_mem(void *drcontext, instrlist_t *ilist, instr_t *where, opnd_t ref, bool write);

	//static void insert_tag_lock(void *drcontext, instrlist_t *ilist, instr_t *where, reg_id_t reg_ptr, bool locked);

	static void insert_save_pc(void *drcontext, instrlist_t *ilist, instr_t *where,
		reg_id_t base, reg_id_t scratch, app_pc pc);
	
	static void insert_save_addr(void *drcontext, instrlist_t *ilist, instr_t *where,
		opnd_t ref, reg_id_t reg_ptr, reg_id_t reg_addr);

	static void insert_load_buf_ptr(void *drcontext, instrlist_t *ilist, instr_t *where,
		reg_id_t reg_ptr);

	static void insert_update_buf_ptr(void *drcontext, instrlist_t *ilist, instr_t *where,
		reg_id_t reg_ptr, int adjust);

	static void insert_save_type(void *drcontext, instrlist_t *ilist, instr_t *where,
		reg_id_t base, reg_id_t scratch, ushort type);

	static void insert_save_size(void *drcontext, instrlist_t *ilist, instr_t *where,
		reg_id_t base, reg_id_t scratch, ushort size);

	/* We transform string loops into regular loops so we can more easily
	* monitor every memory reference they make.
	*/
	static dr_emit_flags_t event_bb_app2app(void *drcontext, void *tag, instrlist_t *bb, bool for_trace, bool translating);
	static dr_emit_flags_t event_app_instruction(void *drcontext, void *tag, instrlist_t *bb,
		instr_t *instr, bool for_trace,
		bool translating, void *user_data);
}
