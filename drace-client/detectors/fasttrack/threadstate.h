#pragma once
#ifndef THREADSTATE_H
#define THREADSTATE_H


#include <string>
#include <vector>
#include "vectorclock.h"
#include "xmap.h"
#include "xvector.h"

namespace drace {
    namespace detector{
        class Fasttrack;
    }
}


class ThreadState : public VectorClock{
private:

    ///contains the current stack trace of the thread
    xvector<size_t> global_stack;
    ///contains 'historic' stack traces of a r/w of a variable
    xmap<size_t, xvector<size_t>> read_write_traces;

    drace::detector::Fasttrack* ft = nullptr;

public:
    ///own thread id
    uint32_t tid;
       
    ///constructor of ThreadState object, initializes tid and clock, copies the vector of parent thread, if a parent thread exists
    ThreadState::ThreadState(drace::detector::Fasttrack* ft_inst,  uint32_t own_tid, std::shared_ptr<ThreadState> parent = nullptr) {
        tid = own_tid;
        ft = ft_inst;
        uint32_t clock = 0;

        vc.insert(vc.end(), std::pair<uint32_t, uint32_t>(tid, clock));
        if (parent != nullptr) {
            //if parent exists vector clock
            vc = parent->vc;
        }
    }

    ThreadState::~ThreadState() {
        ///TODO bring this to work!!!
        //ft->cleanup(tid);
    }

    void pop_stack_element() {
       global_stack.pop_back();
    }

    void push_stack_element(size_t element) {
        global_stack.push_back(element);
    }

    ///when a var is written or read, it copies the stack and adds the pc of the
    ///r/w operation to be able to return the stack trace if a race was detected
    void set_read_write(size_t addr, size_t pc) {
        auto copy_stack(global_stack);
        copy_stack.push_back(pc);

        if (read_write_traces.find(addr) == read_write_traces.end()) {
            read_write_traces.insert(read_write_traces.end(), { addr, copy_stack });
        }
        else {
            read_write_traces.find(addr)->second = copy_stack;
        }
    }

    ///increases own clock value
    void inc_vc() {
        vc[tid]++;
    }

    ///returns own clock value
    uint32_t get_self() {
        return vc[tid];
    };

    ///returns a stack trace of a clock for handing it over to drace
    xvector<size_t> return_stack_trace(size_t address){//bool is_alloc) {
       
        if (read_write_traces.find(address) != read_write_traces.end()) {
            return read_write_traces.find(address)->second;
        }
        else {
            //A read/write operation was not tracked correctly -> return empty stack trace
            return xvector<size_t>(0);
        }

    }

};
#endif // !THREADSTATE_H
