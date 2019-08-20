#include <map>
#include "ftstates.h"

constexpr auto READ_SHARED = -2;
static constexpr int VAR_NOT_INIT = -1;
class VarState : FTStates {
public:
    uint64_t address;
    int32_t w_tid;
    int32_t r_tid;
    int32_t w_clock;
    /// local clock of last read
    int32_t r_clock;
    std::map<uint32_t, uint32_t> rvc;

    VarState::VarState(uint64_t addr) {
        address = addr;
        w_clock = r_clock = r_tid = w_tid = VAR_NOT_INIT;
    };

   
    void update(bool is_write, uint32_t vc, uint32_t tid) {
        if (is_write) {
            r_clock = VAR_NOT_INIT;
            r_tid = VAR_NOT_INIT;
            w_tid = tid;
            w_clock = vc;
        }
        else {
            if (r_clock == READ_SHARED) {
                if (rvc.find(tid) == rvc.end()) {
                    rvc.insert(rvc.end(), std::pair<uint32_t, uint32_t>(tid, vc));
                }
                else {
                    rvc[tid] = vc;
                }
            }
            else {
                r_tid = tid;
                r_clock = vc;
            }
        }
    };

    void set_read_shared() {
        r_clock = READ_SHARED;
        r_tid = READ_SHARED;
    }

    uint32_t get_length() {
        return rvc.size();
    }

    uint32_t get_id_by_pos(uint32_t pos) {
        if (pos < rvc.size()) {
            auto it = rvc.begin();
            std::advance(it, pos);
            return it->first;
        }
        else {
            return 0;
        }
    }

    uint32_t get_vc_by_id(uint32_t tid) {
        if (rvc.find(tid) != rvc.end()) {
            return rvc[tid];
        }
        else {
            return 0;
        }
    }

};
