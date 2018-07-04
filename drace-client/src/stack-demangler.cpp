#include "stack-demangler.h"
#include "drace-client.h"
#include "module-tracker.h"
#include "symbols.h"

#include <iostream>
#include <string>
#include <dr_api.h>

void stack_demangler::demangle(void *stack) {
	// get top element of stack
	app_pc top = *(app_pc *)stack;
	std::string info = symbols::get_symbol_info(top);
	std::cout << info << std::endl;
}
