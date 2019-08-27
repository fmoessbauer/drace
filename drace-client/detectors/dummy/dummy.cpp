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

#include <string>

#include <detector/detector_if.h>

namespace detector
{
    class Dummy : public Detector
    {
        virtual bool init(int argc, const char **argv, Callback rc_clb) {
            return true;
        }

        virtual void finalize() { }

        virtual void func_enter(tls_t tls, void* pc) { }

        virtual void func_exit(tls_t tls) { }

        virtual void acquire(
            tls_t tls,
            void* mutex,
            int recursive,
            bool write) { }

        virtual void release(
            tls_t tls,
            void* mutex,
            bool write) { }

        virtual void happens_before(tls_t tls, void* identifier) { }

        virtual void happens_after(tls_t tls, void* identifier) { }

        virtual void read(tls_t tls, void* pc, void* addr, size_t size) { }

        virtual void write(tls_t tls, void* pc, void* addr, size_t size) { }

        virtual void allocate(tls_t tls, void* pc, void* addr, size_t size) { }

        virtual void deallocate(tls_t tls, void* addr) { }

        virtual void fork(tid_t parent, tid_t child, tls_t * tls) {
            *tls = (void*)child;
        }

        virtual void join(tid_t parent, tid_t child) { }

        virtual const char * name() {
            return "Dummy";
        }

        virtual const char * version() {
            return "1.0.0";
        }
    };
}
