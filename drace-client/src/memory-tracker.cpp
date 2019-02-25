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

#include "globals.h"

#include <dr_api.h>
#include <drmgr.h>
#include <drreg.h>
#include <drutil.h>
#include <dr_tools.h>

#include <detector/detector_if.h>

#include "memory-tracker.h"
#include "Module.h"
#include "MSR.h"
#include "symbols.h"
#include "shadow-stack.h"
#include "function-wrapper.h"
#include "statistics.h"
#include "ipc/SharedMemory.h"
#include "ipc/SMData.h"


namespace drace {
	MemoryTracker::MemoryTracker()
		: _prng(static_cast<unsigned>(
                    std::chrono::high_resolution_clock::now().time_since_epoch().count()))
	{
		/* We need 3 reg slots beyond drreg's eflags slots => 3 slots */
		drreg_options_t ops = { sizeof(ops), 4, false };

		/* Ensure that atomic and native type have equal size as otherwise
		   instrumentation reads invalid value */
		using no_flush_t = decltype(per_thread_t::no_flush);
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

		// setup sampling
		update_sampling();

		DR_ASSERT(
			drmgr_register_bb_app2app_event(instr_event_bb_app2app, NULL) &&
			drmgr_register_bb_instrumentation_event(instr_event_app_analysis, instr_event_app_instruction, NULL));

		LOG_INFO(0, "Initialized");
	}

	MemoryTracker::~MemoryTracker() {
		dr_nonheap_free(cc_flush, page_size);

		drvector_delete(&allowed_xcx);

		if (!drmgr_unregister_bb_app2app_event(instr_event_bb_app2app) ||
			!drmgr_unregister_bb_instrumentation_event(instr_event_app_analysis) ||
			drreg_exit() != DRREG_SUCCESS)
			DR_ASSERT(false);
	}

	inline void flush_region(void* drcontext, uint64_t pc) {
		// Flush this area from the code cache
		dr_delay_flush_region((app_pc)(pc << MemoryTracker::HIST_PC_RES), 1 << MemoryTracker::HIST_PC_RES, 0, NULL);
		LOG_NOTICE(dr_get_thread_id(drcontext), "Flushed %p", pc << MemoryTracker::HIST_PC_RES);
	}

	inline std::vector<uint64_t> get_pcs_from_hist(const Statistics::hist_t & hist) {
		std::vector<uint64_t> result;
		result.reserve(hist.size());

		std::transform(hist.begin(), hist.end(), std::back_inserter(result),
			[](const Statistics::hist_t::value_type & v) {return v.first; });

		std::sort(result.begin(), result.end());

		return result;
	}

	void MemoryTracker::update_cache(per_thread_t * data) {
		// TODO: optimize this
		const auto & new_freq = data->stats->pc_hits.computeOutput<Statistics::hist_t>();
		LOG_NOTICE(data->tid, "Flush Cache with size %i", new_freq.size());
		if (params.lossy_flush) {
			void * drcontext = dr_get_current_drcontext();
			const auto & pc_new = get_pcs_from_hist(new_freq);
			const auto & pc_old = data->stats->freq_pcs;

			std::vector<uint64_t> difference;
			difference.reserve(pc_new.size() + pc_old.size());

			// get difference between both histograms
			std::set_symmetric_difference(pc_new.begin(), pc_new.end(), pc_old.begin(), pc_old.end(), std::back_inserter(difference));

			// Flush new fragments
			for (const auto pc : difference) {
				flush_region(drcontext, pc);
			}
			data->stats->freq_pcs = pc_new;
		}
		data->stats->freq_pc_hist = new_freq;
	}

	bool MemoryTracker::pc_in_freq(per_thread_t * data, void* bb) {
		const auto & freq_pcs = data->stats->freq_pcs;
		return std::binary_search(freq_pcs.begin(), freq_pcs.end(), ((uint64_t)bb >> MemoryTracker::HIST_PC_RES));
	}

