#include "varstate.h"

VarState::VarState(size_t addr, size_t var_size)
:address(addr),
size(var_size),
w_id(VAR_NOT_INIT),
r_id(VAR_NOT_INIT),
w_tid(nullptr),
r_tid(nullptr)
{}


///evaluates for write/write races through this and and access through t
bool VarState::is_ww_race(std::shared_ptr<ThreadState> t) {
    if (t->get_tid() != get_w_tid() &&
        get_w_clock() >= t->get_clock_by_tid(get_w_tid())) {
        return true;
    }
    return false;
}

///evaluates for write/read races through this and and access through t
bool VarState::is_wr_race(std::shared_ptr<ThreadState> t) {
    if (get_write_id() != VAR_NOT_INIT &&
        get_w_tid() != t->get_tid()       &&
        get_w_clock() >= t->get_clock_by_tid(get_w_tid()))
    {
        return true;
    }
    return false;
}

///evaluates for read-exclusive/write races through this and and access through t
bool VarState::is_rw_ex_race(std::shared_ptr<ThreadState> t) {

    if (get_read_id() != VAR_NOT_INIT &&
        t->get_tid() != get_r_tid() &&
        get_r_clock() >= t->get_clock_by_tid(get_r_tid()))// read-write race
    {
        return true;
    }
    return false;
}

///evaluates for read-shared/write races through this and and access through t
std::shared_ptr <ThreadState> VarState::is_rw_sh_race(std::shared_ptr<ThreadState> t) {
    for (int i = 0; i < vc->size(); ++i) {
        std::shared_ptr<ThreadState> act_thr = get_thr(i);

        if (t->get_tid() != act_thr->get_tid() &&
            get_clock_by_thr(act_thr) >= t->get_clock_by_tid(act_thr->get_tid()))
        {
            return act_thr;
        }
    }
    return nullptr;
}

size_t  VarState::get_write_id() const {
    return w_id;
}

size_t VarState::get_read_id() const {
    return r_id;
}


///return tid of thread which last wrote this var
size_t VarState::get_w_tid() const {
    size_t mask;
    if (sizeof(size_t) == 8) {
        mask = VectorClock::bit_mask_64;
    }
/*    if (sizeof(size_t) == 4) {
        mask = bit_mask_32;
    }*/
    return w_id / ~(mask);
}

///return tid of thread which last read this var, if not read shared
size_t VarState::get_r_tid() const{
    size_t mask;
    if (sizeof(size_t) == 8) {
        mask = VectorClock::bit_mask_64;
    }
 /*   if (sizeof(size_t) == 4) {
        mask = bit_mask_32;
    }*/
    return r_id / ~(mask);
}

size_t VarState::get_w_clock() const {
    size_t mask;
    if (sizeof(size_t) == 8) {
        mask = VectorClock::bit_mask_64;
    }
    return w_id % mask;
}
size_t VarState::get_r_clock() const {
    size_t mask;
    if (sizeof(size_t) == 8) {
        mask = VectorClock::bit_mask_64;
    }
    return r_id % mask;
}

///updates the var state because of an new read or write access through an thread
void VarState::update(bool is_write, std::shared_ptr<ThreadState> thread) {
    if (is_write) {
        read_shared = false;
        r_id = VAR_NOT_INIT;
        r_tid = nullptr;

        vc.reset();
        w_tid = thread;
        w_id = thread->return_own_id();

    }
    else {
        if (read_shared) {
            if (vc->find(thread) == vc->end()) {
                vc->insert(vc->end(), { thread, thread->return_own_id() });
            }
            else {
                (*vc)[thread] = thread->return_own_id();
            }
        }
        else {
            r_tid = thread;
            r_id = thread->return_own_id();
        }
    }
}

///sets read state to shared
void VarState::set_read_shared(std::shared_ptr<ThreadState> thread) {
    vc = std::make_unique<xmap<std::shared_ptr<ThreadState>, size_t>>();


    vc->insert(vc->end(), { r_tid, r_id });
    vc->insert(vc->end(), { thread, thread->return_own_id() });

    read_shared = true;
    r_id = VAR_NOT_INIT;
    r_tid = nullptr;
}

///if in read_shared state, then returns thread id of position pos in vector clock
std::shared_ptr<ThreadState> VarState::get_thr(uint32_t pos) const {
    if (pos < vc->size()) {
        auto it = vc->begin();
        std::advance(it, pos);
        return it->first;
    }
    else {
        return nullptr;
    }
}

bool VarState::is_read_shared() const{
    return read_shared;
}


///return stored clock value, which belongs to ThreadState t, 0 if not available
size_t VarState::get_vc_by_thr(std::shared_ptr<ThreadState> t) const {
    if (vc->find(t) != vc->end()) {
        return (*vc)[t];
    }
    return 0;
}


size_t VarState::get_clock_by_thr(std::shared_ptr<ThreadState> t) const {
    if (vc->find(t) != vc->end()) {
        return (*vc)[t] % VectorClock::bit_mask_64;
    }
    return 0;
}
