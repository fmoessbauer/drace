#include "threadstate.h"
#include "fasttrack.h"


ThreadState::ThreadState(drace::detector::Fasttrack* ft_inst, uint32_t own_tid, std::shared_ptr<ThreadState> parent)
:ft(ft_inst)
{
    id = own_tid << 0xFFFFFFFFULL ;

    
    if (parent != nullptr) {
        //if parent exists vector clock
        vc = parent->vc;
    }
}

ThreadState::~ThreadState() {
    uint32_t tid = id & VectorClock::bit_mask_64;
    ft->cleanup(tid);
}

void ThreadState::pop_stack_element() {
    global_stack.pop_back();
}


void ThreadState::push_stack_element(size_t element) {
    global_stack.push_back(element);
}

void ThreadState::set_read_write(size_t addr, size_t pc) {
    auto copy_stack(global_stack);
    copy_stack.push_back(pc);

    if (read_write_traces.find(addr) == read_write_traces.end()) {
        read_write_traces.insert(read_write_traces.end(), { addr, copy_stack });
    }
    else {
        read_write_traces.find(addr)->second = copy_stack;
    }
}

void ThreadState::inc_vc() {
    id++; //as the lower 32 bits are clock just increase it by ine
}

size_t ThreadState::return_own_id() const{
    return id;
};

uint32_t ThreadState::get_tid() const {
    return id / ~(VectorClock::bit_mask_64);
}

uint32_t ThreadState::get_clock() const {
    return id % VectorClock::bit_mask_64;
}


xvector<size_t> ThreadState::return_stack_trace(size_t address) {

    if (read_write_traces.find(address) != read_write_traces.end()) {
        return read_write_traces.find(address)->second;
    }
    else {
        //A read/write operation was not tracked correctly -> return empty stack trace
        return xvector<size_t>(0);
    }

}
