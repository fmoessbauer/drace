#pragma once

#include "memory-instr.h"
#include "module-tracker.h"
#include <dr_api.h>
#include <set>
#include <stack>
#include <string>

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
	unsigned int sampling_rate;
	bool         heap_only;
	bool         frequent_only;
	bool         exclude_master;
};
extern params_t params;

using event_stack_t = std::stack<std::string>;
/* Per Thread data (thread-private)*/
typedef struct {
	byte         *buf_ptr;
	byte         *buf_base;
	ptr_int_t     buf_end;
	void         *cache;
	uint64        last_alloc_size;
	uint64        num_refs;
	thread_id_t   tid;
	uint64        grace_period;
	// for lea trick use 64bit
	uint64        disabled;
	// Stack used to track state of detector
	event_stack_t event_stack;
} per_thread_t;

/* Global data structures */
extern reg_id_t tls_seg;
extern uint     tls_offs;
extern int      tls_idx;

// TODO check if global is better
static std::atomic<int> num_threads_active{ 0 };
extern std::atomic<uint> runtime_tid;

// Module
using mod_t = module_tracker::module_info_t;
extern std::set<mod_t, std::greater<mod_t>> modules;
