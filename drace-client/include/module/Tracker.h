#pragma once

#include "globals.h"
#include "Metadata.h"

#include <dr_api.h>
#include <map>
#include <memory>

namespace module {
	class Tracker {
		// as we use lower_bound search, we have to reverse the sorting
		using map_t = std::map<app_pc, std::shared_ptr<Metadata>, std::greater<app_pc>>;

		// RW mutex for access of modules container
		void *mod_lock;
		std::shared_ptr<Symbols> _syms;
		map_t _modules_idx;

		bool _dotnet_rt_ready = false;

	public:
		using PMetadata = std::shared_ptr<Metadata>;

		std::vector<std::string> excluded_mods;
		std::vector<std::string> excluded_path_prefix;

	public:
		explicit Tracker(const std::shared_ptr<Symbols> & syms);
		~Tracker();

		/* Returns a shared_ptr to the module which contains the given program counter.
		 * If the pc is not in a known module, returns a nullptr
		 */
		PMetadata get_module_containing(const app_pc pc) const;

		/* Registers a new module by moving it */
		inline PMetadata add(Metadata && mod) {
			PMetadata ptr = std::make_shared<Metadata>(mod);
			return ptr;
		}

		/* Registers a new module by copying it */
		inline PMetadata add(const Metadata & mod) {
			PMetadata ptr = std::make_shared<Metadata>(mod);
			return ptr;
		}

		/* Creates new module in place */
		template<class... Args>
		inline PMetadata add_emplace(Args&&... args) {
			PMetadata ptr = std::make_shared<Metadata>(std::forward<Args>(args)...);
			_modules_idx.emplace(ptr->base, ptr);
			return ptr;
		}

		/* Registers a module and sets flags accordingly */
		PMetadata register_module(const module_data_t * mod, bool loaded);

		inline bool dotnet_rt_ready() const {
			return _dotnet_rt_ready;
		}

		/* Request a read-lock for the module dataset*/
		inline void lock_read() const {
			dr_rwlock_read_lock(mod_lock);
		}

		inline void unlock_read() const {
			dr_rwlock_read_unlock(mod_lock);
		}

		/* Request a write-lock for the module dataset*/
		inline void lock_write() const {
			dr_rwlock_write_lock(mod_lock);
		}

		inline void unlock_write() const {
			dr_rwlock_write_unlock(mod_lock);
		}
	};

	static void event_module_load(void *drcontext, const module_data_t *mod, bool loaded);
	static void event_module_unload(void *drcontext, const module_data_t *mod);

} // namespace module
