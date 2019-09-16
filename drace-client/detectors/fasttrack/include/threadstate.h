#ifndef THREADSTATE_H
#define THREADSTATE_H

#include <vector>
#include <memory>
#include "vectorclock.h"
#include "xmap.h"
#include "xvector.h"
#include <atomic>

namespace drace {
    namespace detector {
        class Fasttrack;
    }
}


class ThreadState : public VectorClock {
private:


    ///contains the current stack trace of the thread
    //xvector<size_t> global_stack;
    ///contains 'historic' stack traces of a r/w of a variable
    //xmap<size_t, xvector<size_t>> read_write_traces;
    //xmap<size_t, size_t> read_write_traces;

    drace::detector::Fasttrack* ft = nullptr;

public:
    ///own thread id
    std::atomic<uint64_t> id;
       
    ///constructor of ThreadState object, initializes tid and clock, copies the vector of parent thread, if a parent thread exists
    ThreadState::ThreadState(   drace::detector::Fasttrack* ft_inst,
                                uint32_t own_tid, std::shared_ptr<ThreadState> parent = nullptr); 

    ThreadState::~ThreadState(); 


    ///increases own clock value
    void inc_vc();

    ///returns own clock value (upper bits) & thread (lower bits)
    size_t return_own_id() const;

    uint32_t get_tid() const;

    uint32_t get_clock() const;



};

#endif // !THREADSTATE_H
