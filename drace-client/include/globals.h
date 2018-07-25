#pragma once

#include "config.h"

#include <string>
#include <unordered_map>
#include <atomic>
#include <memory>

#include <dr_api.h>
#include <drvector.h>

// Defines
#define GRACE_PERIOD_TH_START 0 // Begin detection after this number of instructions

/* Runtime parameters */
struct params_t {
	unsigned int sampling_rate{ 1 };
	bool         heap_only{ false };
	bool         frequent_only{ false };
	bool         exclude_master{ false };
	bool         delayed_sym_lookup{ false };
	bool         yield_on_evt{ false };
	bool         fastmode{ false };
	std::string  config_file{"drace.ini"};
	std::string  out_file;
	std::string  xml_file;
};
extern params_t params;

class Statistics;

/* Per Thread data (thread-private)
* \Warning This struct is not default-constructed
*          but just allocated as a memory block and casted
*          Initialisation is done in the thread-creation event
*          in memory_instr.
*/
struct per_thread_t {
	using mutex_map_t = std::unordered_map<uint64_t, int>;
	using tls_map_t = std::vector<std::pair<thread_id_t, per_thread_t*>>;

	byte         *buf_ptr;
	byte         *buf_base;
	ptr_int_t     buf_end;
	void         *cache;
	thread_id_t   tid;
	uint64        grace_period{ GRACE_PERIOD_TH_START };
	// use ptrsize type for lea
	ptr_uint_t    enabled{ true };
	// inverse of flush pending, jmpecxz
	std::atomic<ptr_uint_t> no_flush{ false };
	// external flush is currently executed;
	std::atomic<bool> external_flush{ false };
	// Shadow Stack
	drvector_t    stack;
	// Stack used to track state of detector
	uint64        event_cnt{ 0 };
	// book-keeping of active mutexes
	mutex_map_t   mutex_book;
	// Used for event syncronisation procedure
	tls_map_t    th_towait;
	// Statistics
	std::unique_ptr<Statistics>   stats;
	// as the detector cannot allocate TLS,
	// use this ptr for per-thread data in detector
	void         *detector_data{nullptr};
};

/* Global data structures */
extern reg_id_t tls_seg;
extern uint     tls_offs;
extern int      tls_idx;

// TODO check if global is better
extern std::atomic<int> num_threads_active;
extern std::atomic<uint> runtime_tid;
extern std::atomic<thread_id_t> last_th_start;
extern std::atomic<bool> th_start_pending;

// Global Module Shadow Data
class ModuleTracker;
extern std::unique_ptr<ModuleTracker> module_tracker;

// TLS
extern std::unordered_map<thread_id_t, per_thread_t*> TLS_buckets;
extern void* tls_rw_mutex;

// Global mutex to synchronize threads
extern void* th_mutex;

class MemoryTracker;
extern std::unique_ptr<MemoryTracker> memory_tracker;

// Global Symbol Table
class Symbols;
extern std::unique_ptr<Symbols> symbol_table;

class RaceCollector;
extern std::unique_ptr<RaceCollector> race_collector;

// Global Configuration
extern drace::Config config;

// Global Statistics Collector
extern std::unique_ptr<Statistics> stats;