	void MemoryTracker::analyze_access(per_thread_t * data) {
		DR_ASSERT(data != nullptr);

		if (data->detector_data == nullptr) {
			// Thread starts with a pending clean-call
			// We missed a fork
			// 1. Flush all threads (except this thread)
			// 2. Fork thread
			LOG_TRACE(data->tid, "Missed a fork, do it now");
			detector::fork(
                runtime_tid.load(std::memory_order_relaxed),
                static_cast<detector::tid_t>(data->tid),
                &(data->detector_data));
		}

		// toggle detector on external state change
		// avoid expensive mod by comparing with bitmask
		if ((data->stats->flushes & (0xF - 1)) == (0xF - 1)) {
			// lessen impact of expensive SHM accesses
			memory_tracker->handle_ext_state(data);
		}

		if (data->enabled) {
			mem_ref_t * mem_ref = (mem_ref_t *)data->mem_buf.data;
			uint64_t num_refs = (uint64_t)((mem_ref_t *)data->buf_ptr - mem_ref);

			if (num_refs > 0) {
				//dr_printf("[%i] Process buffer, noflush: %i, refs: %i\n", data->tid, data->no_flush.load(std::memory_order_relaxed), num_refs);
				DR_ASSERT(data->detector_data != nullptr);

				auto * stack = &(data->stack);

				// In non-fast-mode we have to protect the stack
				if (!params.fastmode)
					dr_mutex_lock(th_mutex);

				DR_ASSERT(stack->entries >= 0);
				stack->entries++; // We have one spare-element
				int size = std::min((unsigned)stack->entries, params.stack_size);
				int offset = stack->entries - size;

				// Lossy count first mem-ref (all are adiacent as after each call is flushed)
				if (params.lossy) {
					data->stats->pc_hits.processItem((uint64_t)mem_ref->pc >> HIST_PC_RES);
					if ((data->stats->flushes & (CC_UPDATE_PERIOD - 1)) == (CC_UPDATE_PERIOD - 1)) {
						update_cache(data);
					}
				}

				for (uint64_t i = 0; i < num_refs; ++i) {
					// todo: better use iterator like access
					mem_ref = &((mem_ref_t *)data->mem_buf.data)[i];
					if (params.excl_stack &&
						((ULONG_PTR)mem_ref->addr > data->appstack_beg) && 
						((ULONG_PTR)mem_ref->addr < data->appstack_end))
					{
						// this reference points into the stack range, skip
						continue;
					}
					if ((uint64_t)mem_ref->addr > PROC_ADDR_LIMIT) {
						// outside process address space
						continue;
					}
					// this is a mem-ref candidate
					//if (!memory_tracker->sample_ref(data)) {
					//	continue;
					//}

					stack->data[stack->entries - 1] = mem_ref->pc;
					if (mem_ref->write) {
						detector::write(data->detector_data, stack->data + offset, size, mem_ref->addr, mem_ref->size);
						//printf("[%i] WRITE %p, PC: %p\n", data->tid, mem_ref->addr, mem_ref->pc);
					}
					else {
						detector::read(data->detector_data, stack->data + offset, size, mem_ref->addr, mem_ref->size);
						//printf("[%i] READ  %p, PC: %p\n", data->tid, mem_ref->addr, mem_ref->pc);
					}
					++(data->stats->proc_refs);
				}
				stack->entries--;
				if (!params.fastmode)
					dr_mutex_unlock(th_mutex);
				data->stats->total_refs += num_refs;
			}
		}
		data->buf_ptr = data->mem_buf.data;

		if (!params.fastmode && !data->no_flush.load(std::memory_order_relaxed)) {
			uint64_t expect = 0;
			data->no_flush.compare_exchange_weak(expect, 1, std::memory_order_relaxed);
		}
	}

	/*
	 * Thread init Event
	 */
	void MemoryTracker::event_thread_init(void *drcontext)
	{
		/* allocate thread private data */
		void * tls_buffer = dr_thread_alloc(drcontext, sizeof(per_thread_t));
		drmgr_set_tls_field(drcontext, tls_idx, tls_buffer);

		// Initialize struct at given location (placement new)
		per_thread_t * data = new (tls_buffer) per_thread_t;

		data->mem_buf.resize(MEM_BUF_SIZE, drcontext);
		data->buf_ptr = data->mem_buf.data;
		/* set buf_end to be negative of address of buffer end for the lea later */
		data->buf_end = -(ptr_int_t)(data->mem_buf.data + MEM_BUF_SIZE);
		data->tid = dr_get_thread_id(drcontext);
		// Init ShadowStack with max_size + 1 Element for PC of access
		data->stack.resize(ShadowStack::max_size + 1, drcontext);

		data->mutex_book.reserve(MUTEX_MAP_SIZE);
		// set first sampling period
		data->sampling_pos = params.sampling_rate;

		// If threads are started concurrently, assume first thread is correct one
		bool true_val = true;
		if (th_start_pending.compare_exchange_weak(true_val, false,
			std::memory_order_relaxed, std::memory_order_relaxed))
		{
			last_th_start = data->tid;
			disable(data);
		}

		// this is the master thread
		if (params.exclude_master && (data->tid == runtime_tid)) {
			disable_scope(data);
		}

		data->stats = std::make_unique<Statistics>(data->tid);

		dr_rwlock_write_lock(tls_rw_mutex);
		TLS_buckets.emplace(data->tid, data);
		data->th_towait.reserve(TLS_buckets.bucket_count());
		dr_rwlock_write_unlock(tls_rw_mutex);

		flush_all_threads(data, false, false);

		// determin stack range of this thread
		dr_switch_to_app_state(drcontext);
		// dr does not support this natively, so make syscall in app context
		GetCurrentThreadStackLimits(&(data->appstack_beg), &(data->appstack_end));
		LOG_NOTICE(data->tid, "stack from %p to %p", data->appstack_beg, data->appstack_end);
	}

