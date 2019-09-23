#include "varstate.h"

VarState::VarState(size_t addr, size_t var_size)
:address(addr),
size(var_size),
w_id(VAR_NOT_INIT),
r_id(VAR_NOT_INIT)
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
    auto var_tid = get_r_tid();
    if (get_read_id() != VAR_NOT_INIT &&
        t->get_tid() != var_tid &&
        get_r_clock() >= t->get_clock_by_tid(var_tid))// read-write race
    {
        return true;
    }
    return false;
}

///evaluates for read-shared/write races through this and and access through t
uint32_t VarState::is_rw_sh_race(std::shared_ptr<ThreadState> t) {
    for (int i = 0; i < vc.size(); ++i) {
        size_t act_id = get_thr(i);
        uint32_t act_tid = VectorClock<>::make_tid(act_id);

        if (act_id != 0            &&
            t->get_tid() != act_tid &&
            VectorClock<>::make_clock(act_id) >= t->get_clock_by_tid(act_tid))
        {
            return act_tid;
        }
    }
    return 0;
}

size_t  VarState::get_write_id() const {
    return w_id;
}

size_t VarState::get_read_id() const {
    return r_id;
}


auto VarState::find_in_vec(size_t tid)  {
    for (auto it = vc.begin(); it != vc.end(); ++it) {
        if (VectorClock<>::make_tid(*it)== tid) {
            return it;
        }
    }

    return vc.end();
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
void VarState::update(bool is_write, size_t id) {
    if (is_write) {
        
        r_id = VAR_NOT_INIT;      
        vc.clear();

        w_id = id;
    }
    else {
        if (vc.size() != 0) {
            auto it = find_in_vec(id);
            if (it != vc.end()) {
                vc.erase(it);
            }
            vc.push_back(id);
        }
        else {
            r_id = id;
        }
    }
}

///sets read state to shared
void VarState::set_read_shared(size_t id)
{   
    vc.push_back(r_id);
    vc.push_back(id);

    r_id = VAR_NOT_INIT;

}

///if in read_shared state, then returns thread id of position pos in vector clock
uint32_t VarState::get_thr(uint32_t pos) const {
    if (pos < vc.size()) {
        auto it = vc.begin();
        std::advance(it, pos);
        return *it;
    }
    else {
        return 0;
    }
}

bool VarState::is_read_shared() const{
    return (vc.size() == 0) ? false : true;
}


///return stored clock value, which belongs to ThreadState t, 0 if not available
size_t VarState::get_vc_by_thr(size_t tid)  {
    auto it = find_in_vec(tid);

    if (it != vc.end()) {
        return *it;
    }
    return 0;
}


size_t VarState::get_clock_by_thr(size_t tid)  {

    auto it = find_in_vec(tid);
    if (it != vc.end()) {
        return VectorClock<>::make_clock(*it);
    }
    return 0;
}
