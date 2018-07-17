#pragma once

#include "config.h"
#include "memory-instr.h"
#include "module-tracker.h"
#include <dr_api.h>
#include <set>
#include <map>
#include <unordered_map>
#include <stack>
#include <string>

// Defines
#define GRACE_PERIOD_TH_START 20 // Begin detection after this number of instructions

// Events
static void event_exit(void);
static dr_emit_flags_t event_basic_block(void *drcontext, void *tag, instrlist_t *bb, bool for_trace, bool translating);
static void event_thread_init(void *drcontext);
static void event_thread_exit(void *drcontext);
static void module_load_event(void *drcontext, const module_data_t *mod, bool loaded);

// Runtime Configuration
static void parse_args(int argc, const char **argv);
static void print_config();

/* Runtime parameters */
struct params_t {
	unsigned int sampling_rate{ 1 };
	bool         heap_only{ false };
	bool         frequent_only{ false };
	bool         exclude_master{ false };
	bool         delayed_sym_lookup{ false };
	bool         yield_on_evt{ false };
};
extern params_t params;

using event_stack_t = std::stack<std::string>;
using mutex_map_t = std::unordered_map<uint64_t, int>;
/* Per Thread data (thread-private)
 * \Warning This struct is not default-constructed
 *          but just allocated as a memory block and casted
 *          Initialisation is done in the thread-creation event
 *          in memory_instr.
 */
typedef struct {
	byte         *buf_ptr;
	byte         *buf_base;
	ptr_int_t     buf_end;
	void         *cache;
	uint64        num_refs;
	thread_id_t   tid;
	uint64        grace_period;
	// use ptrsize type for lea
	uint64        enabled;
	// inverse of flush pending, jmpecxz
	std::atomic<uint64> no_flush;
	// Stack used to track state of detector
	uint64        event_cnt;
	// book-keeping of active mutexes
	mutex_map_t   mutex_book;
	// Mutex statistics
	uint64        mutex_ops;

	// as the detector cannot allocate TLS,
	// use this ptr for per-thread data in detector
	void         *detector_data;
} per_thread_t;

/* Global data structures */
extern reg_id_t tls_seg;
extern uint     tls_offs;
extern int      tls_idx;

// TODO check if global is better
static std::atomic<int> num_threads_active{ 0 };
extern std::atomic<uint> runtime_tid;
extern std::atomic<uint> last_th_start;
extern std::atomic<bool> th_start_pending;

// Module
using mod_t = module_tracker::module_info_t;
extern std::set<mod_t, std::greater<mod_t>> modules;

// TLS
extern std::unordered_map<thread_id_t, per_thread_t*> TLS_buckets;

// Global mutex to synchronize threads
extern void* th_mutex;
// do not start C++11 threads concurrently
extern void* th_start_mutex;

// Global Configuration
extern drace::Config config;

/* DRace Client Functions */
void flush_all_threads(per_thread_t * data);