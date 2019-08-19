//#include <atomic>
//#include <algorithm>
#include <mutex> // for lock_guard
//#include <unordered_map>
#include <iostream>
#include <cassert>
#include <string>
#include <detector/detector_if.h>
#include <map>
#include <vector>
#include <ipc/spinlock.h>
#include "threadstate.h"
#include "varstate.h"
#include "lockstate.h"
#include "hpstate.h"
static constexpr int max_stack_size = 16;



/// globals ///
std::map<uint64_t, VarState*> vars;
std::map< void*, LockState*> locks;
std::map<detector::tid_t, ThreadState*> threads;
std::map<void*, HPState*> happens_states;
void * clb;

std::mutex vars_mutex;
std::mutex locks_mutex;
std::mutex threads_mutex;
std::mutex happens_mutex;

///end of globals///


namespace fasttrack {

    void report_race(   unsigned thr1, unsigned thr2,
                        bool wr1, bool wr2,
                        uint64_t var,
                        int clk1, int clk2)
    {
        ThreadState* thread1 = threads[thr1];
        ThreadState* thread2 = threads[thr2];

        std::vector<void *>* stack1 = thread1->get_stack_trace(clk1);
        std::vector<void *>* stack2 = thread2->get_stack_trace(clk2);

        uint64_t st1[max_stack_size];
        int i = 0;
        for (std::vector<void *>::iterator it = stack1->begin(); it != stack1->end(); it++) {
            st1[i] =  reinterpret_cast<uint64_t>(*it);
            i++;
        }

        detector::AccessEntry access1 = {
            thr1,
            wr1,
            var,
            0,
            0,
            0,
            0,
            false,
            (stack1->size()),
            *st1
        };

        uint64_t st2[max_stack_size];
        i = 0;
        for (std::vector<void *>::iterator it = stack2->begin(); it != stack2->end(); it++) {
            st2[i] = reinterpret_cast<uint64_t>(*it);
            i++;
        }
        detector::AccessEntry access2 = {
            thr2,
            wr2,
            var,
            0,
            0,
            0,
            0,
            false,
            (stack2->size()),
            *st2
        };

        detector::Race race;
        race.first = access1;
        race.second = access2;
        
        ((void(*)(const detector::Race*))clb)(&race);  
    }

    
    void read(ThreadState* t_ptr, VarState* v_ptr) {
        
        threads_mutex.lock();
        vars_mutex.lock();
        ThreadState t = *t_ptr;
        //VarState    v = *v_ptr;
        threads_mutex.unlock();
       
        if (t.get_self() == v_ptr->r && t.tid == v_ptr->r_tid) {//read same epoch
            return;
        }

        if (v_ptr->r == READ_SHARED && v_ptr->rvc[t.tid] == t.get_self()) { //read shared same epoch
            return;
        }

        if (v_ptr->w >= t.get_vc_by_id(v_ptr->w_tid) && v_ptr->w_tid != t.tid) { // write-read race
            report_race(v_ptr->w_tid, t.tid,
                        true, false,
                        v_ptr->address,
                        v_ptr->w, t.get_self());
        }

        //update vc
        if (v_ptr->r != READ_SHARED) {
            if (v_ptr->r == -1 || (v_ptr->r < t.get_vc_by_id(v_ptr->r_tid) || v_ptr->r_tid == t.tid)) {//read exclusive
                v_ptr->update(false, t.get_self(), t.tid);
            }
            else { // read gets shared
                int tmp = v_ptr->r;
                v_ptr->set_read_shared();
                v_ptr->update(false, tmp, v_ptr->r_tid);
                v_ptr->update(false, t.get_self(), t.tid);
            } 
        }
        else {//read shared
           v_ptr->update(false, t.get_self(), t.tid);
        }
         vars_mutex.unlock();
         threads_mutex.lock();
         t_ptr->inc_vc();
         threads_mutex.unlock();
    }

    void write(ThreadState* t_ptr, VarState* v_ptr) {
        threads_mutex.lock();
        vars_mutex.lock();
        ThreadState t = *t_ptr;
        threads_mutex.unlock();;

        if(v_ptr->w == VAR_NOT_INIT){ //initial write, update var and 
            v_ptr->update(true, t.get_self(), t.tid);
            vars_mutex.unlock();
            threads_mutex.lock();
            t_ptr->inc_vc();
            threads_mutex.unlock();
            return;
        }
        

        if (t.get_self() == v_ptr->w && t.tid == v_ptr->w_tid) {//write same epoch
            vars_mutex.unlock();
            threads_mutex.lock();
            t_ptr->inc_vc();
            threads_mutex.unlock();
            return;
        }

        
        if (v_ptr->w >= t.get_vc_by_id(v_ptr->w_tid) && t.tid != v_ptr->w_tid) { // write-write race
            report_race(v_ptr->w_tid, t.tid,
                true, true,
                v_ptr->address,
                v_ptr->w, t.get_self());
        }

        if (v_ptr->r != READ_SHARED) {
            if (v_ptr->r >= t.get_vc_by_id(v_ptr->r_tid) && t.tid != v_ptr->r_tid && v_ptr->r_tid != -1) { // read-write race
                report_race(v_ptr->r_tid, t.tid,
                    false, true,
                    v_ptr->address,
                    v_ptr->r, t.get_self());
            }
        }
        else {
            for (int i = 0; i < v_ptr->get_length(); ++i) {
                int act_tid = t.get_thread_id_by_pos(i);
                if (v_ptr->get_vc_by_id(act_tid) >= t.get_vc_by_id(act_tid) && t.tid != act_tid) {
                                                        //read shared read-write race
                    report_race(act_tid, t.tid,
                        false, true,
                        v_ptr->address,
                        v_ptr->get_vc_by_id(act_tid), t.get_self());

                }
            }
        }
        v_ptr->update(true, t.get_self(), t.tid);
        vars_mutex.unlock();
        
    }


