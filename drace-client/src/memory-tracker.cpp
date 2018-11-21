#pragma once

#include "globals.h"

#include <dr_api.h>
#include <drmgr.h>
#include <drreg.h>
#include <drutil.h>
#include <dr_tools.h>

#include <detector/detector_if.h>

#include "ThreadState.h"
#include "memory-tracker.h"
#include "instrumentator.h"
#include "Module.h"
#include "MSR.h"
#include "symbols.h"
#include "shadow-stack.h"
#include "function-wrapper.h"
#include "statistics.h"
#include "ipc/SharedMemory.h"
#include "ipc/SMData.h"


namespace drace {
	MemoryTracker::MemoryTracker(
		void * drcontext,
		Statistics & stats)
		: //_prng(std::chrono::high_resolution_clock::now().time_since_epoch().count()),
		  _stats(stats)
	{
		mem_buf.resize(MEM_BUF_SIZE, drcontext);
		buf_ptr = mem_buf.data;
		/* set buf_end to be negative of address of buffer end for the lea later */
		buf_end = -(ptr_int_t)(mem_buf.data + MEM_BUF_SIZE);
		tid = dr_get_thread_id(drcontext);
		// Init ShadowStack with max_size + 1 Element for PC of access
		stack.resize(ShadowStack::max_size + 1, drcontext);

		if (!params.fastmode) {
			th_towait.reserve(TLS_buckets.bucket_count());
		}

		// determin stack range of this thread
		dr_switch_to_app_state(drcontext);
		// dr does not support this natively, so make syscall in app context
		GetCurrentThreadStackLimits(&(_appstack_beg), &(_appstack_end));
		LOG_NOTICE(tid, "stack from %p to %p", _appstack_beg, _appstack_end);
	}

	MemoryTracker::~MemoryTracker() {
		deallocate(dr_get_current_drcontext());
	}

	inline std::vector<uint64_t> get_pcs_from_hist(const Statistics::hist_t & hist) {
		std::vector<uint64_t> result;
		result.reserve(hist.size());

		std::transform(hist.begin(), hist.end(), std::back_inserter(result),
			[](const Statistics::hist_t::value_type & v) {return v.first; });

		std::sort(result.begin(), result.end());

		return result;
	}

	void MemoryTracker::deallocate(void* drcontext) {
		if (!live) return;
		DR_ASSERT(detector_data != nullptr);

		flush_all_threads(*this, true, false);
		detector::join(runtime_tid.load(std::memory_order_relaxed), tid, detector_data);

		mem_buf.deallocate(drcontext);
		stack.deallocate(drcontext);
		live = false;
	}

	void MemoryTracker::update_cache() {
		// TODO: optimize this
		const auto & new_freq = _stats.pc_hits.computeOutput<Statistics::hist_t>();
		LOG_NOTICE(tid, "Flush Cache with size %i", new_freq.size());
		if (params.lossy_flush) {
			void * drcontext = dr_get_current_drcontext();
			const auto & pc_new = get_pcs_from_hist(new_freq);
			const auto & pc_old = _stats.freq_pcs;

			std::vector<uint64_t> difference;
			difference.reserve(pc_new.size() + pc_old.size());

			// get difference between both histograms
			std::set_symmetric_difference(pc_new.begin(), pc_new.end(), pc_old.begin(), pc_old.end(), std::back_inserter(difference));

			// Flush new fragments
			for (const auto pc : difference) {
				instrumentator->flush_region(drcontext, pc);
			}
			_stats.freq_pcs = pc_new;
		}
		_stats.freq_pc_hist = new_freq;
	}

	bool MemoryTracker::pc_in_freq(ThreadState * data, void* bb) {
		const auto & freq_pcs = data->stats.freq_pcs;
		return std::binary_search(freq_pcs.begin(), freq_pcs.end(), ((uint64_t)bb >> MemoryTracker::HIST_PC_RES));
	}

