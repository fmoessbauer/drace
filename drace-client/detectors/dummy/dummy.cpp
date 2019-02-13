/*
 * DRace, a dynamic data race detector
 *
 * Copyright (c) Siemens AG, 2018
 *
 * Authors:
 *   Felix Moessbauer <felix.moessbauer@siemens.com>
 *
 * This work is licensed under the terms of the MIT license.  See
 * the LICENSE file in the top-level directory.
 */

#include <string>

#include <detector/detector_if.h>

bool detector::init(int argc, const char **argv, Callback rc_clb) {
	return true;
}

void detector::finalize() { }

void detector::acquire(
	tls_t tls,
	void* mutex,
	int recursive,
	bool write) { }

void detector::release(
	tls_t tls,
	void* mutex,
	bool write) { }

void detector::happens_before(tid_t thread_id, void* identifier) { }

void detector::happens_after(tid_t thread_id, void* identifier) { }

void detector::read(tls_t tls, void* callstack, unsigned stacksize, void* addr, size_t size) { }

void detector::write(tls_t tls, void* callstack, unsigned stacksize, void* addr, size_t size) { }

void detector::allocate(tls_t tls, void* pc, void* addr, size_t size) { }

void detector::deallocate(tls_t tls, void* addr) { }

void detector::fork(tid_t parent, tid_t child, tls_t * tls) {
	*tls = (void*)child;
}

void detector::join(tid_t parent, tid_t child, tls_t tls) { }

std::string detector::name() {
	return std::string("Dummy");
}

std::string detector::version() {
	return std::string("1.0.0");
}