    void acquire(ThreadState* t, LockState* l) {
        //LockState lock = *l;
        //ThreadState thr = *t;
        threads_mutex.lock();
        locks_mutex.lock();

        int i = 0;
        while ( l->get_thread_id_by_pos(i) != -1) {
            int tid = l->get_thread_id_by_pos(i);
            int lock_vc = l->get_vc_by_id(tid);
            int thread_vc = t->get_vc_by_id(tid);
            if (thread_vc < lock_vc) {
                t->update(tid, lock_vc);
            }
            i++;
        }
        threads_mutex.unlock();
        locks_mutex.unlock();
    }

    void release(ThreadState* t, LockState* l) {
        threads_mutex.lock();
        locks_mutex.lock();
        //update own clock
        int clock = t->get_self();
        t->update(t->tid, (clock + 1)); //increase vector clock
        
        
        /// update t and l vc to max
        for (int i = 0; i < t->get_length(); ++i) {
            int id = t->get_thread_id_by_pos(i);
            if(id == -1){
                break;
            }
            int thr_vc = t->get_vc_by_id(id);
            int lock_vc = l->get_vc_by_id(id);

            if (thr_vc > lock_vc) {
                l->update(id, thr_vc);
            }
        }
        threads_mutex.unlock();
        locks_mutex.unlock();
    }


    void happens_before(ThreadState* t, HPState* hp) {
        //increment clock and update happens state
        t->inc_vc();
        hp->add_entry(t->tid, t->get_self());
    }

    void happens_after(ThreadState* t, HPState* hp) {
        //check how clock must be incremented

        int i = 0;

        while (hp->get_id_by_pos(i) != -1) {
            int tid = hp->get_id_by_pos(i);
            if (t->thread_vc[tid] < hp->get_vc_by_id(tid)) {
                t->thread_vc[tid] = hp->get_vc_by_id(tid);
            }
            i++;
        }
    }

 
}

namespace detector {
    namespace fasttrack2 {
        
    }
}






bool detector::init(int argc, const char **argv, Callback rc_clb) {

    //fasttrackalgorithm::init((void*) rc_clb);
    clb = rc_clb;
    std::cout << "init done" << std::endl;
    return true;
    //init callback
}


void detector::finalize() {

    while (vars.size() > 0) {
        auto it = vars.begin();
        delete it->second;
        vars.erase(it);
    }
    while (locks.size() > 0) {
        auto it = locks.begin();
        delete it->second;
        locks.erase(it);
    }
    while (threads.size() > 0) {
        auto it = threads.begin();
        delete it->second;
        threads.erase(it);
    }
    while (happens_states.size() > 0) {
        auto it = happens_states.begin();
        delete it->second;
        happens_states.erase(it);
    }
}

void detector::read(tls_t tls, void* pc, void* addr, size_t size) {
    uint64_t var_address = (uint64_t)addr;

    if (vars.find(var_address) == vars.end()) { //create new variable if new 
        VarState* var = new VarState(var_address);
        vars_mutex.lock();
        vars.insert(vars.end(), std::pair<uint64_t, VarState*>(var_address, var));
        vars_mutex.unlock();
    }

    auto thr = threads[reinterpret_cast<detector::tid_t>(tls)];
    fasttrack::read(thr, vars[var_address]);
}

void detector::write(tls_t tls, void* pc, void* addr, size_t size) {
    uint64_t var_address = ((uint64_t)addr);

    if (vars.find(var_address) == vars.end()) {
        VarState* var = new VarState(var_address);
        vars_mutex.lock();
        vars.insert(vars.end(), std::pair<uint64_t, VarState*>(var_address, var));
        vars_mutex.unlock();
    }

    fasttrack::write(threads[reinterpret_cast<detector::tid_t>(tls)], vars[var_address]);
}


