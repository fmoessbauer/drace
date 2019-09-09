#include "threadstate.h"
#include "fasttrack.h"


ThreadState::ThreadState(drace::detector::Fasttrack* ft_inst, size_t own_tid, std::shared_ptr<ThreadState> parent)
:ft(ft_inst),
tid(own_tid)
{
    uint32_t clock = 0;

    vc.insert(vc.end(), std::pair<size_t, uint32_t>(tid, clock));
    if (parent != nullptr) {
        //if parent exists vector clock
        vc = parent->vc;
    }
}

ThreadState::~ThreadState() {
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
    vc[tid]++;
}

uint32_t ThreadState::get_self() {
    return vc[tid];
};

xvector<size_t> ThreadState::return_stack_trace(size_t address) {

    if (read_write_traces.find(address) != read_write_traces.end()) {
        return read_write_traces.find(address)->second;
    }
    else {
        //A read/write operation was not tracked correctly -> return empty stack trace
        return xvector<size_t>(0);
    }

}
