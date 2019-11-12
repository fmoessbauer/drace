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
#include "threadstate.h"
#include "fasttrack.h"


ThreadState::ThreadState(drace::detector::Fasttrack<_mutex>* ft_inst, VectorClock::TID own_tid, std::shared_ptr<ThreadState> parent)
:ft(ft_inst),
id(VectorClock::make_id(own_tid))
{
    vc.insert({ own_tid, id });
    if (parent != nullptr) {
        //if parent exists vector clock
        vc = parent->vc;
    }
}

ThreadState::~ThreadState() {
    uint32_t tid = VectorClock::make_tid(id);

    ft->cleanup(tid);
    ft = nullptr;
}


void ThreadState::inc_vc() {
    id++; //as the lower 32 bits are clock just increase it by ine
    vc[VectorClock::make_tid(id)] = id;
}

VectorClock<>::VC_ID ThreadState::return_own_id() const{
    return id;
};

VectorClock<>::TID ThreadState::get_tid() const {
    return VectorClock::make_tid(id);
}

VectorClock<>::Clock ThreadState::get_clock() const {
    return VectorClock::make_clock(id);
}


