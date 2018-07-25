#include "globals.h"
#include "stack-demangler.h"
#include "symbols.h"

#include <iostream>
#include <string>

SymbolLocation stack_demangler::demangle(void *pc) {
	return symbol_table->get_symbol_info((app_pc)pc);
}
