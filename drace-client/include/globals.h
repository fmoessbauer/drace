#pragma once

// Log up to notice
#ifndef LOGLEVEL
#define LOGLEVEL 3
#endif

#include "config.h"
#include "aligned-stack.h"
#include "memory-tracker.h"

#include <string>
#include <unordered_map>
#include <atomic>
#include <memory>
#include <chrono>

#include <dr_api.h>

/// max number of individual mutexes per thread
#define MUTEX_MAP_SIZE 128

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
		bool     yield_on_evt{ false };
		bool     fastmode{ false };
		/** Use external controller */
		bool     extctrl{ false };
		bool     break_on_race{ false };
		unsigned stack_size{ 10 };
		std::string  config_file{ "drace.ini" };
		std::string  out_file;
		std::string  xml_file;

		// Raw arguments
		int          argc;
		const char** argv;
	};
	extern params_t params;

	class Statistics;
	class ThreadState;

	/** Thread local storage */
	extern int      tls_idx;
	extern std::unordered_map<thread_id_t, ThreadState*> TLS_buckets;
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

	class Instrumentator;
	extern std::unique_ptr<Instrumentator> instrumentator;

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