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

        std::vector<uint64_t> stack1 = thread1->return_stack_trace(clk1);
        std::vector<uint64_t> stack2 = thread2->return_stack_trace(clk2);

       

        detector::AccessEntry access1;
        access1.thread_id           = thr1;
        access1.write               = wr1;
        access1.accessed_memory     = var;
        access1.access_size         = 0;
        access1.access_type         = 0;
        access1.heap_block_begin    = 0;
        access1.heap_block_size     = 0;
        access1.onheap              = false;
        access1.stack_size          = stack1.size();
        std::copy(stack1.begin(), stack1.end(), access1.stack_trace);
           

        detector::AccessEntry access2;
        access2.thread_id           = thr2;
        access2.write               = wr2;
        access2.accessed_memory     = var;
        access2.access_size         = 0;
        access2.access_type         = 0;
        access2.heap_block_begin    = 0;
        access2.heap_block_size     = 0;
        access2.onheap              = false;
        access2.stack_size          = stack2.size();
        std::copy(stack2.begin(), stack2.end(), access2.stack_trace);
        
        detector::Race race;
        race.first = access1;
        race.second = access2;
        
        ((void(*)(const detector::Race*))clb)(&race);  
    }

    
    void read(ThreadState* t_ptr, VarState* v_ptr) {
#if 0  //cannot happen in current implementation as rw increments clock, discuss
        if (t_ptr->get_self() == v_ptr->r && t_ptr->tid == v_ptr->r_tid) {//read same epoch, sane thread
            t_ptr->inc_vc();
            return;
        }

        if (v_ptr->r == READ_SHARED && v_ptr->rvc[t_ptr->tid] == t_ptr->get_self()) { //read shared same epoch
            t_ptr->inc_vc();
            return;
        }
#endif
        if (v_ptr->w_clock >= t_ptr->get_vc_by_id(v_ptr->w_tid) && v_ptr->w_tid != t_ptr->tid) { // write-read race
            report_race(v_ptr->w_tid, t_ptr->tid,
                        true, false,
                        v_ptr->address,
                        v_ptr->w_clock, t_ptr->get_self());
        }

        //update vc
        if (v_ptr->r_clock != READ_SHARED) {
            if (v_ptr->r_clock == VAR_NOT_INIT || (v_ptr->r_clock < t_ptr->get_vc_by_id(v_ptr->r_tid) && v_ptr->r_tid == t_ptr->tid)) {//read exclusive
                v_ptr->update(false, t_ptr->get_self(), t_ptr->tid);
            }
            else { // read gets shared
                int tmp_r = v_ptr->r_clock;
                int tmp_r_tid = v_ptr->r_tid;
                v_ptr->set_read_shared();
                v_ptr->update(false, tmp_r, tmp_r_tid);
                v_ptr->update(false, t_ptr->get_self(), t_ptr->tid);
            } 
        }
        else {//read shared
           v_ptr->update(false, t_ptr->get_self(), t_ptr->tid);
        }

        t_ptr->inc_vc();
    }

    void write(ThreadState* t_ptr, VarState* v_ptr) {
        
        if(v_ptr->w_clock == VAR_NOT_INIT){ //initial write, update var
            v_ptr->update(true, t_ptr->get_self(), t_ptr->tid);
            t_ptr->inc_vc();
            return;
        }

#if 0 // not possib with current impl
        if (t_ptr->get_self() == v_ptr->w && t_ptr->tid == v_ptr->w_tid) {//write same epoch
            t_ptr->inc_vc();
            return;
        }
#endif
        //tids are different && and write epoch greater or equal than known epoch of other thread
        if (t_ptr->tid != v_ptr->w_tid && v_ptr->w_clock >= t_ptr->get_vc_by_id(v_ptr->w_tid)) { // write-write race
            report_race(v_ptr->w_tid, t_ptr->tid,
                true, true,
                v_ptr->address,
                v_ptr->w_clock, t_ptr->get_self());
        }

        if (v_ptr->r_clock != READ_SHARED) {
            if (v_ptr->r_tid != VAR_NOT_INIT && t_ptr->tid != v_ptr->r_tid && v_ptr->r_clock >= t_ptr->get_vc_by_id(v_ptr->r_tid)) { // read-write race
                report_race(v_ptr->r_tid, t_ptr->tid,
                    false, true,
                    v_ptr->address,
                    v_ptr->r_clock, t_ptr->get_self());
            }
        }
        else {
            for (int i = 0; i < v_ptr->get_length(); ++i) {
                uint32_t act_tid = v_ptr->get_id_by_pos(i);

                if (t_ptr->tid != act_tid && v_ptr->get_vc_by_id(act_tid) >= t_ptr->get_vc_by_id(act_tid)) {
                    //read shared read-write race                                    
                    report_race(act_tid, t_ptr->tid,
                        false, true,
                        v_ptr->address,
                        v_ptr->get_vc_by_id(act_tid), t_ptr->get_self());

                }
            }
        }
        v_ptr->update(true, t_ptr->get_self(), t_ptr->tid);
        t_ptr->inc_vc();
    }


    void acquire(ThreadState* t, LockState* l) {
        
        for (uint32_t i = 0; i < l->get_length(); ++i) {
            uint32_t tid = l->get_id_by_pos(i);
            if (tid == 0) {//should not happen
                break;
            }
            uint32_t lock_vc = l->get_vc_by_id(tid);
            uint32_t thread_vc = t->get_vc_by_id(tid);
            if (thread_vc < lock_vc) {
                t->update(tid, lock_vc);
            }
        }
    }

    void release(ThreadState* t, LockState* l) {
        
        //update own clock
        t->inc_vc();       
        
        // update l vc to max
        for (uint32_t i = 0; i < t->get_length(); ++i) {
            uint32_t id = t->get_id_by_pos(i);
            if(id == 0){
                break;
            }
            uint32_t thr_vc = t->get_vc_by_id(id);
            uint32_t lock_vc = l->get_vc_by_id(id);

            if (thr_vc > lock_vc) {
                l->update(id, thr_vc);
            }
        }
    }


    void happens_before(ThreadState* t, HPState* hp) {
        //increment clock of thread and update happens state
        t->inc_vc();
        hp->add_entry(t->tid, t->get_self());
    }

    void happens_after(ThreadState* t, HPState* hp) {
        //update vector clock of thread with happened before clocks

        int i = 0;
        while (hp->get_id_by_pos(i) != 0) {
            int tid = hp->get_id_by_pos(i);
            if (t->thread_vc[tid] < hp->get_vc_by_id(tid)) {
                t->thread_vc[tid] = hp->get_vc_by_id(tid);
            }
            i++;
        }
    }

 
}



