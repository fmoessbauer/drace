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
#include "allocstate.h"
#include <ipc/spinlock.h>
static constexpr int max_stack_size = 16;



/// globals ///
std::map<std::pair<detector::tid_t, uint64_t>, AllocationState*> allocs;
std::map<uint64_t, VarState*> vars;
std::map< void*, LockState*> locks;
std::map<detector::tid_t, ThreadState*> threads;
std::map<void*, HPState*> happens_states;
void * clb;

static ipc::spinlock v_lock;
static ipc::spinlock l_lock;
static ipc::spinlock t_lock;
static ipc::spinlock h_lock;
static ipc::spinlock a_lock;

///end of globals///


namespace fasttrack {

    void report_race(   unsigned thr1, unsigned thr2,
                        bool wr1, bool wr2,
                        uint64_t var,
                        int clk1, int clk2, bool is_alloc_race=false){

        ThreadState* thread1 = threads[thr1];
        ThreadState* thread2 = threads[thr2];

        std::vector<uint64_t> stack1, stack2;
        stack1 = thread1->return_stack_trace(clk1, var, is_alloc_race);
        stack2 = thread2->return_stack_trace(clk2, var, is_alloc_race);
    

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

    
    void read(ThreadState* t, VarState* v) {
#if 0  //cannot happen in current implementation as rw increments clock, discuss
        if (t->get_self() == v_ptr->r && t->tid == v_ptr->r_tid) {//read same epoch, sane thread
            t->inc_vc();
            return;
        }

        if (v->r == READ_SHARED && v->rvc[t_ptr->tid] == t->get_self()) { //read shared same epoch
            t->inc_vc();
            return;
        }
#endif
        if (v->is_wr_race(t))
        { // write-read race
            report_race(v->w_tid, t->tid,
                        true, false,
                        v->address,
                        v->w_clock, t->get_self());
        }

        //update vc
        if (v->r_clock != READ_SHARED)
        {
            if (v->r_clock == VAR_NOT_INIT  ||
               (v->r_tid == t->tid     &&
               v->r_clock < t->get_vc_by_id(v->r_tid)))//read exclusive
            {
                v->update(false, t->get_self(), t->tid);
            }
            else { // read gets shared
                int tmp_r = v->r_clock;
                int tmp_r_tid = v->r_tid;
                v->set_read_shared();
                v->update(false, tmp_r, tmp_r_tid);
                v->update(false, t->get_self(), t->tid);
            } 
        }
        else {//read shared
           v->update(false, t->get_self(), t->tid);
        }

        t->inc_vc();
    }

    void write(ThreadState* t, VarState* v) {
        
        if(v->w_clock == VAR_NOT_INIT){ //initial write, update var
            v->update(true, t->get_self(), t->tid);
            t->inc_vc();
            return;
        }

#if 0 // not possib with current impl
        if (t->get_self() == v->w &&
            t->tid == v->w_tid)
        {//write same epoch
            t->inc_vc();
            return;
        }
#endif
        //tids are different && and write epoch greater or equal than known epoch of other thread
        if (v->is_ww_race(t)) // write-write race
        {
            report_race(v->w_tid, t->tid, true, true, v->address, v->w_clock, t->get_self());
        }

        if (v->r_clock != READ_SHARED) {
            if (v->is_rw_ex_race(t))// read-write race
            { 
                report_race(v->r_tid, t->tid, false, true, v->address, v->r_clock, t->get_self());
            }
        }
        else {//come here in read shared case
            uint32_t act_tid = v->is_rw_sh_race(t);
            if (act_tid) //read shared read-write race       
            {
                report_race(act_tid, t->tid, false, true, v->address, v->get_vc_by_id(act_tid), t->get_self());
            }
        }
        v->update(true, t->get_self(), t->tid);
        t->inc_vc();
    }


    void acquire(ThreadState* t, LockState* l) {
        t->update(l);
    }

    void release(ThreadState* t, LockState* l) {
        
        //update own clock
        t->inc_vc();       
        l->update(t);
    }


    void happens_before(ThreadState* t, HPState* hp) {
        //increment clock of thread and update happens state
        t->inc_vc();
        hp->update(t->tid, t->get_self());
    }

    void happens_after(ThreadState* t, HPState* hp) {
        //update vector clock of thread with happened before clocks
        t->update(hp);
    }

}



void create_var(uint64_t addr) {
    VarState* var = new VarState(addr);
    v_lock.lock();
    vars.insert(vars.end(), std::pair<uint64_t, VarState*>(addr, var));
    v_lock.unlock();
}

void create_lock(void* mutex) {
    l_lock.lock();
    LockState* lock = new LockState();
    locks.insert(locks.end(), std::pair<void*, LockState*>(mutex, lock));
    l_lock.unlock();
}

void create_thread(detector::tid_t tid, ThreadState* parent = nullptr) {
    ThreadState* new_thread;
    if (parent == nullptr) {
        new_thread = new ThreadState(tid);
    }
    else{
        t_lock.lock();
        new_thread = new ThreadState(tid, parent);
        t_lock.unlock();
    }
    t_lock.lock();
    threads.insert(threads.end(), std::pair<detector::tid_t, ThreadState*>(tid, new_thread));
    t_lock.unlock();
}

void create_happens(void* identifier) {
    HPState * new_hp = new HPState();
    h_lock.lock();
    happens_states.insert(happens_states.end(), std::pair<void*, HPState*>(identifier, new_hp));
    h_lock.unlock();
}

AllocationState* create_alloc(detector::tid_t tid, uint64_t addr, uint64_t pc, size_t size) {

    auto alloc_id = std::pair<detector::tid_t, uint64_t>(tid, addr);
    auto alloc_it = allocs.find(alloc_id);
    AllocationState* a;

    if (alloc_it == allocs.end()) {
        a = new AllocationState(addr, size, tid, pc);
        allocs.insert(allocs.end(), { alloc_id, a });
    }
    else {//allocation is not new, but renewed, by same thread
        alloc_it->second->alloc(); //dealloc_clock is reset again
        a = alloc_it->second;
    }

    return a;

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

    t_lock.lock();
    v_lock.lock();

    thr->set_read_write(var_address, reinterpret_cast<uint64_t>(pc));
    fasttrack::read(thr, vars[var_address]);

    t_lock.unlock();
    v_lock.unlock();
}

void detector::write(tls_t tls, void* pc, void* addr, size_t size) {
    uint64_t var_address = ((uint64_t)addr);

    if (vars.find(var_address) == vars.end()) {
        create_var(var_address);
    }
    auto thr = threads[reinterpret_cast<detector::tid_t>(tls)];

    t_lock.lock();
    v_lock.lock();

    thr->set_read_write(var_address, reinterpret_cast<uint64_t>(pc));
    fasttrack::write(thr, vars[var_address]);

    t_lock.unlock();
    v_lock.unlock();
}


void detector::func_enter(tls_t tls, void* pc) {
    ThreadState* thr = threads[reinterpret_cast<detector::tid_t>(tls)];
    uint64_t stack_element = reinterpret_cast<uint64_t>(pc);

    t_lock.lock();
    thr->push_stack_element(stack_element);
    t_lock.unlock();
}

void detector::func_exit(tls_t tls) {
    ThreadState* thr = threads[reinterpret_cast<detector::tid_t>(tls)];

    t_lock.lock();
    thr->pop_stack_element(); //pops last stack element of current clock
    t_lock.unlock();
}


void detector::fork(tid_t parent, tid_t child, tls_t * tls) {
    *tls = reinterpret_cast<void*>(child);
        
    //make parent thread if it isn't existing
    //if (threads.find(parent) == threads.end()) {
      //  create_thread(parent);
    //}

    if (threads.find(parent) != threads.end()) {
        t_lock.lock();
        threads[parent]->inc_vc(); //inc vector clock for creation of new thread
        t_lock.unlock();
        create_thread(child, threads[parent]);
    }
    else {
        create_thread(child);
    }
    
}

void detector::join(tid_t parent, tid_t child) {
    t_lock.lock();
    ThreadState* del_thread = threads[child];
    del_thread->inc_vc();

    //pass incremented clock of deleted thread to parent
    threads[parent]->update(del_thread);


    //delete del_thread;
    //threads.erase(child);

    t_lock.unlock();
}


void detector::acquire(tls_t tls, void* mutex, int recursive, bool write) {
        
    if (locks.find(mutex) == locks.end()) {
        create_lock(mutex);
    }

    auto id = reinterpret_cast<detector::tid_t>(tls);

    t_lock.lock();
    l_lock.lock();
    fasttrack::acquire(threads[id], locks[mutex]);
    t_lock.unlock();
    l_lock.unlock();
}

void detector::release(tls_t tls, void* mutex, bool write) {
 
    if (locks.find(mutex) == locks.end()) {
        std::cerr << "lock is released but was never acquired by any thread" << std::endl;
        create_lock(mutex);
    }

    auto id = reinterpret_cast<detector::tid_t>(tls);

    t_lock.lock();
    l_lock.lock();
    fasttrack::release(threads[id], locks[mutex]);
    t_lock.unlock();
    l_lock.unlock();
}


void detector::happens_before(tls_t tls, void* identifier) {
    if (happens_states.find(identifier) == happens_states.end()) {
        create_happens(identifier);
    }

    HPState* hp = happens_states[identifier];
    ThreadState* thr = threads[reinterpret_cast<detector::tid_t>(tls)];

    h_lock.lock();
    t_lock.lock();
    fasttrack::happens_before(thr, hp);
    h_lock.unlock();
    t_lock.unlock();
}

/** Draw a happens-after edge between thread and identifier (optional) */
void detector::happens_after(tls_t tls, void* identifier) {
    
    if (happens_states.find(identifier) != happens_states.end()) {
        HPState* hp = happens_states[identifier];
        ThreadState* thr = threads[reinterpret_cast<detector::tid_t>(tls)];

        t_lock.lock();
        h_lock.lock();
        fasttrack::happens_after(thr, hp);
        h_lock.unlock();
        t_lock.unlock();

    }
    else {//create -> no happens_before can be synced
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
){
    auto tid = reinterpret_cast<detector::tid_t>(tls);
    auto address = reinterpret_cast<uint64_t>(addr);
    auto prog_count = reinterpret_cast<uint64_t>(pc);
    a_lock.lock();
    t_lock.lock();
    AllocationState* this_alloc = create_alloc(tid, address, prog_count, size);
    ThreadState* thr = threads[tid];

    //iterate through all existing allocation and check, if unsynchronized threads
    //allocated the same 
    for (auto it = allocs.begin(); it != allocs.end(); ++it) {
        
        uint32_t clock = thr->get_vc_by_id(it->first.first);
        if (it->second->is_race(this_alloc, clock)) {
            fasttrack::report_race(it->second->get_owner_tid(), tid,
                        true, true,
                        it->second->get_addr(),
                        it->second->get_dealloc_clock(), thr->get_self());
        }
    }
    t_lock.unlock();
    a_lock.unlock();

}

/** Log a memory deallocation*/
void detector::deallocate(
    /// ptr to thread-local storage of calling thread
    tls_t tls,
    /// begin of memory block
    void* addr
){
    auto tid = reinterpret_cast<detector::tid_t>(tls);
    auto address = reinterpret_cast<uint64_t>(addr);
    auto alloc_id = std::pair<detector::tid_t, uint64_t>(tid, address);
    ThreadState* thr = threads[tid];

    a_lock.lock();
    t_lock.lock();
    allocs[alloc_id]->dealloc(thr->get_self());
    t_lock.unlock();
    a_lock.unlock();
}


void detector::detach(tls_t tls, tid_t thread_id){}
/** Log a thread exit event (detached thread) */
void detector::finish(tls_t tls, tid_t thread_id){}



const char * detector::name() {
    return "FASTTRACK2";
}

const char * detector::version() {
    return "0.1";
}


