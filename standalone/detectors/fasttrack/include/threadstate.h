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
#ifndef THREADSTATE_H
#define THREADSTATE_H


#include <memory>
#include "vectorclock.h"
#include "xvector.h"
#include <atomic>

///implements a threadstate, holds the thread's vectorclock and the thread's id (tid and act clock), as well as pointer to the fasttrack class
class ThreadState : public VectorClock<>{//tl_alloc<std::pair<const size_t, size_t>>> {
private:

    ///holds the tid and the actual clock value -> lower 32 bits are clock, upper 32 are the tid
    std::atomic<VectorClock::VC_ID> id;

public:
       
    ///constructor of ThreadState object, initializes tid and clock
    ///copies the vector of parent thread, if a parent thread exists
    ThreadState::ThreadState(/*drace::detector::Fasttrack<_mutex>* ft_inst,*/
                            VectorClock::TID own_tid,
                            std::shared_ptr<ThreadState> parent = nullptr);

    ///increases own clock value
    void inc_vc();

    ///returns own clock value (upper bits) & thread (lower bits)
    VectorClock::VC_ID return_own_id() const;

    ///returns thread id
    VectorClock::TID get_tid() const;

    ///returns current clock
    VectorClock::Clock get_clock() const;

    ///may be called after exitting of thread
    void delete_vector();

};

#endif // !THREADSTATE_H