void create_var(uint64_t addr) {
    VarState* var = new VarState(addr);
    vars_mutex.lock();
    vars.insert(vars.end(), std::pair<uint64_t, VarState*>(addr, var));
    vars_mutex.unlock();
}

void create_lock(void* mutex) {
    locks_mutex.lock();
    LockState* lock = new LockState();
    locks.insert(locks.end(), std::pair<void*, LockState*>(mutex, lock));
    locks_mutex.unlock();
}

void create_thread(detector::tid_t tid, ThreadState* parent = nullptr) {
    ThreadState* new_thread;
    if (parent == nullptr) {
        new_thread = new ThreadState(tid);
    }
    else{
        threads_mutex.lock();
        new_thread = new ThreadState(tid, parent);
        threads_mutex.unlock();
    }
    threads_mutex.lock();
    threads.insert(threads.end(), std::pair<detector::tid_t, ThreadState*>(tid, new_thread));
    threads_mutex.unlock();
}

void create_happens(void* identifier) {
    HPState * new_hp = new HPState();
    happens_mutex.lock();
    happens_states.insert(happens_states.end(), std::pair<void*, HPState*>(identifier, new_hp));
    happens_mutex.unlock();
}


bool detector::init(int argc, const char **argv, Callback rc_clb) {
    clb = rc_clb; //init callback
    std::cout << "init done" << std::endl;
    return true;
    
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
    uint64_t var_address = reinterpret_cast<uint64_t>(addr);

    if (vars.find(var_address) == vars.end()) { //create new variable if new
        std::cerr << "variable is read before written" << std::endl;
        create_var(var_address);
    }

    auto thr = threads[reinterpret_cast<detector::tid_t>(tls)];

    threads_mutex.lock();
    vars_mutex.lock();

    thr->set_read_write(reinterpret_cast<uint64_t>(pc));
    fasttrack::read(thr, vars[var_address]);

    threads_mutex.unlock();
    vars_mutex.unlock();
}