void detector::func_enter(tls_t tls, void* pc) {
    threads_mutex.lock();
    ThreadState* thr = threads[reinterpret_cast<detector::tid_t>(tls)];
    int clock = thr->get_self();

    if (thr->stack_trace.find(clock) == thr->stack_trace.end()){
        std::vector<void*>* tmp = thr->get_stack_trace(clock - 1); // get stack trace of clock of before
        std::vector<void*> new_stack_trace = *tmp;
        new_stack_trace.push_back(pc);
        thr->stack_trace.insert(thr->stack_trace.end(), std::pair<int, std::vector<void*>>(clock, new_stack_trace));
    }
    else {
        thr->stack_trace[clock].push_back(pc);
    }
    threads_mutex.unlock();
}

void detector::func_exit(tls_t tls) {
    threads_mutex.lock();
    ThreadState* thr = threads[reinterpret_cast<detector::tid_t>(tls)];
    int clock = thr->get_self();

    std::vector<void*>* stack = thr->get_stack_trace(clock);
    stack->pop_back();
    threads_mutex.unlock();
}

void detector::fork(tid_t parent, tid_t child, tls_t * tls) {
    threads_mutex.lock();
    *tls = reinterpret_cast<void*>(child);
    ThreadState* new_thread = new ThreadState(child);

    if (threads.find(parent) == threads.end()) {
        ThreadState* new_parent_thread = new ThreadState(parent);
        threads.insert(threads.end(), std::pair<tid_t, ThreadState*>(parent, new_parent_thread));
    }

    threads[parent]->inc_vc();
    //copy parent vc to child vc
    (new_thread->thread_vc).insert(threads[parent]->thread_vc.begin(), threads[parent]->thread_vc.end());

    threads.insert(threads.end(), std::pair<tid_t, ThreadState*>(child, new_thread));
    threads_mutex.unlock();
}

void detector::join(tid_t parent, tid_t child) {
    threads_mutex.lock();
    ThreadState* del_thread = threads[child];

    //pass incremented clock of deleted thread to parent
    threads[parent]->update(del_thread->tid, (del_thread->get_self() + 1));
    delete del_thread;
    threads.erase(child);
    threads_mutex.unlock();
}


void detector::acquire(tls_t tls, void* mutex, int recursive, bool write) {
    threads_mutex.lock();
    locks_mutex.lock();

    ThreadState* thr = threads[reinterpret_cast<detector::tid_t>(tls)];
    if (locks.find(mutex) == locks.end()) {
        LockState* lock = new LockState();//(thr->tid, thr->get_self());
        locks.insert(locks.end(), std::pair<void*, LockState*>(mutex, lock));
    }
    threads_mutex.unlock();
    locks_mutex.unlock();
       
    fasttrack::acquire(thr, locks[mutex]);

}

void detector::release(tls_t tls, void* mutex, bool write) {
    threads_mutex.lock();
    locks_mutex.lock();

    ThreadState* thr = threads[reinterpret_cast<detector::tid_t>(tls)];
    if (locks.find(mutex) == locks.end()) {
        LockState* lock = new LockState();
        locks.insert(locks.end(), std::pair<void*, LockState*>(mutex, lock));
    }
    threads_mutex.unlock();
    locks_mutex.unlock();

    fasttrack::release(thr, locks[mutex]);

}


void detector::happens_before(tls_t tls, void* identifier) {
    
    threads_mutex.lock();
    happens_mutex.lock();
    ThreadState* thr = threads[reinterpret_cast<detector::tid_t>(tls)];
  
    if (happens_states.find(identifier) == happens_states.end()) {
        HPState * new_hp = new HPState();
        happens_states.insert(happens_states.end(), std::pair<void*, HPState*>(identifier, new_hp));
    }
    HPState* hp = happens_states[identifier];
    fasttrack::happens_before(thr, hp);

    happens_mutex.unlock();
    threads_mutex.unlock();
}

/** Draw a happens-after edge between thread and identifier (optional) */
void detector::happens_after(tls_t tls, void* identifier) {
    threads_mutex.lock();
    happens_mutex.lock();

    ThreadState* thr = threads[reinterpret_cast<detector::tid_t>(tls)];

    if (happens_states.find(identifier) != happens_states.end()) {
        HPState* hp = happens_states[identifier];
        fasttrack::happens_after(thr, hp);
    }
    else {
        HPState * new_hp = new HPState();
        happens_states.insert(happens_states.end(), std::pair<void*, HPState*>(identifier, new_hp));
    }

    happens_mutex.unlock();
    threads_mutex.unlock();
}

void detector::allocate(
    /// ptr to thread-local storage of calling thread
    tls_t  tls,
    /// current instruction pointer
    void*  pc,
    /// begin of allocated memory block
    void*  addr,
    /// size of memory block
    size_t size
){}

/** Log a memory deallocation*/
void detector::deallocate(
    /// ptr to thread-local storage of calling thread
    tls_t tls,
    /// begin of memory block
    void* addr
){}


void detector::detach(tls_t tls, tid_t thread_id){}
/** Log a thread exit event (detached thread) */
void detector::finish(tls_t tls, tid_t thread_id){}



const char * detector::name() {
    return "FASTTRACK2";
}

const char * detector::version() {
    return "0.1";
}


