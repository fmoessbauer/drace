#pragma once

#include <dr_api.h>
#include <drmgr.h>
#include <drreg.h>
#include <drutil.h>
#include <dr_tools.h>

#include <detector_if.h>

#include "globals.h"
#include "memory-tracker.h"
#include "symbols.h"
#include "shadow-stack.h"
#include "statistics.h"

MemoryTracker::MemoryTracker()
	: _prng(std::chrono::high_resolution_clock::now().time_since_epoch().count()),
	_dist({1.0/params.sampling_rate, 1.0-1.0/params.sampling_rate})
{
	/* We need 2 reg slots beyond drreg's eflags slots => 3 slots */
	drreg_options_t ops = { sizeof(ops), 3, false };

	/* Ensure that atomic and native type have equal size as otherwise
	   instrumentation reads invalid value */
	static_assert(
		sizeof(decltype(per_thread_t::no_flush)) == sizeof(uint64_t)
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

	dr_printf("< Initialized\n");
}

MemoryTracker::~MemoryTracker() {
	dr_nonheap_free(cc_flush, page_size);

	drvector_delete(&allowed_xcx);

	if (!drmgr_unregister_bb_app2app_event(instr_event_bb_app2app) ||
		!drmgr_unregister_bb_instrumentation_event(instr_event_app_analysis) ||
		drreg_exit() != DRREG_SUCCESS)
		DR_ASSERT(false);
}

void MemoryTracker::analyze_access(per_thread_t * data) {
	DR_ASSERT(data != nullptr);

	mem_ref_t * mem_ref = (mem_ref_t *)data->mem_buf.array;
	uint64_t num_refs = (uint64_t)((mem_ref_t *)data->buf_ptr - mem_ref);

	if (data->detector_data == nullptr) {
		// Thread starts with a pending clean-call
		if (!params.fastmode) DR_ASSERT(num_refs == 0);
		// We missed a fork
		// 1. Flush all threads (except this thread)
		// 2. Fork thread
		dr_printf("<< [%i] Missed a fork, do it now \n", data->tid, num_refs);
		detector::fork(runtime_tid.load(std::memory_order_relaxed), data->tid, &(data->detector_data));
	}
	if (data->enabled && num_refs > 0) {
		//dr_printf("[%i] Process buffer, noflush: %i, refs: %i\n", data->tid, data->no_flush.load(std::memory_order_relaxed), num_refs);
		DR_ASSERT(data->detector_data != nullptr);

		auto * stack = &(data->stack);

		// In non-fast-mode we have to protect the stack
		if (!params.fastmode) dr_mutex_lock(th_mutex);

		stack->entries++; // We have one spare-element
		int size = min(stack->entries, params.stack_size);
		int offset = stack->entries - size;
		for (uint64_t i = 0; i < num_refs; ++i) {
			stack->array[stack->entries-1] = mem_ref->pc;
			if (mem_ref->write) {
				detector::write(data->tid, stack->array+offset, size, mem_ref->addr, mem_ref->size, data->detector_data);
				//printf("[%i] WRITE %p, PC: %p\n", data->tid, mem_ref->addr, mem_ref->pc);
			}
			else {
				detector::read(data->tid, stack->array+offset, size, mem_ref->addr, mem_ref->size, data->detector_data);
				//printf("[%i] READ  %p, PC: %p\n", data->tid, mem_ref->addr, mem_ref->pc);
			}
			++mem_ref;
		}
		stack->entries--;
		if(!params.fastmode) dr_mutex_unlock(th_mutex);

		data->stats->num_refs += num_refs;
	}
	data->stats->flushes++;
	data->buf_ptr = data->mem_buf.array;

	if (!params.fastmode && !data->no_flush.load(std::memory_order_relaxed)) {
		uint64_t expect = 0;
		if (data->no_flush.compare_exchange_weak(expect, 1, std::memory_order_relaxed)) {
			if (params.yield_on_evt) {
				dr_printf("[%.5i] YIELD\n", data->tid);
				dr_thread_yield();
			}
		}
	}
}

// Events
void MemoryTracker::event_thread_init(void *drcontext)
{
	/* allocate thread private data */
	void * tls_buffer = dr_thread_alloc(drcontext, sizeof(per_thread_t));
	drmgr_set_tls_field(drcontext, tls_idx, tls_buffer);

	// Initialize struct at given location (placement new)
	// As this includes allocation, we have to be in dr state
	DR_ASSERT(!dr_using_app_state(drcontext));
	per_thread_t * data = new (tls_buffer) per_thread_t;

	data->mem_buf.resize(MEM_BUF_SIZE, drcontext);
	data->buf_ptr = data->mem_buf.array;
	/* set buf_end to be negative of address of buffer end for the lea later */
	data->buf_end = -(ptr_int_t)(data->mem_buf.array + MEM_BUF_SIZE);
	data->tid = dr_get_thread_id(drcontext);
	data->stack.resize(ShadowStack::max_size + 1, drcontext);

	// If threads are started concurrently, assume first thread is correct one
	bool true_val = true;
	if (th_start_pending.compare_exchange_weak(true_val, false)) {
		last_th_start = data->tid;
		data->enabled = false;
	}

	// this is the master thread
	if (params.exclude_master && (data->tid == runtime_tid)) {
		data->enabled = false;
		data->event_cnt++;
	}

	data->stats = std::make_unique<Statistics>(data->tid);

	dr_rwlock_write_lock(tls_rw_mutex);
	TLS_buckets.emplace(data->tid, data);
	data->th_towait.reserve(TLS_buckets.bucket_count());
	dr_rwlock_write_unlock(tls_rw_mutex);

	// TODO: investigate why this fails
	flush_all_threads(data, false, false);
}

void MemoryTracker::event_thread_exit(void *drcontext)
{
	per_thread_t* data = (per_thread_t*)drmgr_get_tls_field(drcontext, tls_idx);

	flush_all_threads(data, true, false);

	if (!dr_using_app_state(drcontext)) {
		// in app-state system calls are not allowed
		dr_switch_to_dr_state(drcontext);
	}

	detector::join(runtime_tid.load(std::memory_order_relaxed), data->tid, data->detector_data);

	dr_mutex_lock(th_mutex);
	*stats |= *(data->stats);
	dr_mutex_unlock(th_mutex);

	dr_rwlock_write_lock(tls_rw_mutex);
	// As TLS_buckets stores a pointer to this tls,
	// we cannot release the lock until the deallocation
	// is finished. Otherwise a second thread might
	// read a local copy of the tls while holding
	// a read lock on TLS_buckets during deallocation
	DR_ASSERT(TLS_buckets.count(data->tid) == 1);
	TLS_buckets.erase(data->tid);

	data->stats->print_summary(std::cout);

	// Cleanup TLS
	// As we cannot rely on current drcontext here, use provided one
	data->stack.deallocate(drcontext);
	data->mem_buf.deallocate(drcontext);
	// deconstruct struct
	data->~per_thread_t();
	dr_thread_free(drcontext, data, sizeof(per_thread_t));

	dr_rwlock_write_unlock(tls_rw_mutex);
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
	bool instrument_bb = true;
	if (params.frequent_only && !for_trace) {
		// only instrument traces, much faster startup
		instrument_bb = false;
	}
	else {
		app_pc bb_addr = dr_fragment_app_pc(tag);

		// Lookup module from cache, hit is very likely as adiacent bb's 
		// are mostly in the same module
		auto cached = mc.lookup(bb_addr);
		if (cached.first) {
			instrument_bb = cached.second;
		}
		else {
			module_tracker->lock_read();
			auto modc = module_tracker->get_module_containing(bb_addr);
			if (modc.first) {
				// bb in known module
				instrument_bb = modc.second->instrument;
			}
			else {
				// Module not known
				instrument_bb = false;
			}
			mc.update(modc.second->base, modc.second->end, modc.second->instrument);
			module_tracker->unlock_read();
		}
	}
	*user_data = (void*)instrument_bb;
	return DR_EMIT_DEFAULT;
}

dr_emit_flags_t MemoryTracker::event_app_instruction(void *drcontext, void *tag, instrlist_t *bb,
	instr_t *instr, bool for_trace,
	bool translating, void *user_data) {
	bool instrument_instr = (bool)user_data;

	if (!instr_is_app(instr))
		return DR_EMIT_DEFAULT;


	if (!instrument_instr)
		return DR_EMIT_DEFAULT;

	// Instrument ShadowStack
	ShadowStack::instrument(drcontext, tag, bb, instr, for_trace, translating, user_data);

	// Ignore Locked instructions
	if (instr_get_prefix_flag(instr, PREFIX_LOCK))
		return DR_EMIT_DEFAULT;
	if (!instr_reads_memory(instr) && !instr_writes_memory(instr))
		return DR_EMIT_DEFAULT;

	// Sampling: Only instrument some instructions
	// This is a racy increment, but we do not rely on exact numbers
	auto cnt = ++instrum_count;
	if (cnt % params.sampling_rate != 0) {
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
			instrument_mem(drcontext, bb, instr, dst, true);
	}

	return DR_EMIT_DEFAULT;
}

/* clean_call dumps the memory reference info to the log file */
void MemoryTracker::process_buffer(void)
{
	void *drcontext = dr_get_current_drcontext();
	per_thread_t * data = (per_thread_t*)drmgr_get_tls_field(drcontext, tls_idx);

	DR_ASSERT(!dr_using_app_state(drcontext));
	analyze_access(data);

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
	mem_ref_t *mem_ref = (mem_ref_t *)data->mem_buf.array;
	uint64_t num_refs = (uint64_t)((mem_ref_t *)data->buf_ptr - mem_ref);

	data->stats->num_refs += num_refs;
	data->stats->flushes++;
	data->buf_ptr = data->mem_buf.array;
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
			uint64_t refs = (td.second->buf_ptr - td.second->mem_buf.array) / sizeof(mem_ref_t);
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
				td.second->buf_ptr = td.second->mem_buf.array;
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

// Inline Instrumentation in src/instr
