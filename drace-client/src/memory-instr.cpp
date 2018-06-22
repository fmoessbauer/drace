#pragma once

#include <dr_api.h>
#include <drmgr.h>
#include <drreg.h>
#include <drutil.h>
#include <dr_tools.h>

#include <detector_if.h>

#include "drace-client.h"
#include "memory-instr.h"

bool memory_inst::register_events() {
	return (
		drmgr_register_thread_init_event(memory_inst::event_thread_init) &&
		drmgr_register_thread_exit_event(memory_inst::event_thread_exit) &&
		drmgr_register_bb_app2app_event(event_bb_app2app, NULL)          &&
		drmgr_register_bb_instrumentation_event(NULL, event_app_instruction, NULL)
		);
}

void memory_inst::register_tls() {
	tls_idx = drmgr_register_tls_field();
	DR_ASSERT(tls_idx != -1);
	/* The TLS field provided by DR cannot be directly accessed from the code cache.
	* For better performance, we allocate raw TLS so that we can directly
	* access and update it with a single instruction.
	*/
	if (!dr_raw_tls_calloc(&tls_seg, &tls_offs, MEMTRACE_TLS_COUNT, 0))
		DR_ASSERT(false);
	dr_printf("< Registered TLS\n");
}

void memory_inst::finalize() {
	if (!drmgr_unregister_thread_init_event(memory_inst::event_thread_init) ||
		!drmgr_unregister_thread_exit_event(memory_inst::event_thread_exit) ||
		!drmgr_unregister_bb_app2app_event(memory_inst::event_bb_app2app) ||
		!drmgr_unregister_bb_insertion_event(memory_inst::event_app_instruction) ||
		drreg_exit() != DRREG_SUCCESS)
		DR_ASSERT(false);
}

static inline void memory_inst::process_buffer() {
	void *drcontext = dr_get_current_drcontext();
	memory_inst::analyze_access(drcontext);
}

static void memory_inst::analyze_access(void *drcontext) {
	per_thread_t *data;
	mem_ref_t *mem_ref, *buf_ptr;

	data = (per_thread_t*) drmgr_get_tls_field(drcontext, tls_idx);
	buf_ptr = BUF_PTR(data->seg_base);

	for (mem_ref = (mem_ref_t *)data->buf_base; mem_ref < buf_ptr; mem_ref++) {
		// Not necessary as only R/W is instrumented
		//if (mem_ref->type > REF_TYPE_WRITE) {
		//	data->lastop = mem_ref->type;
		//	data->locked = mem_ref->locked;
		//}

		if (data->locked == 0) {
			// skip if last-op was compare-exchange
			if (mem_ref->type == REF_TYPE_WRITE) {
				detector::write(data->tid, mem_ref->addr, mem_ref->size);
				//printf("[%i] WRITE %p, LAST: %s\n", data->tid, (ptr_uint_t)mem_ref->addr, decode_opcode_name(data->lastop));
			}
			else if (mem_ref->type == REF_TYPE_READ) {
				detector::read(data->tid, mem_ref->addr, mem_ref->size);
				//printf("[%i] READ  %p, LAST: %s\n", data->tid, (ptr_uint_t)mem_ref->addr, decode_opcode_name(data->lastop));
			}
		}
		data->num_refs++;
	}
	BUF_PTR(data->seg_base) = data->buf_base;
}

// Events
static void memory_inst::event_thread_init(void *drcontext)
{
	per_thread_t *data = (per_thread_t*) dr_thread_alloc(drcontext, sizeof(per_thread_t));
	DR_ASSERT(data != NULL);
	drmgr_set_tls_field(drcontext, tls_idx, data);

	/* Keep seg_base in a per-thread data structure so we can get the TLS
	 * slot and find where the pointer points to in the buffer.
	 */
	data->seg_base = (byte*) dr_get_dr_segment_base(tls_seg);
	data->buf_base = (mem_ref_t*) dr_raw_mem_alloc(MEM_BUF_SIZE,
		DR_MEMPROT_READ | DR_MEMPROT_WRITE,
		NULL);
	DR_ASSERT(data->seg_base != NULL && data->buf_base != NULL);
	/* put buf_base to TLS as starting buf_ptr */
	BUF_PTR(data->seg_base) = data->buf_base;

	data->tid = dr_get_thread_id(drcontext);
}

static void memory_inst::event_thread_exit(void *drcontext)
{
	memory_inst::analyze_access(drcontext);

	per_thread_t * data = (per_thread_t *)drmgr_get_tls_field(drcontext, tls_idx);
	memory_inst::refs += data->num_refs;

	dr_raw_mem_free(data->buf_base, MEM_BUF_SIZE);
	dr_thread_free(drcontext, data, sizeof(per_thread_t));

	dr_printf("< memory refs: %i\n", memory_inst::refs.load());
}


