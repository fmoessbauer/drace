#pragma once
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

// Log up to notice (this parameter can be controlled using CMake)
#ifndef LOGLEVEL
#define LOGLEVEL 3
#endif

#include "config.h"
#include "aligned-stack.h"

#include <string>
#include <unordered_map>
#include <atomic>
#include <memory>
#include <chrono>

#include <dr_api.h>

/// max number of individual mutexes per thread
constexpr int MUTEX_MAP_SIZE = 128;

/** Upper limit of process address space according to
*   https://docs.microsoft.com/en-us/windows-hardware/drivers/gettingstarted/virtual-address-spaces
*/
constexpr uint64_t PROC_ADDR_LIMIT = 0x7FF'FFFFFFFF;

/// DRace instrumentation framework
namespace drace {
	/** Runtime parameters */
	struct params_t {
		unsigned sampling_rate{ 1 };
		unsigned instr_rate{ 1 };
		bool     lossy{ false };
		bool     lossy_flush{ false };
		bool     excl_traces{ false };
		bool     excl_stack{ false };
		bool     exclude_master{ false };
		bool     delayed_sym_lookup{ false };
		bool     fastmode{ true };
        /// search for annotations in modules of target application
        bool     annotations{ true };
        unsigned suppression_level{ 1 };
		/** Use external controller */
		bool     extctrl{ false };
		bool     break_on_race{ false };
		unsigned stack_size{ 31 };
		std::string  config_file{ "drace.ini" };
		std::string  out_file;
		std::string  xml_file;
		std::string  logfile{ "stderr" };

		// Raw arguments
		int          argc;
		const char** argv;
	};
	extern params_t params;

	class Statistics;

	/** Per Thread data (thread-private)
	* \warning This struct is not default-constructed
	*          but just allocated as a memory block and casted
	*          Initialisation is done in the thread-creation event
	*          in memory_instr.
	*/
	struct per_thread_t {
		using tls_map_t = std::vector<std::pair<thread_id_t, per_thread_t*>>;

		byte *        buf_ptr;
		ptr_int_t     buf_end;

        /// Represents the detector state.
        byte          enabled{ true };
        /// inverse of flush pending, jmpecxz
        std::atomic<byte> no_flush{ false };
        /// external flush is currently executed;
        std::atomic<byte> external_flush{ false };
        /// bool external change detected
        /// this flag is used to trigger the enable or disable
        /// logic on this thread
        byte enable_external{ true };
        /// local sampling state
        int sampling_pos = 0;

		void         *cache;
		thread_id_t   tid;

		/// Shadow Stack
		AlignedStack<void*, 32> stack;
		/// track state of detector (count nr of disable / enable calls)
		uint64        event_cnt{ 0 };

		/// begin of this threads stack range
		ULONG_PTR appstack_beg{ 0x0 };
		/// end of this threads stack range
		ULONG_PTR appstack_end{ 0x0 };

        /**
         * as the detector cannot allocate TLS,
         * use this ptr for per-thread data in detector */
        void *detector_data{ nullptr };

        /// buffer containing memory accesses
        AlignedBuffer<byte, 64> mem_buf;

		/// Statistics
		std::unique_ptr<Statistics> stats;

        /// book-keeping of active mutexes
        std::unordered_map<uint64_t, unsigned> mutex_book;
        /// Used for event syncronisation procedure
        tls_map_t     th_towait;
	};

	/** Thread local storage */
	extern int      tls_idx;
	extern std::unordered_map<thread_id_t, per_thread_t*> TLS_buckets;
	extern void* tls_rw_mutex;

	// TODO check if global is better
	extern std::atomic<int> num_threads_active;
	extern std::atomic<uint> runtime_tid;
	extern std::atomic<thread_id_t> last_th_start;
	extern std::atomic<bool> th_start_pending;

	// Start time of the application
	extern std::chrono::system_clock::time_point app_start;
	extern std::chrono::system_clock::time_point app_stop;

	// Global Module Shadow Data
	namespace module {
		class Tracker;
	}
	extern std::unique_ptr<module::Tracker> module_tracker;

	// Global mutex to synchronize threads
	extern void* th_mutex;

	class MemoryTracker;
	extern std::unique_ptr<MemoryTracker> memory_tracker;

	class RaceCollector;
	extern std::unique_ptr<RaceCollector> race_collector;

	// Global Configuration
	extern drace::Config config;

	// Global Statistics Collector
	extern std::unique_ptr<Statistics> stats;

} // namespace drace

	// MSR Communication Driver
	namespace ipc {
		template<bool, bool>
		class MtSyncSHMDriver;

		template<typename T, bool>
		class SharedMemory;

		struct ClientCB;
	}

namespace drace {
	extern std::unique_ptr<::ipc::MtSyncSHMDriver<true, true>> shmdriver;
	extern std::unique_ptr<::ipc::SharedMemory<ipc::ClientCB, true>> extcb;
}

// infected by windows.h and clashes with std::min/max
#undef max
#undef min
