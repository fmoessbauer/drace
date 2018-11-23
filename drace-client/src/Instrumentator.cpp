#pragma once

#include "globals.h"

#include <dr_api.h>
#include <drmgr.h>
#include <drreg.h>
#include <drutil.h>
#include <dr_tools.h>

#include <detector/detector_if.h>

#include "Instrumentator.h"
#include "memory-tracker.h"
#include "shadow-stack.h"
#include "statistics.h"
#include "Module.h"

namespace drace {
	Instrumentator::Instrumentator()
	{
		/* We need 3 reg slots beyond drreg's eflags slots => 3 slots */
		drreg_options_t ops = { sizeof(ops), 4, false };

		/* Ensure that atomic and native type have equal size as otherwise
		   instrumentation reads invalid value */
		using no_flush_t = decltype(MemoryTracker::no_flush);
		static_assert(
			sizeof(no_flush_t) == sizeof(no_flush_t::value_type)
			, "atomic uint64 size differs from uint64 size");

		DR_ASSERT(drreg_init(&ops) == DRREG_SUCCESS);
		page_size = dr_page_size();

		tls_idx = drmgr_register_tls_field();
		DR_ASSERT(tls_idx != -1);

		// Prepare vector with allowed registers for REG2
		drreg_init_and_fill_vector(&allowed_xcx, false);
		drreg_set_vector_entry(&allowed_xcx, DR_REG_XCX, true);

		// Initialize Code Caches
		code_cache_init();

		DR_ASSERT(
			drmgr_register_bb_app2app_event(instr_event_bb_app2app, NULL) &&
			drmgr_register_bb_instrumentation_event(instr_event_app_analysis, instr_event_app_instruction, NULL));

		LOG_INFO(0, "Initialized");
	}

	Instrumentator::~Instrumentator() {
		dr_nonheap_free(cc_flush, page_size);

		drvector_delete(&allowed_xcx);

		if (!drmgr_unregister_bb_app2app_event(instr_event_bb_app2app) ||
			!drmgr_unregister_bb_instrumentation_event(instr_event_app_analysis) ||
			drreg_exit() != DRREG_SUCCESS)
			DR_ASSERT(false);
	}

	/*
	 * Thread init Event
	 */
	void Instrumentator::event_thread_init(void *drcontext)
	{
		/* allocate thread private data */
		size_t space_size = sizeof(ThreadState) + alignof(ThreadState);
		void * tls_buffer = dr_thread_alloc(drcontext, space_size);
		void * algn_buffer = std::align(alignof(ThreadState), sizeof(ThreadState), tls_buffer, space_size);
		drmgr_set_tls_field(drcontext, tls_idx, algn_buffer);

		// Initialize struct at given location (placement new)
		// As this includes allocation, we have to be in dr state
		ThreadState * data = new (algn_buffer) ThreadState(drcontext);
		data->alloc_begin = tls_buffer;

		data->mtrack.update_sampling();

		data->tid = dr_get_thread_id(drcontext);

		data->mutex_book.reserve(MUTEX_MAP_SIZE);

		// If threads are started concurrently, assume first thread is correct one
		bool true_val = true;
		if (th_start_pending.compare_exchange_weak(true_val, false,
			std::memory_order_relaxed, std::memory_order_relaxed))
		{
			last_th_start = data->tid;
			data->mtrack.disable();
		}

		// this is the master thread
		if (params.exclude_master && (data->tid == runtime_tid)) {
			data->mtrack.disable();
			data->mtrack._event_cnt++;
		}

		dr_rwlock_write_lock(tls_rw_mutex);
		TLS_buckets.emplace(data->tid, data);
		dr_rwlock_write_unlock(tls_rw_mutex);
	}

	void Instrumentator::event_thread_exit(void *drcontext)
	{
		ThreadState* data = (ThreadState*)drmgr_get_tls_field(drcontext, tls_idx);

		dr_rwlock_write_lock(tls_rw_mutex);
		// as this is a exclusive lock and this is the only place
		// where stats are combined, we use it
		*stats |= data->stats;

		// As TLS_buckets stores a pointer to this tls,
		// we cannot release the lock until the deallocation
		// is finished. Otherwise a second thread might
		// read a local copy of the tls while holding
		// a read lock on TLS_buckets during deallocation
		DR_ASSERT(TLS_buckets.count(data->tid) == 1);
		TLS_buckets.erase(data->tid);
		dr_rwlock_write_unlock(tls_rw_mutex);

		data->stats.print_summary(std::cout);

		// Cleanup TLS
		// As we cannot rely on current drcontext here, use provided one
		data->mtrack.deallocate(drcontext);
		// deconstruct struct
		data->~ThreadState();
		dr_thread_free(drcontext, data->alloc_begin, sizeof(ThreadState) + alignof(ThreadState));
	}


	/* We transform string loops into regular loops so we can more easily
	* monitor every memory reference they make.
	*/
	dr_emit_flags_t Instrumentator::event_bb_app2app(void *drcontext, void *tag, instrlist_t *bb, bool for_trace, bool translating)
	{
		if (!drutil_expand_rep_string(drcontext, bb)) {
			DR_ASSERT(false);
			/* in release build, carry on: we'll just miss per-iter refs */
		}
		return DR_EMIT_DEFAULT;
	}

