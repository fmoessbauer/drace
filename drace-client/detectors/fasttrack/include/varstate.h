#ifndef VARSTATE_H
#define VARSTATE_H

#include "vectorclock.h"
#include "xmap.h"
#include "threadstate.h"
#include <atomic>
//#include <memory>

class VarState  {

    ///pointer to thread which last wrote the var
    //std::shared_ptr<ThreadState> w_tid;

    ///pointer to thread which last read the var, if not read_shared
    //std::shared_ptr<ThreadState> r_tid;

    ///contains read_shared case all involved threads and clocks
    std::unique_ptr <xvector<size_t >> vc = nullptr;
    //xvector<size_t> vc;


    /// the upper half of the bits are the thread id the lower half is the clock of the thread
    std::atomic<size_t> w_id;
    /// local clock of last read
    std::atomic<size_t> r_id;

    auto find_in_vec(size_t tid) ;

public:

    //static constexpr int READ_SHARED = -2;
    static constexpr int VAR_NOT_INIT = 0;


    ///var address
    const size_t address;

    /// var size //TO DO make smaller
    const uint16_t size;

    VarState::VarState(size_t addr, size_t var_size);

    ///evaluates for write/write races through this and and access through t
    bool is_ww_race(std::shared_ptr<ThreadState> t);

    ///evaluates for write/read races through this and and access through t
    bool is_wr_race(std::shared_ptr<ThreadState> t);

    ///evaluates for read-exclusive/write races through this and and access through t
    bool is_rw_ex_race(std::shared_ptr<ThreadState> t);

    ///evaluates for read-shared/write races through this and and access through t
    uint32_t is_rw_sh_race(std::shared_ptr<ThreadState> t);

    size_t get_write_id() const;

    size_t get_read_id() const;

    ///return tid of thread which last wrote this var
    uint32_t get_w_tid() const;

    ///return tid of thread which last read this var, if not read shared
    uint32_t get_r_tid() const;

    uint32_t get_w_clock() const;

    uint32_t get_r_clock() const;

    bool is_read_shared() const;

    ///updates the var state because of an new read or write access through an thread
    void update(bool is_write, size_t id);

    ///sets read state to shared
    void set_read_shared(size_t id);

    ///if in read_shared state, then returns id of position pos in vector clock
    size_t get_sh_id(uint32_t pos) const;

    ///return stored clock value, which belongs to ThreadState t, 0 if not available
    size_t get_vc_by_thr(size_t t) ;

    uint32_t get_clock_by_thr(size_t t) ;


};
#endif // !VARSTATE_H
