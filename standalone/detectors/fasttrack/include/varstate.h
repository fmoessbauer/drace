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
#ifndef VARSTATE_H
#define VARSTATE_H

#include "vectorclock.h"
#include "threadstate.h"
#include <atomic>
#include <memory>
#include "xvector.h"

class VarState  {

    ///contains read_shared case all involved threads and clocks
    std::unique_ptr <xvector<size_t >> shared_vc = nullptr;
    //xvector<size_t> vc;


    /// the upper half of the bits are the thread id the lower half is the clock of the thread
    std::atomic<VectorClock<>::VC_ID> w_id;
    /// local clock of last read
    std::atomic<VectorClock<>::VC_ID> r_id;

    ///finds the entry with the tid in the shared vectorclock
    auto find_in_vec(VectorClock<>::TID tid) ;

public:

    //static constexpr int READ_SHARED = -2;
    static constexpr int VAR_NOT_INIT = 0;


    ///var address
    const size_t address;

    /// var size //TO DO make smaller
    const uint16_t size;

    VarState(size_t addr, uint16_t var_size);

    ///evaluates for write/write races through this and and access through t
    bool is_ww_race(std::shared_ptr<ThreadState> t);

    ///evaluates for write/read races through this and and access through t
    bool is_wr_race(std::shared_ptr<ThreadState> t);

    ///evaluates for read-exclusive/write races through this and and access through t
    bool is_rw_ex_race(std::shared_ptr<ThreadState> t);

    ///evaluates for read-shared/write races through this and and access through t
    VectorClock<>::TID is_rw_sh_race(std::shared_ptr<ThreadState> t);

    ///returns id of last write access
    VectorClock<>::VC_ID get_write_id() const;

    ///returns id of last read access (when read is not shared)
    VectorClock<>::VC_ID get_read_id() const;

    ///return tid of thread which last wrote this var
    VectorClock<>::TID get_w_tid() const;

    ///return tid of thread which last read this var, if not read shared
    VectorClock<>::TID get_r_tid() const;

    ///returns clock value of thread of last write access
    VectorClock<>::Clock  get_w_clock() const;

    ///returns clock value of thread of last read access (returns 0 when read is shared)
    VectorClock<>::Clock  get_r_clock() const;

    ///returns true when read is shared
    bool is_read_shared() const;

    ///updates the var state because of an new read or write access through an thread
    void update(bool is_write, size_t id);

    ///sets read state to shared
    void set_read_shared(size_t id);

    ///if in read_shared state, then returns id of position pos in vector clock
    VectorClock<>::VC_ID get_sh_id(uint32_t pos) const;

    ///return stored clock value, which belongs to ThreadState t, 0 if not available
    VectorClock<>::VC_ID get_vc_by_thr(VectorClock<>::TID t) ;

    VectorClock<>::Clock  get_clock_by_thr(VectorClock<>::TID t) ;


};
#endif // !VARSTATE_H