	dr_emit_flags_t Instrumentator::event_app_analysis(void *drcontext, void *tag, instrlist_t *bb,
		bool for_trace, bool translating, OUT void **user_data) {
		using INSTR_FLAGS = module::Metadata::INSTR_FLAGS;

		if (translating)
			return DR_EMIT_DEFAULT;

		if (for_trace && params.excl_traces) {
			*user_data = (void*)INSTR_FLAGS::STACK;
			return DR_EMIT_DEFAULT;
		}

		auto instrument_bb = INSTR_FLAGS::MEMORY;
		app_pc bb_addr = dr_fragment_app_pc(tag);

		// Lookup module from cache, hit is very likely as adiacent bb's 
		// are mostly in the same module
		module::Metadata * modptr = mc.lookup(bb_addr);
		if (nullptr != modptr) {
			instrument_bb = modptr->instrument;
		}
		else {
			module_tracker->lock_read();
			modptr = (module_tracker->get_module_containing(bb_addr)).get();
			if (modptr) {
				// bb in known module
				instrument_bb = modptr->instrument;
				mc.update(modptr);
			}
			else {
				// Module not known
				LOG_TRACE(0, "Module unknown, probably JIT code (%p)", bb_addr);
				instrument_bb = (INSTR_FLAGS)(INSTR_FLAGS::MEMORY | INSTR_FLAGS::STACK);
			}
			module_tracker->unlock_read();
		}

		// Do not instrument if block is frequent
		if (for_trace && instrument_bb) {
			ThreadState * data = (ThreadState*)drmgr_get_tls_field(drcontext, tls_idx);
			// TODO
			if (MemoryTracker::pc_in_freq(data, bb_addr)) {
				instrument_bb = INSTR_FLAGS::NONE;
			}
		}

		// Avoid temporary allocation by using ptr-value directly
		*user_data = (void*)instrument_bb;
		return DR_EMIT_DEFAULT;
	}

	dr_emit_flags_t Instrumentator::event_app_instruction(void *drcontext, void *tag, instrlist_t *bb,
		instr_t *instr, bool for_trace,
		bool translating, void *user_data)
	{
		if (translating)
			return DR_EMIT_DEFAULT;

		using INSTR_FLAGS = module::Metadata::INSTR_FLAGS;
		auto instrument_instr = (INSTR_FLAGS)(uint8_t)user_data;
		// we treat all atomic accesses as reads
		bool instr_is_atomic{ false };

		if (!instr_is_app(instr))
			return DR_EMIT_DEFAULT;

		if (instrument_instr & INSTR_FLAGS::STACK) {
			// Instrument ShadowStack
			ShadowStack::instrument(drcontext, tag, bb, instr, for_trace, translating, user_data);
		}

		if (!(instrument_instr & INSTR_FLAGS::MEMORY))
			return DR_EMIT_DEFAULT;

		if (!instr_reads_memory(instr) && !instr_writes_memory(instr))
			return DR_EMIT_DEFAULT;

		// exclude pop and push
		int opcode = instr_get_opcode(instr);
		if (opcode == OP_pop || opcode == OP_popa || opcode == OP_popf ||
			opcode == OP_push || opcode == OP_pusha || opcode == OP_pushf) {
			return DR_EMIT_DEFAULT;
		}

		// exclude other modifications of stackptr
		if (instr_reads_from_reg(instr, DR_REG_XSP, DR_QUERY_DEFAULT) ||
			instr_writes_to_reg(instr, DR_REG_XSP, DR_QUERY_DEFAULT) ||
			instr_reads_from_reg(instr, DR_REG_XBP, DR_QUERY_DEFAULT) ||
			instr_writes_to_reg(instr, DR_REG_XBP, DR_QUERY_DEFAULT))
		{
			return DR_EMIT_DEFAULT;
		}

		// atomic instruction
		if (instr_get_prefix_flag(instr, PREFIX_LOCK))
			instr_is_atomic = true;

		// Sampling: Only instrument some instructions
		// This is a racy increment, but we do not rely on exact numbers
		auto cnt = ++instrum_count;
		if (cnt % params.instr_rate != 0) {
			return DR_EMIT_DEFAULT;
		}

		/* insert code to add an entry for each memory reference opnd */
		for (int i = 0; i < instr_num_srcs(instr); i++) {
			opnd_t src = instr_get_src(instr, i);
			if (opnd_is_memory_reference(src))
				instrument_mem(drcontext, bb, instr, src, false);
		}

		for (int i = 0; i < instr_num_dsts(instr); i++) {
			opnd_t dst = instr_get_dst(instr, i);
			if (opnd_is_memory_reference(dst))
				instrument_mem(drcontext, bb, instr, dst, !instr_is_atomic);
		}

		return DR_EMIT_DEFAULT;
	}

	void Instrumentator::code_cache_init(void) {
		void         *drcontext;
		instrlist_t  *ilist;
		// address to jump back
		instr_t      *where;
		byte         *end;

		drcontext = dr_get_current_drcontext();
		cc_flush = (app_pc)dr_nonheap_alloc(page_size,
			DR_MEMPROT_READ |
			DR_MEMPROT_WRITE |
			DR_MEMPROT_EXEC);
		ilist = instrlist_create(drcontext);
		/* The lean procecure simply performs a clean call, and then jump back */
		/* jump back to the DR's code cache */
		where = INSTR_CREATE_jmp_ind(drcontext, opnd_create_reg(DR_REG_XCX));
		instrlist_meta_append(ilist, where);
		/* clean call */
		dr_insert_clean_call(drcontext, ilist, where, MemoryTracker::process_buffer_static, false, 0);
		/* Encodes the instructions into memory and then cleans up. */
		end = instrlist_encode(drcontext, ilist, cc_flush, false);
		DR_ASSERT((size_t)(end - cc_flush) < page_size);
		instrlist_clear_and_destroy(drcontext, ilist);
		/* set the memory as just +rx now */
		dr_memory_protect(cc_flush, page_size, DR_MEMPROT_READ | DR_MEMPROT_EXEC);
	}
} // namespace drace

// Inline Instrumentation in src/instr
