#include "drace-client.h"
#include "stack-demangler.h"
#include "symbols.h"

#include <iostream>
#include <string>

SymbolLocation stack_demangler::demangle(void *stack) {
	// get top element of stack
	app_pc top = *(app_pc *)stack;
	return symbol_table->get_symbol_info(top);
}
