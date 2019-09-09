#ifndef VARSTATE_H
#define VARSTATE_H

#include "vectorclock.h"
#include "xmap.h"
#include "threadstate.h"
#include <memory>

class VarState  {

    ///pointer to thread which last wrote the var
    std::shared_ptr<ThreadState> w_tid;

    ///pointer to thread which last read the var, if not read_shared
    std::shared_ptr<ThreadState> r_tid;

    ///contains read_shared case all involved threads and clocks
    std::unique_ptr <xmap< std::shared_ptr<ThreadState>, uint32_t >> vc;
   

public:
    static constexpr int READ_SHARED = -2;
    static constexpr int VAR_NOT_INIT = 0;

    ///var address
    size_t address;

    /// var size
    uint32_t size;

    /// local clock of last write
    int32_t w_clock;

    /// local clock of last read
    int32_t r_clock;

    VarState::VarState(size_t addr, uint32_t var_size);


    ///evaluates for write/write races through this and and access through t
    bool is_ww_race(std::shared_ptr<ThreadState> t);

    ///evaluates for write/read races through this and and access through t
    bool is_wr_race(std::shared_ptr<ThreadState> t);

    ///evaluates for read-exclusive/write races through this and and access through t
    bool is_rw_ex_race(std::shared_ptr<ThreadState> t);

    ///evaluates for read-shared/write races through this and and access through t
    std::shared_ptr <ThreadState> is_rw_sh_race(std::shared_ptr<ThreadState> t);

    ///return tid of thread which last wrote this var
    size_t get_w_tid();

    ///return tid of thread which last read this var, if not read shared
    size_t get_r_tid();

    ///updates the var state because of an new read or write access through an thread
    void update(bool is_write, std::shared_ptr<ThreadState> thread);

    ///sets read state to shared
    void set_read_shared(std::shared_ptr<ThreadState> thread);

    ///if in read_shared state, then returns thread id of position pos in vector clock
    std::shared_ptr<ThreadState> get_thr(uint32_t pos);

    ///return stored clock value, which belongs to ThreadState t, 0 if not available
    uint32_t get_vc_by_id(std::shared_ptr<ThreadState> t);

};
#endif // !VARSTATE_H
