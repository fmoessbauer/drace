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
#include <iostream>
#include <detector/Detector.h>
#include "threadstate.h"
#include "varstate.h"
#include "stacktrace.h"
#include "xvector.h"
#include "parallel_hashmap/phmap.h"

#define MAKE_OUTPUT false
#define REGARD_ALLOCS true

namespace drace {
    namespace detector {

        template<class LockT>
        class Fasttrack : public Detector {
        
        public:
            typedef size_t tid_ft;
            //make some shared pointers a bit more handy
            typedef std::shared_ptr<ThreadState> ts_ptr;
            typedef std::shared_ptr<VarState> vs_ptr;
            
        private:

            ///these maps hold the various state objects together with the identifiers

            phmap::parallel_node_hash_map<size_t, size_t> allocs;
            phmap::parallel_node_hash_map<size_t, vs_ptr> vars;
            phmap::parallel_node_hash_map<void*, std::shared_ptr<VectorClock<>>> locks;
            phmap::parallel_node_hash_map<tid_ft, ts_ptr> threads;
            phmap::parallel_node_hash_map<void*, std::shared_ptr<VectorClock<>>> happens_states;
            phmap::parallel_node_hash_map<size_t, std::shared_ptr<StackTrace>> traces;
        
            ///holds the callback address to report a race to the drace-main 
            void * clb;

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
            LockT t_insert; ///belongs to thread map
            LockT v_insert; ///belongs to var map
            LockT s_insert; ///belongs to traces map (stacktraces)
            LockT l_insert; ///belongs to lock map
            LockT h_insert; ///belongs to happens map

            ///lock for the accesses of the var and thread objects
            LockT t_LockT;
            ///locks for the accesses of the allocation objects
            LockT a_LockT;
            ///locks for the accesses of the stacktrace objects
            LockT s_LockT;