	void MemoryTracker::event_thread_exit(void *drcontext)
	{
		per_thread_t* data = (per_thread_t*)drmgr_get_tls_field(drcontext, tls_idx);

		flush_all_threads(data, true, false);

		detector::join(
            runtime_tid.load(std::memory_order_relaxed),
            static_cast<detector::tid_t>(data->tid),
            data->detector_data);

		dr_rwlock_write_lock(tls_rw_mutex);
		// as this is a exclusive lock and this is the only place
		// where stats are combined, we use it
		*stats |= *(data->stats);

		// As TLS_buckets stores a pointer to this tls,
		// we cannot release the lock until the deallocation
		// is finished. Otherwise a second thread might
		// read a local copy of the tls while holding
		// a read lock on TLS_buckets during deallocation
		DR_ASSERT(TLS_buckets.count(data->tid) == 1);
		TLS_buckets.erase(data->tid);
		dr_rwlock_write_unlock(tls_rw_mutex);

		data->stats->print_summary(drace::log_target);

		// Cleanup TLS
		// As we cannot rely on current drcontext here, use provided one
		data->stack.deallocate(drcontext);
		data->mem_buf.deallocate(drcontext);
		// deconstruct struct
		data->~per_thread_t();
		dr_thread_free(drcontext, data, sizeof(per_thread_t));
	}


	/* We transform string loops into regular loops so we can more easily
	* monitor every memory reference they make.
	*/
	dr_emit_flags_t MemoryTracker::event_bb_app2app(void *drcontext, void *tag, instrlist_t *bb, bool for_trace, bool translating)
	{
		if (!drutil_expand_rep_string(drcontext, bb)) {
			DR_ASSERT(false);
			/* in release build, carry on: we'll just miss per-iter refs */
		}
		return DR_EMIT_DEFAULT;
	}

	dr_emit_flags_t MemoryTracker::event_app_analysis(void *drcontext, void *tag, instrlist_t *bb,
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
			per_thread_t * data = (per_thread_t*)drmgr_get_tls_field(drcontext, tls_idx);
			if (pc_in_freq(data, bb_addr)) {
				instrument_bb = INSTR_FLAGS::NONE;
			}
		}

