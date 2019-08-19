#include <string>
#include <map>
#include <vector>
#include "ftstates.h"


class ThreadState : public FTStates{
public:
    std::map<int, int> thread_vc; //thread id vector clocks

    std::map<int, std::vector< void* >> stack_trace;
    int tid;

    ThreadState::ThreadState(int own_tid, void* pc=0) {
        tid = own_tid;
        int clock = 0;
        thread_vc.insert(thread_vc.end(), std::pair<int, int>(tid, clock));
        stack_trace.insert(stack_trace.end(), std::pair <int, std::vector<void*>>(clock, { pc }));
    };

    void update(uint32_t thread_id, uint32_t vc) {
        std::map<int, int>::iterator it;
        it = thread_vc.find(thread_id);
        if (it != thread_vc.end()) {
            thread_vc[thread_id] = vc;
        }
        else {
            thread_vc.insert(thread_vc.end(), std::pair<int, int>(thread_id, vc));
        }

    };

    void update(ThreadState other) {
        for (auto it = other.thread_vc.begin(); it != other.thread_vc.end(); it++) {
            if (it->second > get_vc_by_id(it->first)) {
                update(it->first, it->second);
            }
        }
    };


    uint32_t get_self() {
        return thread_vc[tid];
    };

    void inc_vc() {
        thread_vc[tid]++;
    }


    uint32_t get_vc_by_id(uint32_t other_tid) {
        std::map<int, int>::iterator it;
        it = thread_vc.find(other_tid);
        if (it == thread_vc.end()){
            update(other_tid, 0);
        }
        return thread_vc[other_tid];
    };

    uint32_t get_length() {
        return thread_vc.size();
    };

    uint32_t get_thread_id_by_pos(uint32_t pos) {
        if (pos < thread_vc.size()) {
            std::map<int, int>::iterator it = thread_vc.begin();
            std::advance(it, pos);
            return it->first;
        }
        else {
            return -1;
        }
    };

    std::vector<void*>* get_stack_trace(int clock) {
        /// return pointer to stack trace element of clock or if not existing the closest one before
        int i = clock;
        while (i >= 0) {
            if (stack_trace.find(i) == stack_trace.end()) {
                i--;
            }
            else {
                stack_trace.insert(stack_trace.end(), std::pair<int, std::vector<void*>>(clock, stack_trace[i]));
                break;
            }
            
        }
        return &stack_trace[clock];
    }

};