	void MemoryTracker::analyze_access() {
		if (detector_data == nullptr) {
			// Thread starts with a pending clean-call
			// We missed a fork
			// 1. Flush all threads (except this thread)
			// 2. Fork thread
			LOG_INFO(tid, "Missed a fork, do it now");
			detector::fork(runtime_tid.load(std::memory_order_relaxed), tid, &detector_data);
			// and seed prng
			_prng.seed(std::chrono::high_resolution_clock::now().time_since_epoch().count());
		}

		// TODO: move to instrumentator
		// toggle detector on external state change
		// avoid expensive mod by comparing with bitmask
		if ((stats->flushes & (0xF - 1)) == (0xF - 1)) {
			// lessen impact of expensive SHM accesses
			handle_ext_state();
		}

		if (_enabled) {
			mem_ref_t * mem_ref = (mem_ref_t *)mem_buf.data;
			uint64_t num_refs = (uint64_t)((mem_ref_t *)buf_ptr - mem_ref);

			if (num_refs > 0) {
				//dr_printf("[%i] Process buffer, noflush: %i, refs: %i\n", data->tid, data->no_flush.load(std::memory_order_relaxed), num_refs);
				DR_ASSERT(detector_data != nullptr);

				// In non-fast-mode we have to protect the stack
				if (!params.fastmode)
					dr_mutex_lock(th_mutex);

				DR_ASSERT(stack.entries >= 0);
				stack.entries++; // We have one spare-element
				int size = std::min((unsigned)stack.entries, params.stack_size); // TODO: remove params
				int offset = stack.entries - size;

				// Lossy count first mem-ref (all are adiacent as after each call is flushed)
				if (params.lossy) {
					_stats.pc_hits.processItem((uint64_t)mem_ref->pc >> HIST_PC_RES);
					if ((_stats.flushes & (CC_UPDATE_PERIOD - 1)) == (CC_UPDATE_PERIOD - 1)) {
						update_cache();
					}
				}

				for (uint64_t i = 0; i < num_refs; ++i) {
					// todo: better use iterator like access
					mem_ref = &((mem_ref_t *)mem_buf.data)[i];
					if (params.excl_stack &&
						((ULONG_PTR)mem_ref->addr > _appstack_beg) && 
						((ULONG_PTR)mem_ref->addr < _appstack_end))
					{
						// this reference points into the stack range, skip
						continue;
					}
					if ((uint64_t)mem_ref->addr > PROC_ADDR_LIMIT) {
						// outside process address space
						continue;
					}
					// this is a mem-ref candidate
					if (!params.fastmode) {
						// in fast-mode, sampling is implemented in the instrumentation
						if (!sample_ref()) {
							continue;
						}
					}
					stack.data[stack.entries - 1] = mem_ref->pc;
					if (mem_ref->write) {
						//printf("[%i] WRITE %p, PC: %p\n", tid, mem_ref->addr, mem_ref->pc);
						detector::write(detector_data, stack.data + offset, size, mem_ref->addr, mem_ref->size);
					}
					else {
						//printf("[%i] READ  %p, PC: %p\n", tid, mem_ref->addr, mem_ref->pc);
						detector::read(detector_data, stack.data + offset, size, mem_ref->addr, mem_ref->size);
					}
					++(_stats.proc_refs);
				}
				stack.entries--;
				if (!params.fastmode)
					dr_mutex_unlock(th_mutex);
				_stats.total_refs += num_refs;
			}
		}
		buf_ptr = mem_buf.data;

		if (!params.fastmode && !no_flush.load(std::memory_order_relaxed)) {
			uint64_t expect = 0;
			if (no_flush.compare_exchange_weak(expect, 1, std::memory_order_relaxed)) {
				if (params.yield_on_evt) {
					LOG_INFO(tid, "yield on event");
					dr_thread_yield();
				}
			}
		}
	}

	void MemoryTracker::clear_buffer(void)
	{
		mem_ref_t *mem_ref = (mem_ref_t *)mem_buf.data;
		uint64_t num_refs = (uint64_t)((mem_ref_t *)buf_ptr - mem_ref);

		_stats.proc_refs += num_refs;
		buf_ptr = mem_buf.data;
		no_flush.store(true, std::memory_order_relaxed);
	}