void detector::write(tls_t tls, void* pc, void* addr, size_t size) {
    uint64_t var_address = ((uint64_t)addr);

    if (vars.find(var_address) == vars.end()) {
        create_var(var_address);
    }
    auto thr = threads[reinterpret_cast<detector::tid_t>(tls)];

    threads_mutex.lock();
    vars_mutex.lock();

    thr->set_read_write(reinterpret_cast<uint64_t>(pc));
    fasttrack::write(thr, vars[var_address]);

    threads_mutex.unlock();
    vars_mutex.unlock();
}


void detector::func_enter(tls_t tls, void* pc) {
    ThreadState* thr = threads[reinterpret_cast<detector::tid_t>(tls)];
    uint64_t stack_element = reinterpret_cast<uint64_t>(pc);

    threads_mutex.lock();
    thr->push_stack_element(stack_element);
    threads_mutex.unlock();
}

void detector::func_exit(tls_t tls) {
    ThreadState* thr = threads[reinterpret_cast<detector::tid_t>(tls)];

    threads_mutex.lock();
    thr->pop_stack_element(); //pops last stack element of current clock
    threads_mutex.unlock();
}


void detector::fork(tid_t parent, tid_t child, tls_t * tls) {
    *tls = reinterpret_cast<void*>(child);
        
    //make parent thread if it isn't existing
    if (threads.find(parent) == threads.end()) {
        create_thread(parent);
    }

    threads_mutex.lock();
    threads[parent]->inc_vc(); //inc vector clock for creation of new thread
    threads_mutex.unlock();

    create_thread(child, threads[parent]);
}

void detector::join(tid_t parent, tid_t child) {
    threads_mutex.lock();
    ThreadState* del_thread = threads[child];
    del_thread->inc_vc();

    //pass incremented clock of deleted thread to parent
    threads[parent]->update(del_thread);

    delete del_thread;
    threads.erase(child);

    threads_mutex.unlock();
}


void detector::acquire(tls_t tls, void* mutex, int recursive, bool write) {
        
    if (locks.find(mutex) == locks.end()) {
        create_lock(mutex);
    }

    auto id = reinterpret_cast<detector::tid_t>(tls);

    threads_mutex.lock();
    locks_mutex.lock();
    fasttrack::acquire(threads[id], locks[mutex]);
    threads_mutex.unlock();
    locks_mutex.unlock();
}

void detector::release(tls_t tls, void* mutex, bool write) {
 
    if (locks.find(mutex) == locks.end()) {
        std::cerr << "lock is released but was never acquired by any thread" << std::endl;
        create_lock(mutex);
    }

    auto id = reinterpret_cast<detector::tid_t>(tls);

    threads_mutex.lock();
    locks_mutex.lock();
    fasttrack::release(threads[id], locks[mutex]);
    threads_mutex.unlock();
    locks_mutex.unlock();
}


void detector::happens_before(tls_t tls, void* identifier) {
    if (happens_states.find(identifier) == happens_states.end()) {
        create_happens(identifier);
    }

    HPState* hp = happens_states[identifier];
    ThreadState* thr = threads[reinterpret_cast<detector::tid_t>(tls)];

    happens_mutex.lock();
    threads_mutex.lock();
    fasttrack::happens_before(thr, hp);
    happens_mutex.unlock();
    threads_mutex.unlock();
}

/** Draw a happens-after edge between thread and identifier (optional) */
void detector::happens_after(tls_t tls, void* identifier) {
    
    if (happens_states.find(identifier) != happens_states.end()) {
        HPState* hp = happens_states[identifier];
        ThreadState* thr = threads[reinterpret_cast<detector::tid_t>(tls)];

        threads_mutex.lock();
        happens_mutex.lock();
        fasttrack::happens_after(thr, hp);
        happens_mutex.unlock();
        threads_mutex.unlock();

    }
    else {
        create_happens(identifier);
    }

    
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


