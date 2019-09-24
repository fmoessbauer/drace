#ifndef VECTORCLOCK_H
#define VECTORCLOCK_H

#include "parallel_hashmap/phmap.h"
#include <unordered_map>

/**
    Implements a VectorClock.
    Can hold arbitrarily much pairs of a Thread Id and the belonging clock
*/
template <class _al = std::allocator<std::pair<const size_t, size_t>>>

class VectorClock {
public:
    ///by dividing the id with the multiplier one gets the tid, with modulo one gets the clock
    static constexpr size_t multplier_64 = 0x100000000ull;

    ///vector clock which contains multiple thread ids, clocks
    std::unordered_map<uint32_t, size_t> vc;
 
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

    ///updates this.vc with values of other.vc, if they're larger -> one way update
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
        auto it = vc.find(tid);
        if (it == vc.end()) {
            vc.insert({ tid, id });
        }
        else {
            if (it->second < id) { it->second = id; }
        }
    };

    ///deletes a vector clock entry, checks existance before
    void delete_vc(uint32_t tid) {
        vc.erase(tid);
    }

    size_t get_clock_by_tid(size_t tid) {
        auto it = vc.find(tid);
        if (it != vc.end()) {
            return make_clock(it->second);
        }
        else {
            return 0;
        }
    }


    ///returns known vector clock of tid
    size_t get_id_by_tid(size_t tid) {
        auto it = vc.find(tid);
        if (it != vc.end()) {
            return it->second;
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
