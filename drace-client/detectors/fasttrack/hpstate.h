#include <map>

class HPState  {
public:
    std::map<uint32_t, uint32_t> vc;

    HPState::HPState() {    }

    void add_entry(uint32_t tid, uint32_t clock) {
        if (vc.find(tid) == vc.end()) {
            vc.insert(vc.end(), std::pair<int, int>(tid, clock));
        }
        else {
            vc[tid] = clock;
        }
    }

    void delete_vc(uint32_t tid) {
        if(vc.find(tid) != vc.end())
            vc.erase(tid);
    }

    uint32_t get_vc_by_id(uint32_t tid) {
        if (vc.find(tid) == vc.end()) {
            return 0;
        }
        else
        {
            return vc[tid];
        }
    }

    uint32_t get_id_by_pos(uint32_t pos) {
        if (pos < vc.size()) {
            auto it = vc.begin();
            std::advance(it, pos);
            return it->first;
        }
        else {
            return 0;
        }
    }

};
