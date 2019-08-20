#include <map>

class HPState  {
public:
    std::map<int, int> vc;

    HPState::HPState() {    }

    void add_entry(int tid, int clock) {
        if (vc.find(tid) == vc.end()) {
            vc.insert(vc.end(), std::pair<int, int>(tid, clock));
        }
        else {
            vc[tid] = clock;
        }
    }

    uint32_t get_vc_by_id(int tid) {
        if (vc.find(tid) == vc.end()) {
            return 0;
        }
        else
        {
            return vc[tid];
        }
    }

    uint32_t get_id_by_pos(int pos) {
        if (pos < vc.size()) {
            std::map<int, int>::iterator it = vc.begin();
            std::advance(it, pos);
            return it->first;
        }
        else {
            return 0;
        }
    }

};
