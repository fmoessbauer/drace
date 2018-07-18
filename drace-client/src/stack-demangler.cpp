#include "stack-demangler.h"
#include "symbols.h"

#include <iostream>
#include <string>

std::string stack_demangler::demangle(void *stack) {
	// get top element of stack
	app_pc top = *(app_pc *)stack;
	std::string info = symbols::get_symbol_info(top);
	return info;
}