	void MemoryTracker::process_buffer_static() {
		void * drcontext = dr_get_current_drcontext();
		ThreadState * data = (ThreadState*) drmgr_get_tls_field(drcontext, tls_idx);
		data->mtrack.process_buffer();
	}

	void MemoryTracker::flush_all_threads(MemoryTracker & mtr, bool self, bool flush_external) {
		mtr._stats.flush_events++;
		if (params.fastmode) {
			if (self) {
				mtr.process_buffer();
				return;
			}
		}

		auto start = std::chrono::system_clock::now();
		// Do not run two flushes simultaneously
		int expect = false;
		int cntr = 0;
		while (mtr.flush_active.compare_exchange_weak(expect, true, std::memory_order_relaxed)) {
			if (++cntr > 100) {
				dr_thread_yield();
			}
		}

		if (self) {
			mtr.analyze_access();
		}

		mtr.th_towait.clear();

		// Preserve state by copying tls pointers to local array
		dr_rwlock_read_lock(tls_rw_mutex);


		// Get threads and notify them
		for (const auto & td : TLS_buckets) {
			if (td.first != mtr.tid && td.second->mtrack._enabled && td.second->mtrack.no_flush.load(std::memory_order_relaxed))
			{
				uint64_t refs = (td.second->mtrack.buf_ptr - td.second->mtrack.mem_buf.data) / sizeof(mem_ref_t);
				if (refs > 0) {
					//printf("[%.5i] Flush thread %.5i, ~numrefs, %u\n",
					//	data->tid, td.first, refs);
					// TODO: check if memory_order_relaxed is sufficient
					td.second->mtrack.no_flush.store(false, std::memory_order_relaxed);
					mtr.th_towait.emplace_back(td.first, td.second->mtrack);
				}
			}
		}

		// wait until all threads flushed
		// this is a hacky half-barrier implementation
		for (const auto & td : mtr.th_towait) {
			// Flush thread given that:
			// 1. thread is not the calling thread
			// 2. thread is not disabled
			unsigned waits = 0;
			while (!td.second.no_flush.load(std::memory_order_relaxed)) {
				if (++waits > 100) {
					// avoid busy-waiting and blocking CPU if other thread did not flush
					// within given period
					if (flush_external) {
						ptr_uint_t expect = false;
						if (td.second.external_flush.compare_exchange_weak(expect, true, std::memory_order_release)) {
							//	//dr_printf("<< Thread %i took to long to flush, flush externaly\n", td.first);
							td.second.clear_buffer();
							td.second._stats.external_flushes++;
							td.second.external_flush.store(false, std::memory_order_release);
						}
					}
					// Here we might loose some refs, clear them
					td.second.buf_ptr = td.second.mem_buf.data;
					td.second.no_flush.store(true, std::memory_order_relaxed);
					break;
				}
			}
		}
		mtr.flush_active.store(false, std::memory_order_relaxed);
		dr_rwlock_read_unlock(tls_rw_mutex);

		mtr._stats.flushes++;
		auto duration = std::chrono::system_clock::now() - start;
		mtr._stats.time_in_flushes += std::chrono::duration_cast<std::chrono::milliseconds>(duration);
	}

	void MemoryTracker::update_sampling() {
		unsigned delta;
		if (params.sampling_rate < 10)
			delta = 1;
		else {
			delta = 0.1 * params.sampling_rate;
		}
		_min_period = std::max(params.sampling_rate - delta, 1u);
		_max_period = params.sampling_rate + delta;
		_sampling_period = params.sampling_rate;
		_sample_pos = _sampling_period;
	}

	void MemoryTracker::handle_ext_state() {
		if (shmdriver) {
			bool external_state = extcb->get()->enabled.load(std::memory_order_relaxed);
			if (_enable_external != external_state) {
				{
					LOG_INFO(0, "externally switched state: %s", external_state ? "ON" : "OFF");
					_enable_external = external_state;
					if (!external_state) {
						funwrap::event::beg_excl_region(*this);
					}
					else {
						funwrap::event::end_excl_region(*this);
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

} // namespace drace

// Inline Instrumentation in src/instr
