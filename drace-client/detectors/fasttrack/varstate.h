#include <map>
#include "vectorclock.h"

constexpr auto READ_SHARED = -2;
static constexpr int VAR_NOT_INIT = -1;
class VarState : public VectorClock {
public:
    uint64_t address;
    int32_t w_tid;
    int32_t r_tid;
    int32_t w_clock;
    /// local clock of last read
    int32_t r_clock;
    //std::map<uint32_t, uint32_t> vc;

    VarState::VarState(uint64_t addr) {
        address = addr;
        w_clock = r_clock = r_tid = w_tid = VAR_NOT_INIT;
    };

    bool is_ww_race(ThreadState* t) {
        if (t->tid != w_tid &&
            w_clock >= t->get_vc_by_id(w_tid)) {
            return true;
        }
        return false;
    };

    bool is_wr_race(ThreadState* t) {
        if (w_clock != VAR_NOT_INIT &&
            w_tid != t->tid      &&
            w_clock >= t->get_vc_by_id(w_tid)) {
            return true;
        }
        return false;
    };

    bool is_rw_ex_race(ThreadState* t) {
      
        if (r_tid != VAR_NOT_INIT &&
            t->tid != r_tid      &&
            r_clock >= t->get_vc_by_id(r_tid))// read-write race
        {
            return true;
        }
        return false;
    };

    uint32_t is_rw_sh_race(ThreadState* t) {
        for (int i = 0; i < get_length(); ++i) {
            uint32_t act_tid = get_id_by_pos(i);

            if (t->tid != act_tid &&
                get_vc_by_id(act_tid) >= t->get_vc_by_id(act_tid))
            {
                return act_tid;
            }
        }
        return 0;
    }
    


    void update(bool is_write, uint32_t clock, uint32_t tid) {
        if (is_write) {
            r_clock = VAR_NOT_INIT;
            r_tid = VAR_NOT_INIT;
            w_tid = tid;
            w_clock = clock;
        }
        else {
            if (r_clock == READ_SHARED) {
                if (vc.find(tid) == vc.end()) {
                    vc.insert(vc.end(), std::pair<uint32_t, uint32_t>(tid, clock));
                }
                else {
                    vc[tid] = clock;
                }
            }
            else {
                r_tid = tid;
                r_clock = clock;
            }
        }
    };

    void set_read_shared() {
        r_clock = READ_SHARED;
        r_tid = READ_SHARED;
    }

};
