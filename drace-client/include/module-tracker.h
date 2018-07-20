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

class ModuleTracker {
	// RW mutex for access of modules container
	void *mod_lock;

public:
	using ModuleData_Set = std::set<ModuleData, std::greater<ModuleData>>;
	ModuleData_Set modules;

	std::vector<std::string> excluded_mods;
	std::vector<std::string> excluded_path_prefix;

public:
	ModuleTracker();
	~ModuleTracker();

	bool register_events();

	std::pair<bool, ModuleData_Set::iterator> get_module_containing(app_pc pc) const {
		ModuleData current(pc, nullptr);

		auto m_it = modules.lower_bound(current);
		if (m_it != modules.end() && pc < m_it->end) {
			return std::make_pair(true, m_it);
		}
		else {
			return std::make_pair(false, modules.end());
		}
	}

	inline void lock_read() const {
		dr_rwlock_read_lock(mod_lock);
	}

	inline void unlock_read() const {
		dr_rwlock_read_unlock(mod_lock);
	}

	inline void lock_write() const {
		dr_rwlock_write_lock(mod_lock);
	}

	inline void unlock_write() const {
		dr_rwlock_write_unlock(mod_lock);
	}

private:
	void ModuleTracker::print_modules();
};

static void event_module_load(void *drcontext, const module_data_t *mod, bool loaded);
static void event_module_unload(void *drcontext, const module_data_t *mod);