		// Avoid temporary allocation by using ptr-value directly
		*user_data = (void*)instrument_bb;
		return DR_EMIT_DEFAULT;
	}

	dr_emit_flags_t MemoryTracker::event_app_instruction(void *drcontext, void *tag, instrlist_t *bb,
		instr_t *instr, bool for_trace,
		bool translating, void *user_data)
	{
		if (translating)
			return DR_EMIT_DEFAULT;

		using INSTR_FLAGS = module::Metadata::INSTR_FLAGS;
		auto instrument_instr = (INSTR_FLAGS)(util::unsafe_ptr_cast<uint8_t>(user_data));
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

		if (params.excl_stack) {
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

	/* clean_call dumps the memory reference info into the analyzer */
	void MemoryTracker::process_buffer(void)
	{
		void *drcontext = dr_get_current_drcontext();
		per_thread_t * data = (per_thread_t*)drmgr_get_tls_field(drcontext, tls_idx);

		analyze_access(data);
		data->stats->flushes++;

		// block until flush is done
		unsigned tries = 0;
		if (!params.fastmode) {
			while (memory_tracker->flush_active.load(std::memory_order_relaxed)) {
				++tries;
				if (tries > 100) {
					dr_thread_yield();
					tries = 0;
				}
			}
		}
	}

	void MemoryTracker::clear_buffer(void)
	{
		void *drcontext = dr_get_current_drcontext();
		per_thread_t *data = (per_thread_t*)drmgr_get_tls_field(drcontext, tls_idx);
		mem_ref_t *mem_ref = (mem_ref_t *)data->mem_buf.data;
		uint64_t num_refs = (uint64_t)((mem_ref_t *)data->buf_ptr - mem_ref);

		data->stats->proc_refs += num_refs;
		data->stats->flushes++;
		data->buf_ptr = data->mem_buf.data;
		data->no_flush.store(true, std::memory_order_relaxed);
	}

	/* Request a flush of all non-disabled threads.
	*  \Warning: This function read-locks the TLS mutex
	*/
	void MemoryTracker::flush_all_threads(per_thread_t * data, bool self, bool flush_external) {
		if (params.fastmode) {
			if (self) process_buffer();
			return;
		}

		DR_ASSERT(data != nullptr);
		auto start = std::chrono::system_clock::now();
		data->stats->flush_events++;

		// Do not run two flushes simultaneously
		int expect = false;
		int cntr = 0;
		while (memory_tracker->flush_active.compare_exchange_weak(expect, true, std::memory_order_relaxed)) {
			if (++cntr > 100) {
				dr_thread_yield();
			}
		}

		if (self) {
			analyze_access(data);
		}

		data->th_towait.clear();

		// Preserve state by copying tls pointers to local array
		dr_rwlock_read_lock(tls_rw_mutex);


		// Get threads and notify them
		for (const auto & td : TLS_buckets) {
			if (td.first != data->tid && td.second->enabled && td.second->no_flush.load(std::memory_order_relaxed))
			{
				uint64_t refs = (td.second->buf_ptr - td.second->mem_buf.data) / sizeof(mem_ref_t);
				if (refs > 0) {
					//printf("[%.5i] Flush thread %.5i, ~numrefs, %u\n",
					//	data->tid, td.first, refs);
					// TODO: check if memory_order_relaxed is sufficient
					td.second->no_flush.store(false, std::memory_order_relaxed);
					data->th_towait.push_back(td);
				}
			}
		}

		// wait until all threads flushed
		// this is a hacky half-barrier implementation
		for (const auto & td : data->th_towait) {
			// Flush thread given that:
			// 1. thread is not the calling thread
			// 2. thread is not disabled
			unsigned waits = 0;
			while (!td.second->no_flush.load(std::memory_order_relaxed)) {
				if (++waits > 100) {
					// avoid busy-waiting and blocking CPU if other thread did not flush
					// within given period
					if (flush_external) {
						bool expect = false;
						if (td.second->external_flush.compare_exchange_weak(expect, true, std::memory_order_release)) {
							//	//dr_printf("<< Thread %i took to long to flush, flush externaly\n", td.first);
							analyze_access(td.second);
							td.second->stats->external_flushes++;
							td.second->external_flush.store(false, std::memory_order_release);
						}
					}
					// Here we might loose some refs, clear them
					td.second->buf_ptr = td.second->mem_buf.data;
					td.second->no_flush.store(true, std::memory_order_relaxed);
					break;
				}
			}
		}
		memory_tracker->flush_active.store(false, std::memory_order_relaxed);
		dr_rwlock_read_unlock(tls_rw_mutex);

		auto duration = std::chrono::system_clock::now() - start;
		data->stats->time_in_flushes += std::chrono::duration_cast<std::chrono::milliseconds>(duration);
	}

	void MemoryTracker::code_cache_init(void) {
		void         *drcontext;
		instrlist_t  *ilist;
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
		dr_insert_clean_call(drcontext, ilist, where, (void *)process_buffer, false, 0);
		/* Encodes the instructions into memory and then cleans up. */
		end = instrlist_encode(drcontext, ilist, cc_flush, false);
		DR_ASSERT((size_t)(end - cc_flush) < page_size);
		instrlist_clear_and_destroy(drcontext, ilist);
		/* set the memory as just +rx now */
		dr_memory_protect(cc_flush, page_size, DR_MEMPROT_READ | DR_MEMPROT_EXEC);
	}

	void MemoryTracker::handle_ext_state(per_thread_t * data) {
		if (shmdriver) {
			bool external_state = extcb->get()->enabled.load(std::memory_order_relaxed);
			if (data->enable_external != external_state) {
				{
					LOG_INFO(0, "externally switched state: %s", external_state ? "ON" : "OFF");
					data->enable_external = external_state;
					if (!external_state) {
						funwrap::event::beg_excl_region(data);
					}
					else {
						funwrap::event::end_excl_region(data);
					}
				}
			}
			// set sampling rate
			unsigned sampling_rate = extcb->get()->sampling_rate.load(std::memory_order_relaxed);
			if (sampling_rate != params.sampling_rate) {
				LOG_INFO(0, "externally changed sampling rate to: %i", sampling_rate);
				params.sampling_rate = sampling_rate;
				update_sampling();
			}
		}
	}

	void MemoryTracker::update_sampling() {
		unsigned delta;
		if (params.sampling_rate < 10)
			delta = 1;
		else {
			delta = static_cast<unsigned>(0.1 * params.sampling_rate);
		}
		_min_period = std::max(params.sampling_rate - delta, 1u);
		_max_period = params.sampling_rate + delta;
	}

} // namespace drace

// Inline Instrumentation in src/instr
