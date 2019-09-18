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
    auto var_tid = get_w_tid();
    if (get_write_id() != VAR_NOT_INIT &&
        var_tid != t->get_tid()       &&
        get_w_clock() >= t->get_clock_by_tid(var_tid))
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

        if (act_thr != nullptr                  &&
            t->get_tid() != act_thr->get_tid()  &&
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


auto VarState::find_in_vec(std::shared_ptr<ThreadState>  t) const {
    for (auto it = vc->begin(); it != vc->end(); ++it) {
        if (it->first == t) {
            return it;
        }
    }

    return vc->end();
}


///return tid of thread which last wrote this var
size_t VarState::get_w_tid() const {   
    return VectorClock<>::make_tid(w_id);
}

///return tid of thread which last read this var, if not read shared
size_t VarState::get_r_tid() const{
    return VectorClock<>::make_tid(r_id);
}

size_t VarState::get_w_clock() const {
    return VectorClock<>::make_clock(w_id);
}

size_t VarState::get_r_clock() const {
    return VectorClock<>::make_clock(r_id);
}

///updates the var state because of an new read or write access through an thread
void VarState::update(bool is_write, std::shared_ptr<ThreadState> thread) {
    if (is_write) {
        
        r_id = VAR_NOT_INIT;
        r_tid = nullptr;

        vc.reset();
        vc = nullptr;

        w_tid = thread;
        w_id = thread->return_own_id();

    }
    else {
        if (vc != nullptr) {
            auto it = find_in_vec(thread);
            if (it == vc->end()) {
                vc->insert(it, { thread, thread->return_own_id() });
            }
            else {
                it->second = thread->return_own_id();
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
    vc = std::make_unique<xvector<std::pair<std::shared_ptr<ThreadState>, size_t>>>();


    vc->push_back({ r_tid, r_id });
    vc->push_back({ thread, thread->return_own_id() });

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
    return (vc == nullptr) ? false : true;
}


///return stored clock value, which belongs to ThreadState t, 0 if not available
size_t VarState::get_vc_by_thr(std::shared_ptr<ThreadState> t) const {
    auto it = find_in_vec(t);
    if (it != vc->end()) {
        return it->second;
    }
    return 0;
}


size_t VarState::get_clock_by_thr(std::shared_ptr<ThreadState> t) const {

    auto it = find_in_vec(t);
    if (it != vc->end()) {
        return VectorClock<>::make_clock(it->second);
    }
    return 0;
}