/* We transform string loops into regular loops so we can more easily
* monitor every memory reference they make.
*/
static dr_emit_flags_t memory_inst::event_bb_app2app(void *drcontext, void *tag, instrlist_t *bb, bool for_trace, bool translating)
{
	if (!drutil_expand_rep_string(drcontext, bb)) {
		DR_ASSERT(false);
		/* in release build, carry on: we'll just miss per-iter refs */
	}
	return DR_EMIT_DEFAULT;
}

/**
 * load current buffer ptr into register
 */
static void memory_inst::insert_load_buf_ptr(void *drcontext, instrlist_t *ilist, instr_t *where,
	reg_id_t reg_ptr)
{
	dr_insert_read_raw_tls(drcontext, ilist, where, tls_seg,
		tls_offs + MEMTRACE_TLS_OFFS_BUF_PTR, reg_ptr);
}

static void memory_inst::insert_save_pc(void *drcontext, instrlist_t *ilist, instr_t *where,
	reg_id_t base, reg_id_t scratch, app_pc pc)
{
	instrlist_insert_mov_immed_ptrsz(drcontext, (ptr_int_t)pc,
		opnd_create_reg(scratch),
		ilist, where, NULL, NULL);
	instrlist_meta_preinsert(ilist, where,
		XINST_CREATE_store(drcontext,
			OPND_CREATE_MEMPTR(base,
				offsetof(mem_ref_t, addr)),
			opnd_create_reg(scratch)));
}

static void memory_inst::insert_save_addr(void *drcontext, instrlist_t *ilist, instr_t *where,
	opnd_t ref, reg_id_t reg_ptr, reg_id_t reg_addr)
{
	/* we use reg_ptr as scratch to get addr */
	bool ok = drutil_insert_get_mem_addr(drcontext, ilist, where, ref, reg_addr, reg_ptr);
	DR_ASSERT(ok);
	insert_load_buf_ptr(drcontext, ilist, where, reg_ptr);
	instrlist_meta_preinsert(ilist, where,
		XINST_CREATE_store(drcontext,
			OPND_CREATE_MEMPTR(reg_ptr,
				offsetof(mem_ref_t, addr)),
			opnd_create_reg(reg_addr)));
}

static void memory_inst::insert_tag_lock(void *drcontext, instrlist_t *ilist, instr_t *where, reg_id_t reg_ptr, bool locked)
{
	instrlist_meta_preinsert(ilist, where,
		XINST_CREATE_store_2bytes(drcontext,
			OPND_CREATE_MEM16(reg_ptr,
				offsetof(mem_ref_t, locked)),
			OPND_CREATE_INT16((ushort) locked)));
}

/**
 * increment buffer pointer (TODO: Check if necessary)
 */
static void memory_inst::insert_update_buf_ptr(void *drcontext, instrlist_t *ilist, instr_t *where,
	reg_id_t reg_ptr, int adjust)
{
	instrlist_meta_preinsert(ilist, where,
		XINST_CREATE_add(drcontext,
			opnd_create_reg(reg_ptr),
			OPND_CREATE_INT16(adjust)));
	dr_insert_write_raw_tls(drcontext, ilist, where, tls_seg,
		tls_offs + MEMTRACE_TLS_OFFS_BUF_PTR, reg_ptr);
}

static void memory_inst::insert_save_type(void *drcontext, instrlist_t *ilist, instr_t *where,
	reg_id_t base, reg_id_t scratch, ushort type)
{
	scratch = reg_resize_to_opsz(scratch, OPSZ_2);
	instrlist_meta_preinsert(ilist, where,
		XINST_CREATE_load_int(drcontext,
			opnd_create_reg(scratch),
			OPND_CREATE_INT16(type)));
	instrlist_meta_preinsert(ilist, where,
		XINST_CREATE_store_2bytes(drcontext,
			OPND_CREATE_MEM16(base,
				offsetof(mem_ref_t, type)),
			opnd_create_reg(scratch)));
}

static void memory_inst::insert_save_size(void *drcontext, instrlist_t *ilist, instr_t *where,
	reg_id_t base, reg_id_t scratch, ushort size)
{
	scratch = reg_resize_to_opsz(scratch, OPSZ_2);
	instrlist_meta_preinsert(ilist, where,
		XINST_CREATE_load_int(drcontext,
			opnd_create_reg(scratch),
			OPND_CREATE_INT16(size)));
	instrlist_meta_preinsert(ilist, where,
		XINST_CREATE_store_2bytes(drcontext,
			OPND_CREATE_MEM16(base,
				offsetof(mem_ref_t, size)),
			opnd_create_reg(scratch)));
}

