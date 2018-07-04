#pragma once

#include <dr_api.h>
#include <drmgr.h>
#include <drreg.h>
#include <drutil.h>
#include <dr_tools.h>

#include <detector_if.h>

#include "drace-client.h"
#include "memory-instr.h"

/* Number of instrumented calls, used for sampling */
static std::atomic<int> instrum_count{ 0 };

bool memory_inst::register_events() {
	return (
		drmgr_register_thread_init_event(memory_inst::event_thread_init) &&
		drmgr_register_thread_exit_event(memory_inst::event_thread_exit) &&
		drmgr_register_bb_app2app_event(event_bb_app2app, NULL) &&
		drmgr_register_bb_instrumentation_event(event_app_analysis, event_app_instruction, NULL)
		);
}

void memory_inst::init() {
	/* We need 2 reg slots beyond drreg's eflags slots => 3 slots */
	drreg_options_t ops = { sizeof(ops), 3, false };

	DR_ASSERT(drreg_init(&ops) == DRREG_SUCCESS);
	page_size = dr_page_size();

	tls_idx = drmgr_register_tls_field();
	DR_ASSERT(tls_idx != -1);

	code_cache_init();
	dr_printf("< Initialized\n");
}

void memory_inst::finalize() {
	code_cache_exit();

	if (!drmgr_unregister_thread_init_event(event_thread_init) ||
		!drmgr_unregister_thread_exit_event(event_thread_exit) ||
		!drmgr_unregister_bb_app2app_event(event_bb_app2app) ||
		!drmgr_unregister_bb_instrumentation_event(event_app_analysis) ||
		drreg_exit() != DRREG_SUCCESS)
		DR_ASSERT(false);
}

static void memory_inst::analyze_access(void *drcontext) {
	per_thread_t *data;
	mem_ref_t *mem_ref;

	data = (per_thread_t*)drmgr_get_tls_field(drcontext, tls_idx);
	mem_ref = (mem_ref_t *)data->buf_base;
	int num_refs = (int)((mem_ref_t *)data->buf_ptr - mem_ref);

	if (!data->disabled && data->grace_period < data->num_refs) {
		for (int i = 0; i < num_refs; ++i) {
			if (mem_ref->write) {
				detector::write(data->tid, mem_ref->pc, mem_ref->addr, mem_ref->size);
				//printf("[%i] WRITE %p, PC: %p\n", data->tid, mem_ref->addr, mem_ref->pc);
			}
			else {
				detector::read(data->tid, mem_ref->pc, mem_ref->addr, mem_ref->size);
				//printf("[%i] READ  %p, PC: %p\n", data->tid, mem_ref->addr, mem_ref->pc);
			}
			++mem_ref;
		}
	}
	memset(data->buf_base, 0, MEM_BUF_SIZE);
	data->num_refs += num_refs;
	data->buf_ptr = data->buf_base;
}

// Events
static void memory_inst::event_thread_init(void *drcontext)
{
	/* allocate thread private data */
	per_thread_t* data = (per_thread_t *) dr_thread_alloc(drcontext, sizeof(per_thread_t));
	drmgr_set_tls_field(drcontext, tls_idx, data);

	data->buf_base = (byte*) dr_thread_alloc(drcontext, MEM_BUF_SIZE);
	data->buf_ptr = data->buf_base;
	/* set buf_end to be negative of address of buffer end for the lea later */
	data->buf_end = -(ptr_int_t)(data->buf_base + MEM_BUF_SIZE);
	data->num_refs = 0;
	data->tid = dr_get_thread_id(drcontext);
	// avoid races during thread startup
	data->grace_period = data->num_refs + 1'000;

	// this is the master thread
	if (params.exclude_master && data->tid == runtime_tid) {
		data->disabled = true;
		data->event_stack.push("th-master");
	}

	// TODO: Possible speed up by checking if thread is started by non-instrumented
	// Lib
}

