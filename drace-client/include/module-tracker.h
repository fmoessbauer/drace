#pragma once

#include <dr_api.h>

class ModuleData {
public:
	app_pc base;
	app_pc end;
	mutable bool   loaded; // This is necessary to modify value in-place in set
	mutable bool   instrument;
	module_data_t *info = nullptr;

	ModuleData(app_pc mbase, app_pc mend, bool mloaded = true) :
		base(mbase),
		end(mend),
		loaded(mloaded),
		instrument(false) { }

	~ModuleData() {
		if (info != nullptr) {
			dr_free_module_data(info);
			info = nullptr;
		}
	}

	/* Copy constructor, duplicates dr module data */
	ModuleData(const ModuleData & other) :
		base(other.base),
		end(other.end),
		loaded(other.loaded),
		instrument(other.instrument)
	{
		info = dr_copy_module_data(other.info);
	}

	/* Move constructor, moves dr module data */
	ModuleData(ModuleData && other) :
		base(other.base),
		end(other.end),
		loaded(other.loaded),
		instrument(other.instrument),
		info(other.info)
	{
		other.info = nullptr;
	}

	void set_info(const module_data_t * mod) {
		info = dr_copy_module_data(mod);
	}

	inline bool operator==(const ModuleData & other) const {
		return (base == other.base);
	}
	inline bool operator!=(const ModuleData & other) const {
		return !(base == other.base);
	}
	inline bool operator<(const ModuleData & other) const {
		return (base < other.base);
	}
	inline bool operator>(const ModuleData & other) const {
		return (base > other.base);
	}
};

namespace module_tracker {

	// RW mutex for access of modules container
	extern void *mod_lock;

	void init();
	void finalize();

	bool register_events();

	static void event_module_load(void *drcontext, const module_data_t *mod, bool loaded);
	static void event_module_unload(void *drcontext, const module_data_t *mod);
}
