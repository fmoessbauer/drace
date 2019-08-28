#ifndef VECTORCLOCK_H
#define VECTORCLOCK_H
#include "xmap.h"

/**
    Implements a VectorClock.
    Can hold arbitrarily much pairs of a Thread Id and the belonging clock
*/


class VectorClock {


public:
    ///vector clock which contains multiple thread ids, clocks
    xmap<uint32_t, uint32_t> vc;

 
    ///return the thread id of the position pos of the vector clock
    uint32_t get_thr(uint32_t pos) {
        if (pos < vc.size()) {
            auto it = vc.begin();
            std::advance(it, pos);
            return it->first;
        }
        else {
            return 0;
        }
    };

    ///returns the no. of elements of the vector clock
    uint32_t get_length() {
        return vc.size();
    };

    ///updates this.vc with values of other.vc, if they're larger -> one way update
    void update(VectorClock* other) {
        for (auto it = other->vc.begin(); it != other->vc.end(); it++)
        {
            if (it->second > get_vc_by_id(it->first)) {
                update(it->first, it->second);
            }
        }
    };

    ///updates vector clock entry or creates entry if non-existant
    void update(uint32_t id, uint32_t clock) {
        if (vc.find(id) == vc.end()) {
            vc.insert(vc.end(), std::pair<uint32_t, uint32_t>(id, clock));
        }
        else {
            if (vc[id] < clock) { vc[id] = clock; }
        }
    };

    ///deletes a vector clock entry, checks existance before
    void delete_vc(uint32_t tid) {
        if (vc.find(tid) != vc.end())
            vc.erase(tid);
    }

    ///returns known vector clock of tid
    uint32_t get_vc_by_id(uint32_t tid) {
        if (vc.find(tid) != vc.end()) {
            return vc[tid];
        }
        else {
            return 0;
        }
    };

};

#endif