static void memory_inst::event_thread_exit(void *drcontext)
{
	// process remaining accesses
	memory_inst::analyze_access(drcontext);

	per_thread_t* data = (per_thread_t*) drmgr_get_tls_field(drcontext, tls_idx);
	
	refs += data->num_refs; // atomic access

	dr_thread_free(drcontext, data->buf_base, MEM_BUF_SIZE);
	dr_thread_free(drcontext, data, sizeof(per_thread_t));

	dr_printf("< [%.5i] memory refs: %i\n", data->tid, memory_inst::refs.load());
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

static dr_emit_flags_t memory_inst::event_app_analysis(void *drcontext, void *tag, instrlist_t *bb,
	bool for_trace, bool translating, OUT void **user_data) {
	bool instrument_bb = true;
	if (params.frequent_only && !for_trace) {
		// only instrument traces, much faster startup
		instrument_bb = false;
	}
	else {
		app_pc bb_addr = dr_fragment_app_pc(tag);

		// Create dummy shadow module
		module_tracker::module_info_t bb_mod(bb_addr, nullptr);

		auto bb_mod_it = modules.lower_bound(bb_mod);
		if ((bb_mod_it != modules.end()) && (bb_addr < bb_mod_it->end)) {
			instrument_bb = bb_mod_it->instrument;
			// bb in known module
		}
		else {
			// Module not known
			//dr_printf("< Unknown MOD at %p\n", bb_addr);
			instrument_bb = false;
		}
	}

	*(bool *)user_data = instrument_bb;
	return DR_EMIT_DEFAULT;
}

static dr_emit_flags_t memory_inst::event_app_instruction(void *drcontext, void *tag, instrlist_t *bb,
	instr_t *instr, bool for_trace,
	bool translating, void *user_data) {
	bool instrument_instr = (bool)(ptr_uint_t)user_data;
	if (!instrument_instr)
		return DR_EMIT_DEFAULT;

	if (!instr_is_app(instr))
		return DR_EMIT_DEFAULT;

	// Ignore Locked instructions
	if (instr_get_prefix_flag(instr, PREFIX_LOCK))
		return DR_EMIT_DEFAULT;
	if (!instr_reads_memory(instr) && !instr_writes_memory(instr))
		return DR_EMIT_DEFAULT;

	// Sampling: Only instrument some instructions
	// TODO: Improve this by using per-type counters
	++instrum_count;
	if (instrum_count % params.sampling_rate != 0) {
		return DR_EMIT_DEFAULT;
	}

	// Check if actually in instrumented module:
	// Ideally this should not be necessary as analysis function catches all
	app_pc bb_addr = dr_fragment_app_pc(tag);
	// Create dummy shadow module
	module_tracker::module_info_t bb_mod(bb_addr, nullptr);

	auto bb_mod_it = modules.lower_bound(bb_mod);
	if ((bb_mod_it != modules.end()) && (bb_addr < bb_mod_it->end)) {
		if (!bb_mod_it->instrument) {
			//dr_printf("NO INSTR at %s\n",dr_module_preferred_name(bb_mod_it->info));
			return DR_EMIT_DEFAULT;
		}
		// bb in known module
	}
	else {
		// Do not instrument unknown modules
		return DR_EMIT_DEFAULT;
	}

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

	return DR_EMIT_DEFAULT;
}

/* clean_call dumps the memory reference info to the log file */
void memory_inst::process_buffer(void)
{
	void *drcontext = dr_get_current_drcontext();
	memory_inst::analyze_access(drcontext);
}

void memory_inst::clear_buffer(void)
{
	void *drcontext = dr_get_current_drcontext();
	per_thread_t *data;
	mem_ref_t *mem_ref;

	data = (per_thread_t*)drmgr_get_tls_field(drcontext, tls_idx);
	mem_ref = (mem_ref_t *)data->buf_base;
	int num_refs = (int)((mem_ref_t *)data->buf_ptr - mem_ref);

	//dr_printf(">> Clear Buffer, drop %i\n", num_refs);

	memset(data->buf_base, 0, MEM_BUF_SIZE);
	data->num_refs += num_refs;
	data->buf_ptr = data->buf_base;
}

static void memory_inst::code_cache_init(void)
{
	void         *drcontext;
	instrlist_t  *ilist;
	instr_t      *where;
	byte         *end;

	drcontext = dr_get_current_drcontext();
	code_cache = (app_pc) dr_nonheap_alloc(page_size,
		DR_MEMPROT_READ |
		DR_MEMPROT_WRITE |
		DR_MEMPROT_EXEC);
	ilist = instrlist_create(drcontext);
	/* The lean procecure simply performs a clean call, and then jump back */
	/* jump back to the DR's code cache */
	where = INSTR_CREATE_jmp_ind(drcontext, opnd_create_reg(DR_REG_XCX));
	instrlist_meta_append(ilist, where);
	/* clean call */
	dr_insert_clean_call(drcontext, ilist, where, (void *)process_buffer, false, 0);
	/* Encodes the instructions into memory and then cleans up. */
	end = instrlist_encode(drcontext, ilist, code_cache, false);
	DR_ASSERT((size_t)(end - code_cache) < page_size);
	instrlist_clear_and_destroy(drcontext, ilist);
	/* set the memory as just +rx now */
	dr_memory_protect(code_cache, page_size, DR_MEMPROT_READ | DR_MEMPROT_EXEC);
}


static void memory_inst::code_cache_exit(void)
{
	dr_nonheap_free(code_cache, page_size);
}

/* insert inline code to add a memory reference info entry into the buffer */
static void memory_inst::instrument_mem(void *drcontext, instrlist_t *ilist, instr_t *where,
	opnd_t ref, bool write)
{
	/*
	* instrument_mem is called whenever a memory reference is identified.
	* It inserts code before the memory reference to to fill the memory buffer
	* and jump to our own code cache to call the clean_call when the buffer is full.
	*/
	instr_t *instr, *call, *restore;
	opnd_t   opnd1, opnd2;
	reg_id_t reg1, reg2;
	drvector_t allowed;
	per_thread_t *data;
	app_pc pc;

	data = (per_thread_t*) drmgr_get_tls_field(drcontext, tls_idx);

	/* Steal two scratch registers.
	* reg2 must be ECX or RCX for jecxz.
	*/
	drreg_init_and_fill_vector(&allowed, false);
	drreg_set_vector_entry(&allowed, DR_REG_XCX, true);
	if (drreg_reserve_register(drcontext, ilist, where, &allowed, &reg2) !=
		DRREG_SUCCESS ||
		drreg_reserve_register(drcontext, ilist, where, NULL, &reg1) != DRREG_SUCCESS) {
		DR_ASSERT(false); /* cannot recover */
		drvector_delete(&allowed);
		return;
	}
	drvector_delete(&allowed);

	/* use drutil to get mem address */
	drutil_insert_get_mem_addr(drcontext, ilist, where, ref, reg1, reg2);

	/* The following assembly performs the following instructions
	* if (disabled)
	*   jmp .restore;
	* buf_ptr->write = write;
	* buf_ptr->addr  = addr;
	* buf_ptr->size  = size;
	* buf_ptr->pc    = pc;
	* buf_ptr++;
	* if (buf_ptr >= buf_end_ptr)
	*    clean_call();
	* .restore
	*/

	/* Precondition:
	   reg1: memory address of access
	   reg2: wiped / unknown state
	*/
	drmgr_insert_read_tls_field(drcontext, tls_idx, ilist, where, reg2);
	
	/* Jump if tracing is disabled */
	restore = INSTR_CREATE_label(drcontext);

	/* save reg1 containing memory address */
	opnd1 = opnd_create_reg(reg1);
	instr = INSTR_CREATE_push(drcontext, opnd1);
	instrlist_meta_preinsert(ilist, where, instr);

	/* we use lea + jecxz trick for better performance
	* lea and jecxz won't disturb the eflags, so we won't insert
	* code to save and restore application's eflags.
	*/
	/* lea [reg1 - disabled] => reg1 */
	opnd1 = opnd_create_reg(reg1);
	opnd2 = OPND_CREATE_MEMPTR(reg2, offsetof(per_thread_t, disabled));
	instr = INSTR_CREATE_mov_ld(drcontext, opnd1, opnd2);
	instrlist_meta_preinsert(ilist, where, instr);
	
	/* %reg1 + NULL*0 + (-1)
	 => lea evaluates to 0 if disabled == 1 */
	opnd2 = opnd_create_base_disp(reg1, NULL, 1, -1, OPSZ_lea);
	instr = INSTR_CREATE_lea(drcontext, opnd1, opnd2);
	instrlist_meta_preinsert(ilist, where, instr);

	/* Jump if (E|R)CX is 0 */
	restore = INSTR_CREATE_label(drcontext);
	opnd1 = opnd_create_instr(restore);
	instr = INSTR_CREATE_jecxz(drcontext, opnd1);
	instrlist_meta_preinsert(ilist, where, instr);

	/* Restore reg1 with memory addr */
	opnd1 = opnd_create_reg(reg1);
	instr = INSTR_CREATE_pop(drcontext, opnd1);
	instrlist_meta_preinsert(ilist, where, instr);

	/* Load data->buf_ptr into reg2 */
	opnd1 = opnd_create_reg(reg2);
	opnd2 = OPND_CREATE_MEMPTR(reg2, offsetof(per_thread_t, buf_ptr));
	instr = INSTR_CREATE_mov_ld(drcontext, opnd1, opnd2);
	instrlist_meta_preinsert(ilist, where, instr);

	/* Move write/read to write field */
	opnd1 = OPND_CREATE_MEM32(reg2, offsetof(mem_ref_t, write));
	opnd2 = OPND_CREATE_INT32(write);
	instr = INSTR_CREATE_mov_imm(drcontext, opnd1, opnd2);
	instrlist_meta_preinsert(ilist, where, instr);

	/* Store address in memory ref */
	opnd1 = OPND_CREATE_MEMPTR(reg2, offsetof(mem_ref_t, addr));
	opnd2 = opnd_create_reg(reg1);
	instr = INSTR_CREATE_mov_st(drcontext, opnd1, opnd2);
	instrlist_meta_preinsert(ilist, where, instr);

	/* Store size in memory ref */
	opnd1 = OPND_CREATE_MEMPTR(reg2, offsetof(mem_ref_t, size));
	/* drutil_opnd_mem_size_in_bytes handles OP_enter */
	opnd2 = OPND_CREATE_INT32(drutil_opnd_mem_size_in_bytes(ref, where));
	instr = INSTR_CREATE_mov_st(drcontext, opnd1, opnd2);
	instrlist_meta_preinsert(ilist, where, instr);

	///* Store pc in memory ref */
	pc = instr_get_app_pc(where);
	///* For 64-bit, we can't use a 64-bit immediate so we split pc into two halves.
	//* We could alternatively load it into reg1 and then store reg1.
	//* We use a convenience routine that does the two-step store for us.
	//*/
	opnd1 = OPND_CREATE_MEMPTR(reg2, offsetof(mem_ref_t, pc));
	instrlist_insert_mov_immed_ptrsz(drcontext, (ptr_int_t)pc, opnd1,
		ilist, where, NULL, NULL);

	/* Increment reg value by pointer size using lea instr */
	opnd1 = opnd_create_reg(reg2);
	opnd2 = opnd_create_base_disp(reg2, DR_REG_NULL, 0,
		sizeof(mem_ref_t),
		OPSZ_lea);
	instr = INSTR_CREATE_lea(drcontext, opnd1, opnd2);
	instrlist_meta_preinsert(ilist, where, instr);

	/* Update the data->buf_ptr */
	drmgr_insert_read_tls_field(drcontext, tls_idx, ilist, where, reg1);
	opnd1 = OPND_CREATE_MEMPTR(reg1, offsetof(per_thread_t, buf_ptr));
	opnd2 = opnd_create_reg(reg2);
	instr = INSTR_CREATE_mov_st(drcontext, opnd1, opnd2);
	instrlist_meta_preinsert(ilist, where, instr);

	/* we use lea + jecxz trick for better performance
	* lea and jecxz won't disturb the eflags, so we won't insert
	* code to save and restore application's eflags.
	*/
	/* lea [reg2 - buf_end] => reg2 */
	opnd1 = opnd_create_reg(reg1);
	opnd2 = OPND_CREATE_MEMPTR(reg1, offsetof(per_thread_t, buf_end));
	instr = INSTR_CREATE_mov_ld(drcontext, opnd1, opnd2);
	instrlist_meta_preinsert(ilist, where, instr);
	opnd1 = opnd_create_reg(reg2);
	opnd2 = opnd_create_base_disp(reg1, reg2, 1, 0, OPSZ_lea);
	instr = INSTR_CREATE_lea(drcontext, opnd1, opnd2);
	instrlist_meta_preinsert(ilist, where, instr);

	/* jecxz call */
	call = INSTR_CREATE_label(drcontext);
	opnd1 = opnd_create_instr(call);
	instr = INSTR_CREATE_jecxz(drcontext, opnd1);
	instrlist_meta_preinsert(ilist, where, instr);

	/* jump restore to skip clean call */
	opnd1 = opnd_create_instr(restore);
	instr = INSTR_CREATE_jmp(drcontext, opnd1);
	instrlist_meta_preinsert(ilist, where, instr);

	/* clean call */
	/* We jump to lean procedure which performs full context switch and
	* clean call invocation. This is to reduce the code cache size.
	*/
	instrlist_meta_preinsert(ilist, where, call);
	/* mov restore DR_REG_XCX */
	opnd1 = opnd_create_reg(reg2);
	/* this is the return address for jumping back from lean procedure */
	opnd2 = opnd_create_instr(restore);
	/* We could use instrlist_insert_mov_instr_addr(), but with a register
	* destination we know we can use a 64-bit immediate.
	*/
	instr = INSTR_CREATE_mov_imm(drcontext, opnd1, opnd2);
	instrlist_meta_preinsert(ilist, where, instr);
	/* jmp code_cache */
	opnd1 = opnd_create_pc(code_cache);
	instr = INSTR_CREATE_jmp(drcontext, opnd1);
	instrlist_meta_preinsert(ilist, where, instr);

	/* Restore scratch registers */
	instrlist_meta_preinsert(ilist, where, restore);
	if (drreg_unreserve_register(drcontext, ilist, where, reg1) != DRREG_SUCCESS ||
		drreg_unreserve_register(drcontext, ilist, where, reg2) != DRREG_SUCCESS)
		DR_ASSERT(false);
}
