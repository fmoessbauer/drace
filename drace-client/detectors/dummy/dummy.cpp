#include "../detector_if.h"
#include <string>

bool detector::init(int argc, const char **argv, Callback rc_clb) {
	return true;
}

void detector::finalize() { }

void detector::acquire(
	tid_t thread_id,
	void* mutex,
	int recursive,
	bool write,
	bool try_lock,
	tls_t tls) { }

/* Release a mutex */
void detector::release(
	tid_t thread_id,
	void* mutex,
	bool write,
	tls_t tls) { }

void detector::happens_before(tid_t thread_id, void* identifier) { }

void detector::happens_after(tid_t thread_id, void* identifier) { }

void detector::read(tid_t thread_id, void* pc, void* addr, size_t size, tls_t tls) { }

void detector::write(tid_t thread_id, void* pc, void* addr, size_t size, tls_t tls) { }

void detector::allocate(tid_t thread_id, void* pc, void* addr, size_t size, tls_t tls) { }

void detector::deallocate(tid_t thread_id, void* addr, tls_t tls) { }

void detector::fork(tid_t parent, tid_t child, tls_t tls) { }

void detector::join(tid_t parent, tid_t child, tls_t tls) { }

std::string detector::name() {
	return std::string("Dummy");
}

std::string detector::version() {
	return std::string("1.0.0");
}
