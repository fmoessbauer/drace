#pragma once

#include <dr_api.h>

enum {
	REF_TYPE_READ = 0,
	REF_TYPE_WRITE = 1,
};

typedef struct _mem_ref_t {
	ushort type; /* r(0), w(1), or opcode (assuming 0/1 are invalid opcode) */
	ushort size; /* mem ref size or instr length */
	app_pc addr; /* mem ref addr or instr pc */
} mem_ref_t;

static std::atomic<int> num_threads_active;

static void event_exit(void);
static dr_emit_flags_t event_basic_block(void *drcontext, void *tag, instrlist_t *bb, bool for_trace, bool translating);
static void event_thread_init(void *drcontext);
static void event_thread_exit(void *drcontext);
