#include "stack-demangler.h"
#include "symbols.h"
#include "drace-client.h"

#include <iostream>
#include <string>

#include <dr_api.h>

void stack_demangler::demangle(void *stack) {
	// get top element of stack
	app_pc top = *(app_pc *)stack;
	std::string info = symbols::get_symbol_info(top);
	std::cout << info << std::endl;

	void *drcontext = dr_get_current_drcontext();
	thread_id_t tid = dr_get_thread_id(drcontext);
	for (const auto th : TLS_buckets) {
		std::cout << "NUM REFS ON " << th.first << ": " << th.second->num_refs << std::endl;
	}
}