/* insert inline code to add an instruction entry into the buffer */
static void memory_inst::instrument_instr(void *drcontext, instrlist_t *ilist, instr_t *where)
{
	/* We need two scratch registers */
	reg_id_t reg_ptr, reg_tmp;

	if (drreg_reserve_register(drcontext, ilist, where, NULL, &reg_ptr) !=
		DRREG_SUCCESS ||
		drreg_reserve_register(drcontext, ilist, where, NULL, &reg_tmp) !=
		DRREG_SUCCESS) {
		DR_ASSERT(false); /* cannot recover */
		return;
	}
	// Check if instr has LOCK prefix
	bool locked = instr_get_prefix_flag(where, PREFIX_LOCK);

	insert_load_buf_ptr(drcontext, ilist, where, reg_ptr);

	insert_tag_lock(drcontext, ilist, where, reg_ptr, locked);

	insert_save_type(drcontext, ilist, where, reg_ptr, reg_tmp,
		(ushort)instr_get_opcode(where));
	insert_save_size(drcontext, ilist, where, reg_ptr, reg_tmp,
		(ushort)instr_length(drcontext, where));
	insert_save_pc(drcontext, ilist, where, reg_ptr, reg_tmp,
		instr_get_app_pc(where));
	insert_update_buf_ptr(drcontext, ilist, where, reg_ptr, sizeof(mem_ref_t));
	/* Restore scratch registers */
	if (drreg_unreserve_register(drcontext, ilist, where, reg_ptr) != DRREG_SUCCESS ||
		drreg_unreserve_register(drcontext, ilist, where, reg_tmp) != DRREG_SUCCESS)
		DR_ASSERT(false);
}

/* insert inline code to add a memory reference info entry into the buffer */
static void memory_inst::instrument_mem(void *drcontext, instrlist_t *ilist, instr_t *where,
	opnd_t ref, bool write)
{
	/* We need two scratch registers */
	reg_id_t reg_ptr, reg_tmp;
	if (drreg_reserve_register(drcontext, ilist, where, NULL, &reg_ptr) !=
		DRREG_SUCCESS ||
		drreg_reserve_register(drcontext, ilist, where, NULL, &reg_tmp) !=
		DRREG_SUCCESS) {
		DR_ASSERT(false);
		return;
	}

	/* save_addr should be called first as reg_ptr or reg_tmp maybe used in ref */
	memory_inst::insert_save_addr(drcontext, ilist, where, ref, reg_ptr, reg_tmp);
	memory_inst::insert_save_type(drcontext, ilist, where, reg_ptr, reg_tmp,
		write ? REF_TYPE_WRITE : REF_TYPE_READ);
	memory_inst::insert_save_size(drcontext, ilist, where, reg_ptr, reg_tmp,
		(ushort)drutil_opnd_mem_size_in_bytes(ref, where));
	memory_inst::insert_update_buf_ptr(drcontext, ilist, where, reg_ptr, sizeof(mem_ref_t));

	/* Restore scratch registers */
	if (drreg_unreserve_register(drcontext, ilist, where, reg_ptr) != DRREG_SUCCESS ||
		drreg_unreserve_register(drcontext, ilist, where, reg_tmp) != DRREG_SUCCESS)
		DR_ASSERT(false);
}


static dr_emit_flags_t memory_inst::event_app_instruction(void *drcontext, void *tag, instrlist_t *bb,
	instr_t *instr, bool for_trace,
	bool translating, void *user_data) {

	if (!instr_is_app(instr))
		return DR_EMIT_DEFAULT;
	// Only track moves and ignore Locked instructions
	// TODO: Check if sufficient
	if (!instr_is_mov(instr) || instr_get_prefix_flag(instr, PREFIX_LOCK))
		return DR_EMIT_DEFAULT;
	if (!instr_reads_memory(instr) && !instr_writes_memory(instr))
		return DR_EMIT_DEFAULT;

	// Filter certain opcodes
	//ushort opcode = instr_get_opcode(instr);
	//if (opcode == OP_cmpxchg ||
	//	opcode == OP_push ||
	//	opcode == OP_pop ||
	//	opcode == OP_ret ||
	//	opcode == OP_call ||
	//	opcode == OP_stos)

	/* insert code to add an entry for app instruction */
	// TODO: Check if instruction instrumentation is necessary (probably not)
	//instrument_instr(drcontext, bb, instr);
	
	/* insert code to add an entry for each memory reference opnd */
	for (int i = 0; i < instr_num_srcs(instr); i++) {
		opnd_t src = instr_get_src(instr, i);
		if (opnd_is_memory_reference(src))
			instrument_mem(drcontext, bb, instr, src, false);
	}

	for (int i = 0; i < instr_num_dsts(instr); i++) {
		opnd_t dst = instr_get_dst(instr, i);
		if (opnd_is_memory_reference(dst))
			instrument_mem(drcontext, bb, instr, dst, true);
	}

	// TODO: Try to do less clean calls as extremely expensive
	// TODO: Currently this approach is probabilistic as buffer size is never checked
	// Segfaults on buffer overflow
	dr_insert_clean_call(drcontext, bb, instr, (void *)process_buffer, false, 0);

	return DR_EMIT_DEFAULT;
}
