#ifndef THREADSTATE_H
#define THREADSTATE_H

#include <vector>
#include <memory>
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
    size_t tid;
       
    ///constructor of ThreadState object, initializes tid and clock, copies the vector of parent thread, if a parent thread exists
    ThreadState::ThreadState(   drace::detector::Fasttrack* ft_inst,
                                size_t own_tid, std::shared_ptr<ThreadState> parent = nullptr); 

    ThreadState::~ThreadState(); 

    void pop_stack_element(); 

    void push_stack_element(size_t element);

    ///when a var is written or read, it copies the stack and adds the pc of the
    ///r/w operation to be able to return the stack trace if a race was detected
    void set_read_write(size_t addr, size_t pc);

    ///increases own clock value
    void inc_vc();

    ///returns own clock value
    uint32_t get_self();

    ///returns a stack trace of a clock for handing it over to drace
    xvector<size_t> return_stack_trace(size_t address);

};
#endif // !THREADSTATE_H
