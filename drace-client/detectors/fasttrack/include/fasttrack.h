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
#include <ipc/spinlock.h>
//#include <ipc/DrLock.h>
#include "threadstate.h"
#include "varstate.h"
#include "stacktrace.h"
#include "xvector.h"
#include "ipc/xlock.h"
//#include <unordered_map>
#include "parallel_hashmap/phmap.h"

#define MAKE_OUTPUT true
#define REGARD_ALLOCS false


namespace drace {

    namespace detector {

        class Fasttrack : public Detector {
        public:
            typedef size_t tid_ft;
            //typedef DrLock rwlock;
            typedef xlock rwlock;
            
        private:    

            ///these maps hold the various state objects together with the identifiers

            phmap::parallel_node_hash_map<size_t, size_t> allocs;
            phmap::parallel_node_hash_map<size_t, std::shared_ptr<VarState>> vars;
            phmap::parallel_node_hash_map<void*, std::shared_ptr<VectorClock<>>> locks;
            phmap::parallel_node_hash_map<tid_ft, std::shared_ptr<ThreadState>> threads;
            phmap::parallel_node_hash_map<void*, std::shared_ptr<VectorClock<>>> happens_states;
            phmap::parallel_node_hash_map<size_t, std::shared_ptr<StackTrace>> traces;
        
            ///holds the callback address to report a race to the drace-main 
            void * clb;

            ///those locks secure the .find and .insert actions to the according hashmaps
            xlock t_insert; ///belongs to thread map
            xlock v_insert; ///belongs to var map
            xlock s_insert; ///belongs to traces map (stacktraces)
            xlock l_insert; ///belongs to lock map
            xlock h_insert; ///belongs to happens map

            ///lock for the accesses of the var and thread objects
            xlock t_lock;

            ///locks for the accesses of the allocation objects
            rwlock a_lock;

            ///locks for the accesses of the stacktrace objects
            rwlock s_lock;


            void report_race(
                uint32_t thr1, uint32_t thr2,
                bool wr1, bool wr2,
                size_t var,
                uint32_t clk1, uint32_t clk2);

            ///function takes care of a read access (works only on thread and var object, not on any list)
            void read(std::shared_ptr<ThreadState> t, std::shared_ptr<VarState> v);

            ///function takes care of a write access (works only on thread and var object, not on any list)
            void write(std::shared_ptr<ThreadState> t, std::shared_ptr<VarState> v);

            ///creates a new variable object (is called, when var is read or written for the first time)
            auto create_var(size_t addr, size_t size);

            ///creates a new lock object (is called when a lock is acquired or released for the first time)
            auto create_lock(void* mutex);

            ///creates a new thread object (is called when fork() called)
            void create_thread(Detector::tid_t tid, std::shared_ptr<ThreadState> parent = nullptr);

            ///creates a happens_before object
            auto create_happens(void* identifier);

            ///creates an allocation object
            void create_alloc(size_t addr, size_t size);

        public:
            Fasttrack() {};


            ///deletes all data which is related to the tid
            ///is called when a thread finishes (either join() or finish())
            ///is actually called from the threadstate destrutor
            void cleanup(uint32_t tid);

            //public functions are explained in the detector base class
            bool init(int argc, const char **argv, Callback rc_clb) final;

            void finalize() final;

            void read(tls_t tls, void* pc, void* addr, size_t size) final;

            void write(tls_t tls, void* pc, void* addr, size_t size) final;


            void func_enter(tls_t tls, void* pc) final;

            void func_exit(tls_t tls) final;


            void fork(tid_t parent, tid_t child, tls_t * tls) final;

            void join(tid_t parent, tid_t child) final;


            void acquire(tls_t tls, void* mutex, int recursive, bool write) final;

            void release(tls_t tls, void* mutex, bool write) final;


            void happens_before(tls_t tls, void* identifier) final;

            void happens_after(tls_t tls, void* identifier) final;

            void allocate(tls_t  tls, void*  pc, void*  addr, size_t size) final;

            void deallocate( tls_t tls, void* addr) final;

            void detach(tls_t tls, tid_t thread_id) final;

            void finish(tls_t tls, tid_t thread_id) final;

            void map_shadow(void* startaddr, size_t size_in_bytes) final;


            const char * name() final;

            const char * version() final;
        };

    }
}



#endif // !FASTTRACK_H
