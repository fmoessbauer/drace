#ifndef VECTORCLOCK_H
#define VECTORCLOCK_H

class VectorClock {


public:
    ///thread id, clock
    std::map<uint32_t, uint32_t> vc;


    //virtual void update(uint32_t tid, uint32_t vc) = 0;
    //virtual uint32_t get_vc_by_id(uint32_t tid) = 0;
    uint32_t get_id_by_pos(uint32_t pos) {
        if (pos < vc.size()) {
            auto it = vc.begin();
            std::advance(it, pos);
            return it->first;
        }
        else {
            return 0;
        }
    };

    uint32_t get_length() {
        return vc.size();
    };

    ///updates this.vc with values of other.vc, if they're larger
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
            vc[id] = clock;
        }
    };

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
