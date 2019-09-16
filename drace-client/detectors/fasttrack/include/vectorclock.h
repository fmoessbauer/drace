#ifndef VECTORCLOCK_H
#define VECTORCLOCK_H

#include "xmap.h"
#include <memory>
/**
    Implements a VectorClock.
    Can hold arbitrarily much pairs of a Thread Id and the belonging clock
*/


class VectorClock {
public:
    static constexpr size_t multplier_64 = 0x100000000ull;
    ///vector clock which contains multiple thread ids, clocks
    xmap<size_t, size_t> vc;

 
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
            if (it->second > get_id_by_tid(it->first)) {
                update(it->first, it->second);
            }
        }
    };

    void update(std::shared_ptr<VectorClock> other) {
        for (auto it = other->vc.begin(); it != other->vc.end(); it++)
        {
            if (it->second > get_id_by_tid(it->first)) {
                update(it->first, it->second);
            }
        }
    };

    ///updates vector clock entry or creates entry if non-existant
    void update(size_t tid, size_t id) {
        if (vc.find(tid) == vc.end()) {
            vc.insert(vc.end(), { tid, id });
        }
        else {
            if (vc[tid] < id) { vc[tid] = id; }
        }
    };

    ///deletes a vector clock entry, checks existance before
    void delete_vc(uint32_t tid) {
        vc.erase(tid);
    }

    size_t get_clock_by_tid(size_t tid) {
        if (vc.find(tid) != vc.end()) {
            return make_clock(vc[tid]);
        }
        else {
            return 0;
        }
    }


    ///returns known vector clock of tid
    size_t get_id_by_tid(size_t tid) {
        if (vc.find(tid) != vc.end()) {
            return vc[tid];
        }
        else {
            return 0;
        }
    }

    static const uint32_t make_tid(size_t id) {
        return id / multplier_64;
    }

    static const uint32_t make_clock(size_t id) {
        return id % multplier_64;
    }

    static const size_t make_id(size_t tid) {
        return tid * VectorClock::multplier_64;
    }

};

#endif