            void report_race(
                uint32_t thr1, uint32_t thr2,
                bool wr1, bool wr2,
                size_t var,
                uint32_t clk1, uint32_t clk2)
            {
                std::list<size_t> stack1, stack2;
                {
                    std::shared_ptr<StackTrace> s1;
                    std::shared_ptr<StackTrace> s2;
                    std::lock_guard<LockT> exLockT(s_LockT);
                    auto it = traces.find(thr1);
                    if (it != traces.end()) {
                        s1 = it->second;
                    }
                    else { //if thread is, because of finishing not in stack traces anymore, return
                        return;
                    }
                    it = traces.find(thr2);
                    if (it != traces.end()) {
                        s2 = it->second;
                    }
                    else {//if thread is, because of finishing not in stack traces anymore, return
                        return;
                    }

                    stack1 = s1->return_stack_trace(var);
                    stack2 = s2->return_stack_trace(var);
                }
                
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
             */
            void read(ts_ptr t, vs_ptr v){
                        
                if (t->return_own_id() == v->get_read_id()) {//read same epoch, same thread;
                    if(log_flag){
                        log_count.read_ex_same_epoch++;
                    }
                    return;
                }

                std::lock_guard<LockT> exLockT(t_LockT);
                size_t id = t->return_own_id();
                uint32_t tid = t->get_tid();
                
                if (v->is_read_shared()  && v->get_vc_by_thr(tid) == id) { //read shared same epoch
                    if(log_flag){
                        log_count.read_sh_same_epoch++;
                    }
                    return;
                }

                if (v->is_wr_race(t))
                { // write-read race
                    report_race(v->get_w_tid(), tid, true, false, v->address, v->get_w_clock(), t->get_clock());
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
             */
            void write(ts_ptr t, vs_ptr v){

                if (t->return_own_id() == v->get_write_id()){//write same epoch
                    if(log_flag){
                        log_count.write_same_epoch++;
                    }
                    return;
                }


                std::lock_guard<LockT> exLockT(t_LockT);

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
                    report_race(v->get_w_tid(), tid, true, true, v->address, v->get_w_clock(), t->get_clock());
                }

                if (! v->is_read_shared()) {
                    if(log_flag){
                        log_count.write_exclusive++;
                    }
                    if (v->is_rw_ex_race(t))// read-write race
                    {
                        report_race(v->get_r_tid(), t->get_tid(), false, true, v->address, v->get_r_clock(), t->get_clock());
                    }
                }
                else {//come here in read shared case
                    if(log_flag){
                        log_count.write_shared++;
                    }
                    uint32_t act_tid = v->is_rw_sh_race(t);
                    if (act_tid != 0) //read shared read-write race       
                    {
                        report_race(act_tid, tid, false, true, v->address, v->get_clock_by_thr(act_tid), t->get_clock());
                    }
                }
                v->update(true, t->return_own_id());
            }


            ///creates a new variable object (is called, when var is read or written for the first time)
            auto create_var(size_t addr, size_t size){
                uint16_t u16_size = static_cast<uint16_t>(size);

                auto var = std::make_shared<VarState>(addr, u16_size);
                return vars.insert({ addr, var }).first;
            }


            ///creates a new lock object (is called when a lock is acquired or released for the first time)
            auto createLockTock(void* mutex){
                auto lock = std::make_shared<VectorClock<>>();
                return locks.insert({ mutex, lock }).first;
            }

            ///creates a new thread object (is called when fork() called)
            void create_thread(VectorClock<>::TID tid, ts_ptr parent = nullptr){

                ts_ptr new_thread;
                if (parent == nullptr) {
                    new_thread = std::make_shared<ThreadState>(tid);
                    threads.insert( { tid, new_thread });
                }
                else {
                    std::lock_guard<LockT> exLockT(t_LockT);
                    new_thread = std::make_shared<ThreadState>(tid, parent);
                    threads.insert({ tid, new_thread });
                }

                //create also a object to save the stack traces
                std::lock_guard<LockT> exLockT(s_insert);
                traces.insert({ tid, std::make_shared<StackTrace>() });

            }


            ///creates a happens_before object
            auto create_happens(void* identifier){
                auto new_hp = std::make_shared<VectorClock<>>();
                return happens_states.insert({ identifier, new_hp }).first;
            }

            ///creates an allocation object
            void create_alloc(size_t addr, size_t size){
                allocs.insert({ addr, size });
            }

        public:
            Fasttrack() = default;


            /**
             * \brief deletes all data which is related to the tid
             * 
             * is called when a thread finishes (either from \ref join() or from \ref finish())
             */
            void cleanup(uint32_t tid) {
                {
                    std::lock_guard<LockT> exLockT(t_LockT);
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
                traces.clear();
                while (threads.size() != 0) {
                    threads.erase(threads.begin());
                }
                
                if(log_flag){
                    std::cout << "read excl. same epoch: " << log_count.read_ex_same_epoch << std::endl;
                    std::cout << "read shared same epoch: " << log_count.read_sh_same_epoch << std::endl;
                    std::cout << "read exclusive: " << log_count.read_exclusive << std::endl;
                    std::cout << "read share: " << log_count.read_share << std::endl;
                    std::cout << "read shared: " << log_count.read_shared << std::endl;
                    std::cout << "write same epoch: " << log_count.write_same_epoch << std::endl;
                    std::cout << "write exclusive: " << log_count.write_exclusive << std::endl;
                    std::cout << "write shared: " << log_count.write_shared << std::endl;
                }

            }

            void read(tls_t tls, void* pc, void* addr, size_t size) final
            {
                vs_ptr var;
                ts_ptr thr;
                {
                    std::shared_lock<LockT> shLockT(s_LockT);
                    auto stack = traces[reinterpret_cast<Fasttrack::tid_ft>(tls)];
                    stack->set_read_write((size_t)(addr), reinterpret_cast<size_t>(pc));
                }

                {
                    std::lock_guard<LockT> exLockT(v_insert);
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

                thr = threads[reinterpret_cast<Fasttrack::tid_ft>(tls)];
                read(thr, var);
            }

            void write(tls_t tls, void* pc, void* addr, size_t size) final {
                vs_ptr var;
                ts_ptr thr;
                {
                    std::shared_lock<LockT> shLockT(s_LockT);
                    auto stack = traces[reinterpret_cast<Fasttrack::tid_ft>(tls)];
                    stack->set_read_write((size_t)addr, reinterpret_cast<size_t>(pc));
                }
                {
                    std::lock_guard<LockT> exLockT(v_insert);
                    auto it = vars.find((size_t)addr);//vars[(size_t)addr];
                    if (it == vars.end()) {
                        var = (create_var((size_t)(addr), size))->second;
                    }
                    else {
                        var = it->second;
                    }
                }
                thr = threads[reinterpret_cast<Fasttrack::tid_ft>(tls)];
            
                write(thr, var); //func is thread_safe     
            }


            void func_enter(tls_t tls, void* pc) final {
                auto stack = traces[reinterpret_cast<Fasttrack::tid_ft>(tls)];
                size_t stack_element = reinterpret_cast<size_t>(pc);
                //shared lock is enough as a single thread cannot do more than one action (r,w,enter,exit...) at a time
                std::shared_lock<LockT> shLockT(s_LockT);
                stack->push_stack_element(stack_element);
            }

            void func_exit(tls_t tls) final {
                auto stack = traces[reinterpret_cast<Fasttrack::tid_ft>(tls)];
            
                //shared lock is enough as a single thread cannot do more than one action (r,w,enter,exit...) at a time
                std::shared_lock<LockT> shLockT(s_LockT);
                stack->pop_stack_element(); //pops last stack element of current clock
            }


            void fork(tid_t parent, tid_t child, tls_t * tls) final{
                Fasttrack::tid_ft child_size_t = child;
                *tls = reinterpret_cast<void*>(child_size_t);

                std::lock_guard<LockT> exLockT(t_insert);
                if (threads.find(parent) != threads.end()) {
                    {
                        threads[parent]->inc_vc(); //inc vector clock for creation of new thread
                    }
                    create_thread(child, threads[parent]);
                }
                else {
                    create_thread(child);
                }   
            }

            void join(tid_t parent, tid_t child) final{
                ts_ptr del_thread = threads[child];

                {
                    std::lock_guard<LockT> exLockT(t_LockT);
                    del_thread->inc_vc();
                    //pass incremented clock of deleted thread to parent
                    threads[parent]->update(del_thread);
                    del_thread->delete_vector();
                }
                {
                    std::lock_guard<LockT> exLockT(t_insert);
                    threads.erase(child);
                    cleanup(child);
                }
                std::lock_guard<LockT> exLockT(s_insert);
                traces.erase(child);
            }

            //sync thread vc to lock vc 
            void acquire(tls_t tls, void* mutex, int recursive, bool write) final{

                if(recursive > 1){ //lock haven't been updated by another thread (by definition) 
                    return;        //therefore no action needed here as only the invoking thread may have updated the lock
                }
                
                std::shared_ptr<VectorClock<>> lock;
                {
                    std::lock_guard<LockT> exLockT(l_insert);
                    auto it = locks.find(mutex);
                    if (it == locks.end()) {
                        lock = createLockTock(mutex)->second;
                    }
                    else {
                        lock = it->second;
                    }
                }
                auto id = reinterpret_cast<Fasttrack::tid_ft>(tls);
                auto thr = threads[id];

                std::lock_guard<LockT> exLockT(t_LockT);            
                (thr)->update(lock);
            }

            void release(tls_t tls, void* mutex, bool write) final 
            {
                std::shared_ptr<VectorClock<>> lock;

                {
                    std::lock_guard<LockT> exLockT(l_insert);
                    auto it = locks.find(mutex);
                    if (it == locks.end()) {
                    #if MAKE_OUTPUT
                        std::cerr << "lock is released but was never acquired by any thread" << std::endl;
                    #endif
                        createLockTock(mutex)->second;
                        return;//as lock is empty (was never acquired), we can return here
                    }
                    else {
                        lock = it->second;
                    }
                }
                tid_ft id = reinterpret_cast<tid_ft>(tls);
                auto thr = threads[id];
                
                std::lock_guard<LockT> exLockT(t_LockT);
                (thr)->inc_vc();
                
                //increase vector clock and propagate to lock    
                (lock)->update(thr);
            }

            void happens_before(tls_t tls, void* identifier) final {
                std::shared_ptr<VectorClock<>> hp;

                {
                    std::lock_guard<LockT> exLockT(h_insert);
                    auto it = happens_states.find(identifier);
                    if (it == happens_states.end()) {
                        hp = create_happens(identifier)->second;
                    }
                    else {
                        hp = it->second;
                    }
                }
            
                ts_ptr thr = threads[reinterpret_cast<Fasttrack::tid_ft>(tls)];

                std::lock_guard<LockT> exLockT(t_LockT);
                thr->inc_vc();//increment clock of thread and update happens state
                hp->update(thr->get_tid(), thr->return_own_id());
            }

            void happens_after(tls_t tls, void* identifier) final{
                std::shared_ptr<VectorClock<>> hp;
                {
                    std::lock_guard<LockT> exLockT(h_insert);
                    auto it = happens_states.find(identifier);
                    if (it == happens_states.end()) {
                        create_happens(identifier);
                        return;//create -> no happens_before can be synced
                    }
                    else {
                        hp = it->second;
                    }
                }
                ts_ptr thr = threads[reinterpret_cast<Fasttrack::tid_ft>(tls)];

                std::lock_guard<LockT> exLockT(t_LockT);
                //update vector clock of thread with happened before clocks
                thr->update(hp);                
            }

            void allocate(tls_t  tls, void*  pc, void*  addr, size_t size) final{
                #if REGARD_ALLOCS
                size_t address = reinterpret_cast<size_t>(addr);
                std::lock_guard<LockT> exLockT(a_LockT);
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
                {
                    std::shared_lock<LockT> shLockT(a_LockT);
                    end_addr = address + allocs[address];
                }

                //variable is deallocated so varstate objects can be destroyed
                while (address < end_addr) {
                    std::lock_guard<LockT> exLockT(t_LockT);
                    if (vars.find(address) != vars.end()) {
                        vs_ptr var = vars.find(address)->second;
                        vars.erase(address);
                        address++;
                    }
                    else {
                        address++;
                    }
                }
                std::lock_guard<LockT> exLockT(a_LockT);
                allocs.erase(address);
                #endif
            }

            void detach(tls_t tls, tid_t thread_id) final{
                /// \todo This is currently not supported
                return;
            }

            void finish(tls_t tls, tid_t thread_id) final
            {
                ///just delete thread from list, no backward sync needed
                {
                    std::lock_guard<LockT> exLockT(t_LockT);
                    threads[thread_id]->delete_vector();
                }
                {
                    std::lock_guard<LockT> exLockT(t_insert);
                    threads.erase(thread_id);
                    cleanup(thread_id);
                }
                std::lock_guard<LockT> exLockT(s_insert);
                traces.erase(thread_id);
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
