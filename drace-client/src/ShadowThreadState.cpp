#include "ShadowThreadState.h"
#include "globals.h"
#include "statistics.h"

namespace drace {
ShadowThreadState::ShadowThreadState(void* drcontext)
    : tid(dr_get_thread_id(drcontext)), stats(tid) {
  hashtable_init(&mutex_book, 8, HASH_INTPTR, false);
  // set first sampling period
  sampling_pos = params.sampling_rate;
  stack.bindDetector(detector.get());
}

ShadowThreadState::~ShadowThreadState() { hashtable_delete(&mutex_book); }
}  // namespace drace
