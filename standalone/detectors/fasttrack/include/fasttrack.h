/*
 * DRace, a dynamic data race detector
 *
 * Copyright 2018 Siemens AG
 *
 * Authors:
 *   Felix Moessbauer <felix.moessbauer@siemens.com>
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef FASTTRACK_H
#define FASTTRACK_H

#include <mutex> // for lock_guard
#include <shared_mutex>
#include <iostream>
#include <iomanip>
#include <detector/Detector.h>
#include "threadstate.h"
#include "varstate.h"
#include "stacktrace.h"
#include "xvector.h"
#include "parallel_hashmap/phmap.h"

#define MAKE_OUTPUT false
#define REGARD_ALLOCS true
#define POOL_ALLOC false

#if POOL_ALLOC
#include "util/PoolAllocator.h"
#endif

namespace drace {
    namespace detector {

        template<class LockT>
        class Fasttrack : public Detector {
        
        public:
            typedef size_t tid_ft;
            //make some shared pointers a bit more handy
            typedef std::shared_ptr<ThreadState> ts_ptr;
            typedef std::shared_ptr<VarState> vs_ptr;

            #if POOL_ALLOC
            template <class X>
            using c_alloc = Allocator<X, 524288>;
            #else
            template <class X>
            using c_alloc = std::allocator<X>;
            #endif
            
        private:

            ///these maps hold the various state objects together with the identifiers

            phmap::parallel_node_hash_map<size_t, size_t> allocs;
            phmap::parallel_node_hash_map<size_t, vs_ptr> vars;
            phmap::parallel_node_hash_map<void*, std::shared_ptr<VectorClock<>>> locks;
            phmap::node_hash_map<tid_ft, ts_ptr> threads;
            phmap::parallel_node_hash_map<void*, std::shared_ptr<VectorClock<>>> happens_states;
        
            ///holds the callback address to report a race to the drace-main 
            Callback clb;

            ///switch logging of read/write operations
            bool log_flag = false;

            ///
            struct log_counters{
                uint32_t read_ex_same_epoch = 0;
                uint32_t read_sh_same_epoch = 0;
                uint32_t read_shared = 0;
                uint32_t read_exclusive = 0;
                uint32_t read_share = 0;
                uint32_t write_same_epoch = 0;
                uint32_t write_exclusive = 0;
                uint32_t write_shared = 0;
            } log_count;

            ///those locks secure the .find and .insert actions to the according hashmaps
            LockT g_lock; // global Lock

            void report_race(
                uint32_t thr1, uint32_t thr2,
                bool wr1, bool wr2,
                size_t var)
            {
                std::list<size_t> stack1, stack2;
                auto it = threads.find(thr1);
                auto it2 = threads.find(thr2);
                auto it_end = threads.end();

                if (it == it_end || it2 == it_end) {//if thread_id is, because of finishing, not in stack traces anymore, return
                    return;
                }
                stack1 = it->second->get_stackDepot().return_stack_trace(var);
                stack2 = it2->second->get_stackDepot().return_stack_trace(var);
                
                size_t var_size = 0;
                var_size = vars[var]->size; //size is const member -> thread safe

                while(stack1.size() > Detector::max_stack_size) {
                    stack1.pop_front();
                }
                while (stack2.size() > Detector::max_stack_size) {
                    stack2.pop_front();
                }

                Detector::AccessEntry access1;
                access1.thread_id = thr1;
                access1.write = wr1;
                access1.accessed_memory = var;
                access1.access_size = var_size;
                access1.access_type = 0;
                access1.heap_block_begin = 0;
                access1.heap_block_size = 0;
                access1.onheap = false;
                access1.stack_size = stack1.size();
                std::copy(stack1.begin(), stack1.end(), access1.stack_trace);

                Detector::AccessEntry access2;
                access2.thread_id = thr2;
                access2.write = wr2;
                access2.accessed_memory = var;
                access2.access_size = var_size;
                access2.access_type = 0;
                access2.heap_block_begin = 0;
                access2.heap_block_size = 0;
                access2.onheap = false;
                access2.stack_size = stack2.size();
                std::copy(stack2.begin(), stack2.end(), access2.stack_trace);


                Detector::Race race;
                race.first = access1;
                race.second = access2;

                ((void(*)(const Detector::Race*))clb)(&race);
            }

            /**
             * \brief takes care of a read access 
             * \note works only on thread and var object, not on any list
             *       Invariant: var object is locked
             */
            void read(ThreadState * t, vs_ptr v){
                
                if (t->return_own_id() == v->get_read_id()) {//read same epoch, same thread;
                    if(log_flag){
                        log_count.read_ex_same_epoch++;
                    }
                    return;
                }

                size_t id = t->return_own_id();
                uint32_t tid = t->get_tid();
                
                if (v->is_read_shared() && v->get_vc_by_thr(tid) == id) { //read shared same epoch
                    if(log_flag){
                        log_count.read_sh_same_epoch++;
                    }
                    return;
                }

                if (v->is_wr_race(t))
                { // write-read race
                    report_race(v->get_w_tid(), tid, true, false, v->address);
                }

                //update vc
                if (!v->is_read_shared())
                {
                    if (v->get_read_id() == VarState::VAR_NOT_INIT ||
                    (v->get_r_tid() == tid )) //read exclusive->read of same thread but newer epoch
                    {
                        if(log_flag){
                            log_count.read_exclusive++;
                        }
                        v->update(false, id);
                    }
                    else { // read gets shared
                        if(log_flag){
                            log_count.read_share++;
                        }
                        v->set_read_shared(id);
                    }
                }
                else {//read shared
                    if(log_flag){
                        log_count.read_shared++;
                    }
                    v->update(false, id);
                }
            }

            /**
             * \brief takes care of a write access 
             * \note works only on thread and var object, not on any list
             *       Invariant: var object is locked
             */
            void write(ThreadState * t, vs_ptr v){

                if (t->return_own_id() == v->get_write_id()){//write same epoch
                    if(log_flag){
                        log_count.write_same_epoch++;
                    }
                    return;
                }

                if (v->get_write_id() == VarState::VAR_NOT_INIT) { //initial write, update var
                    if(log_flag){
                        log_count.write_exclusive++;
                    }
                    v->update(true, t->return_own_id());
                    return;
                }

                uint32_t tid = t->get_tid();

                //tids are different and write epoch greater or equal than known epoch of other thread
                if (v->is_ww_race(t)) // write-write race
                {
                    report_race(v->get_w_tid(), tid, true, true, v->address);
                }

                if (! v->is_read_shared()) {
                    if(log_flag){
                        log_count.write_exclusive++;
                    }
                    if (v->is_rw_ex_race(t))// read-write race
                    {
                        report_race(v->get_r_tid(), t->get_tid(), false, true, v->address);
                    }
                }
                else {//come here in read shared case
                    if(log_flag){
                        log_count.write_shared++;
                    }
                    uint32_t act_tid = v->is_rw_sh_race(t);
                    if (act_tid != 0) //read shared read-write race       
                    {
                        report_race(act_tid, tid, false, true, v->address);
                    }
                }
                v->update(true, t->return_own_id());
            }

            /**
             * \brief creates a new variable object (is called, when var is read or written for the first time)
             * \note Invariant: var object is locked
             */
            auto create_var(size_t addr, size_t size){
                uint16_t u16_size = static_cast<uint16_t>(size);

                auto var = std::allocate_shared<VarState, c_alloc<VarState>>(c_alloc<VarState>(), addr, u16_size);
                return vars.insert({ addr, var }).first;
            }

            ///creates a new lock object (is called when a lock is acquired or released for the first time)
            auto createLock(void* mutex){
                auto lock = std::allocate_shared<VectorClock<>, c_alloc<VectorClock<>>>(c_alloc<VectorClock<>>());
                return locks.insert({ mutex, lock }).first;
            }

            ///creates a new thread object (is called when fork() called)
            ThreadState * create_thread(VectorClock<>::TID tid, ts_ptr parent = nullptr){
                ts_ptr new_thread;

                if (parent == nullptr) {
                    new_thread = std::make_shared<ThreadState>(tid); 
                    threads.insert( { tid, new_thread });
                }
                else {
                    new_thread = std::make_shared<ThreadState>(tid, parent);
                    threads.insert({ tid, new_thread });
                }

                return new_thread.get();
            }


            ///creates a happens_before object
            auto create_happens(void* identifier){
                auto new_hp = std::allocate_shared<VectorClock<>, c_alloc<VectorClock<>>>(c_alloc<VectorClock<>>());               
                return happens_states.insert({ identifier, new_hp }).first;
            }

            ///creates an allocation object
            void create_alloc(size_t addr, size_t size){
                allocs.insert({ addr, size });
            }

            void process_log_output(){
                double read_actions, write_actions;
                //percentages
                double rd, wr, r_ex_se, r_sh_se, r_ex, r_share, r_shared, w_se, w_ex, w_sh;
                
                read_actions  = log_count.read_ex_same_epoch + log_count.read_sh_same_epoch + log_count.read_exclusive + log_count.read_share + log_count.read_shared;
                write_actions = log_count.write_same_epoch + log_count.write_exclusive + log_count.write_shared;
                
                rd    = (read_actions/(read_actions + write_actions))*100;
                wr   = 100 - rd;
                r_ex_se = (log_count.read_ex_same_epoch / read_actions)*100;
                r_sh_se = (log_count.read_sh_same_epoch / read_actions)*100;
                r_ex = (log_count.read_exclusive / read_actions)*100;
                r_share = (log_count.read_share / read_actions)*100;
                r_shared = (log_count.read_shared / read_actions)*100;
                w_se = (log_count.write_same_epoch / write_actions)*100;
                w_ex = (log_count.write_exclusive / write_actions)*100;
                w_sh = (log_count.write_shared / write_actions)*100;

                std::cout << "FASTTRACK_STATISTICS: All values are percentages!" << std::endl;                
                std::cout << std::fixed << std::setprecision(2) << "Read Actions: " << rd << std::endl;
                std::cout << "Of which: " << std::endl;
                std::cout << std::fixed << std::setprecision(2) << "Read exclusive same epoch: " << r_ex_se << std::endl;
                std::cout << std::fixed << std::setprecision(2) << "Read shared same epoch: " << r_sh_se<< std::endl;
                std::cout << std::fixed << std::setprecision(2) << "Read exclusive: " << r_ex << std::endl;
                std::cout << std::fixed << std::setprecision(2) << "Read share: " << r_share << std::endl;
                std::cout << std::fixed << std::setprecision(2) << "Read shared: " << r_shared << std::endl;
                std::cout << std::endl;
                std::cout << std::fixed << std::setprecision(2) << "Write Actions: " << wr << std::endl; 
                std::cout << "Of which: " << std::endl;
                std::cout << std::fixed << std::setprecision(2) << "Write same epoch: " << w_se << std::endl;
                std::cout << std::fixed << std::setprecision(2) << "Write exclusive: " << w_ex << std::endl;
                std::cout << std::fixed << std::setprecision(2) << "Write shared: " << w_sh << std::endl;

            }

        public:
            Fasttrack() = default;

            /**
             * \brief deletes all data which is related to the tid
             * 
             * is called when a thread finishes (either from \ref join() or from \ref finish())
             * 
             * \note Not Threadsafe
             */
            void cleanup(uint32_t tid) {
                {
                    for (auto it = locks.begin(); it != locks.end(); ++it) {
                        it->second->delete_vc(tid);
                    }

                    for (auto it = threads.begin(); it != threads.end(); ++it) {
                        it->second->delete_vc(tid);
                    }

                    for (auto it = happens_states.begin(); it != happens_states.end(); ++it) {
                        it->second->delete_vc(tid);
                    }
                }
            }

            void parse_args(int argc, const char ** argv) {
                int processed = 1;
                while (processed < argc) {
                    if (strcmp(argv[processed], "--stats") == 0) {
                        log_flag = true;
                        return;
                    }
                    else {
                        ++processed;
                    }
                }
            }

            bool init(int argc, const char **argv, Callback rc_clb) final{
                parse_args(argc, argv);
                clb = rc_clb; //init callback
                return true;
            }

            void finalize() final {
                
                vars.clear();
                locks.clear();
                happens_states.clear();
                allocs.clear();
                while (threads.size() != 0) {
                    threads.erase(threads.begin());
                }
                
                if(log_flag){
                    process_log_output();
                }

            }

            void read(tls_t tls, void* pc, void* addr, size_t size) final
            {
                vs_ptr var;
                ThreadState * thr = reinterpret_cast<ThreadState*>(tls);
                thr->get_stackDepot().set_read_write((size_t)(addr), reinterpret_cast<size_t>(pc));

                {
                    // TODO: get rid of this lock (or use spinlock)!
                    std::lock_guard<LockT> exLockT(g_lock);
                    auto it = vars.find((size_t)(addr));
                    if (it == vars.end()) { //create new variable if new
                    #if MAKE_OUTPUT
                        std::cout << "variable is read before written" << std::endl;//warning
                    #endif
                        var = (create_var((size_t)(addr), size))->second;
                    }
                    else {
                        var = it->second;
                    }
                }

                read(thr, var);
            }

            void write(tls_t tls, void* pc, void* addr, size_t size) final {
                vs_ptr var;
                ThreadState * thr = reinterpret_cast<ThreadState*>(tls);
                thr->get_stackDepot().set_read_write((size_t)addr, reinterpret_cast<size_t>(pc));
                {
                    // TODO: get rid of this lock!
                    std::lock_guard<LockT> exLockT(g_lock);
                    auto it = vars.find((size_t)addr);//vars[(size_t)addr];
                    if (it == vars.end()) {
                        var = (create_var((size_t)(addr), size))->second;
                    }
                    else {
                        var = it->second;
                    }
                }
                write(thr, var); //func is thread_safe     
            }

            void func_enter(tls_t tls, void* pc) final {
                ThreadState * thr = reinterpret_cast<ThreadState*>(tls);
                thr->get_stackDepot().push_stack_element(reinterpret_cast<size_t>(pc));
            }

            void func_exit(tls_t tls) final {
                ThreadState * thr = reinterpret_cast<ThreadState*>(tls);
                thr->get_stackDepot().pop_stack_element();
            }

            void fork(tid_t parent, tid_t child, tls_t * tls) final{
                ThreadState * thr;

                std::lock_guard<LockT> exLockT(g_lock);
                if (threads.find(parent) != threads.end()) {
                    {
                        threads[parent]->inc_vc(); //inc vector clock for creation of new thread
                    }
                    thr = create_thread(child, threads[parent]);
                }
                else {
                    thr = create_thread(child);
                }
                *tls = reinterpret_cast<void*>(thr);
            }

            void join(tid_t parent, tid_t child) final{
                std::lock_guard<LockT> exLockT(g_lock);
                ts_ptr del_thread = threads[child];
                del_thread->inc_vc();
                //pass incremented clock of deleted thread to parent
                threads[parent]->update(del_thread);
                del_thread->delete_vector();

                threads.erase(child);
                cleanup(child);
            }

            //sync thread vc to lock vc 
            void acquire(tls_t tls, void* mutex, int recursive, bool write) final{

                if(recursive > 1){ //lock haven't been updated by another thread (by definition) 
                    return;        //therefore no action needed here as only the invoking thread may have updated the lock
                }
                
                std::shared_ptr<VectorClock<>> lock;
                {
                    std::lock_guard<LockT> exLockT(g_lock);
                    auto it = locks.find(mutex);
                    if (it == locks.end()) {
                        lock = createLock(mutex)->second;
                    }
                    else {
                        lock = it->second;
                    }
                }
                ThreadState * thr = reinterpret_cast<ThreadState*>(tls);   
                (thr)->update(lock);
            }

            void release(tls_t tls, void* mutex, bool write) final 
            {
                std::shared_ptr<VectorClock<>> lock;

                {
                    std::lock_guard<LockT> exLockT(g_lock);
                    auto it = locks.find(mutex);
                    if (it == locks.end()) {
                    #if MAKE_OUTPUT
                        std::cerr << "lock is released but was never acquired by any thread" << std::endl;
                    #endif
                        createLock(mutex)->second;
                        return;//as lock is empty (was never acquired), we can return here
                    }
                    else {
                        lock = it->second;
                    }
                }
                ThreadState * thr = reinterpret_cast<ThreadState*>(tls);

                (thr)->inc_vc();
                
                //increase vector clock and propagate to lock    
                (lock)->update(thr);
            }

            void happens_before(tls_t tls, void* identifier) final {
                std::shared_ptr<VectorClock<>> hp;

                {
                    std::lock_guard<LockT> exLockT(g_lock);
                    auto it = happens_states.find(identifier);
                    if (it == happens_states.end()) {
                        hp = create_happens(identifier)->second;
                    }
                    else {
                        hp = it->second;
                    }
                }
            
                ThreadState * thr = reinterpret_cast<ThreadState*>(tls);

                thr->inc_vc();//increment clock of thread and update happens state
                hp->update(thr->get_tid(), thr->return_own_id());
            }

            void happens_after(tls_t tls, void* identifier) final{
                std::shared_ptr<VectorClock<>> hp;
                {
                    std::lock_guard<LockT> exLockT(g_lock);
                    auto it = happens_states.find(identifier);
                    if (it == happens_states.end()) {
                        create_happens(identifier);
                        return;//create -> no happens_before can be synced
                    }
                    else {
                        hp = it->second;
                    }
                }

                ThreadState * thr = reinterpret_cast<ThreadState*>(tls);
                //update vector clock of thread with happened before clocks
                thr->update(hp);                
            }

            void allocate(tls_t  tls, void*  pc, void*  addr, size_t size) final{
                #if REGARD_ALLOCS
                size_t address = reinterpret_cast<size_t>(addr);
                std::lock_guard<LockT> exLockT(g_lock);
                if (allocs.find(address) == allocs.end()) {
                    create_alloc(address, size);
                }
                #endif
            }

            void deallocate( tls_t tls, void* addr) final
            {
                #if REGARD_ALLOCS
                size_t address = reinterpret_cast<size_t>(addr);
                size_t end_addr;

                std::lock_guard<LockT> exLockT(g_lock);
                end_addr = address + allocs[address];

                //variable is deallocated so varstate objects can be destroyed
                while (address < end_addr) {
                    if (vars.find(address) != vars.end()) {
                        vs_ptr var = vars.find(address)->second;
                        vars.erase(address);
                        address++;
                    }
                    else {
                        address++;
                    }
                }
                allocs.erase(address);
                #endif
            }

            void detach(tls_t tls, tid_t thread_id) final{
                /// \todo This is currently not supported
                return;
            }

            void finish(tls_t tls, tid_t thread_id) final
            {
                std::lock_guard<LockT> exLockT(g_lock);
                ///just delete thread from list, no backward sync needed
                threads[thread_id]->delete_vector();
                threads.erase(thread_id);
                cleanup(thread_id);
            }

            /**
             * \note allocation of shadow memory is not required in this
             *       detector. If a standalone version of Fasttrack is used
             *       this function can be ignored
             */
            void map_shadow(void* startaddr, size_t size_in_bytes) final{
                return;
            }

            const char * name() final{
                return "FASTTRACK";
            }

            const char * version() final{
                return "0.0.1";
            }
        };
    }
}

#endif // !FASTTRACK_H
