#pragma once

#include "memory-instr.h"
#include <dr_api.h>

#include <tsan-custom.h>

// memory instrumentator
static std::atomic<int> num_threads_active;

static void event_exit(void);
static dr_emit_flags_t event_basic_block(void *drcontext, void *tag, instrlist_t *bb, bool for_trace, bool translating);
static void event_thread_init(void *drcontext);
static void event_thread_exit(void *drcontext);
static void module_load_event(void *drcontext, const module_data_t *mod, bool loaded);

// Detector Stuff
void reportRaceCallBack(__tsan_race_info* raceInfo, void* callback_parameter);

