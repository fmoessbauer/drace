#include "threadstate.h"
#include "fasttrack.h"


ThreadState::ThreadState(drace::detector::Fasttrack* ft_inst, uint32_t own_tid, std::shared_ptr<ThreadState> parent)
:ft(ft_inst),
id(VectorClock::make_id(own_tid))
{
    vc.insert(vc.end(), { own_tid, id });
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

size_t ThreadState::return_own_id() const{
    return id;
};

uint32_t ThreadState::get_tid() const {
    return VectorClock::make_tid(id);
}

uint32_t ThreadState::get_clock() const {
    return VectorClock::make_clock(id);
}
