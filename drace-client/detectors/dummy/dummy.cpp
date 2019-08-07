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

bool detector::init(int argc, const char **argv, Callback rc_clb) {
    return true;
}

void detector::finalize() { }

void detector::func_enter(tls_t tls, void* pc) { }

void detector::func_exit(tls_t tls) { }

void detector::acquire(
    tls_t tls,
    void* mutex,
    int recursive,
    bool write) { }

void detector::release(
    tls_t tls,
    void* mutex,
    bool write) { }

void detector::happens_before(tls_t tls, void* identifier) { }

void detector::happens_after(tls_t tls, void* identifier) { }

void detector::read(tls_t tls, void* pc, void* addr, size_t size) { }

void detector::write(tls_t tls, void* pc, void* addr, size_t size) { }

void detector::allocate(tls_t tls, void* pc, void* addr, size_t size) { }

void detector::deallocate(tls_t tls, void* addr) { }

void detector::fork(tid_t parent, tid_t child, tls_t * tls) {
    *tls = (void*)child;
}

void detector::join(tid_t parent, tid_t child) { }

const char * detector::name() {
    return "Dummy";
}

const char * detector::version() {
    return "1.0.0";
}
