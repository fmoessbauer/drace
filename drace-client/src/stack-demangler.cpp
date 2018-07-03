#include "stack-demangler.h"
#include "drace-client.h"
#include "module-tracker.h"
#include "symbols.h"

#include <string>
#include <dr_api.h>

void stack_demangler::demangle(void *stack) {
	// get top element of stack
	app_pc top = *(app_pc *)stack;

	// Create dummy shadow module
	module_tracker::module_info_t pc_mod(top, nullptr);
	
	auto pc_mod_it = modules.lower_bound(pc_mod);
	if ((pc_mod_it != modules.end()) && (top < pc_mod_it->end)) {
		// bb in known module
		std::string sym_name = symbols::get_bb_symbol(top);
		const char * mod_name = dr_module_preferred_name(pc_mod_it->info);
		dr_printf("PC %p is in Module %s - %s, from %p to %p\n",
			top, mod_name, sym_name.c_str(), pc_mod_it->base, pc_mod_it->end);
	}
	else {
		dr_printf("PC %p is in Unknown Module\n", top);
	}
}
