#pragma once

#include "../globals.h"
#include <dr_api.h>

namespace module {
	/* Encapsulates and enriches a dynamorio module_data_t struct */
	class Metadata {
	public:
		/* Flags describing characteristics of a module */
		enum MOD_TYPE_FLAGS : uint8_t {
			// no information available
			UNKNOWN = 0x0,
			// native module
			NATIVE  = 0x1,
			// managed module
			MANAGED = 0x2,
			// this (managed module) contains sync mechanisms
			SYNC    = 0x4 | MANAGED
		};

	public:
		app_pc base;
		app_pc end;
		bool   loaded; // This is necessary to modify value in-place in set
		INSTR_FLAGS instrument;
		MOD_TYPE_FLAGS modtype{ MOD_TYPE_FLAGS::UNKNOWN };
		module_data_t *info{ nullptr };
		bool   debug_info;

	private:
		/*
		* Determines if the module is a managed-code module
		* using the PE header
		*/
		void tag_module();

	public:

		Metadata(app_pc mbase, app_pc mend, bool mloaded = true, bool debug_info = false) :
			base(mbase),
			end(mend),
			loaded(mloaded),
			instrument(INSTR_FLAGS::NONE),
			debug_info(debug_info) { }

		~Metadata() {
			if (info != nullptr) {
				dr_free_module_data(info);
				info = nullptr;
			}
		}

		/* Copy constructor, duplicates dr module data */
		Metadata(const Metadata & other) :
			base(other.base),
			end(other.end),
			loaded(other.loaded),
			instrument(other.instrument),
			modtype(other.modtype),
			debug_info(other.debug_info)
		{
			info = dr_copy_module_data(other.info);
		}

		/* Move constructor, moves dr module data */
		Metadata(Metadata && other) :
			base(other.base),
			end(other.end),
			loaded(other.loaded),
			instrument(other.instrument),
			info(other.info),
			modtype(other.modtype),
			debug_info(other.debug_info)
		{
			other.info = nullptr;
		}

		void set_info(const module_data_t * mod) {
			info = dr_copy_module_data(mod);
			tag_module();
		}

		inline Metadata & operator=(const Metadata & other) {
			if (this == &other) return *this;
			dr_free_module_data(info);
			base = other.base;
			end = other.end;
			loaded = other.loaded;
			instrument = other.instrument;
			info = dr_copy_module_data(other.info);
			modtype = other.modtype;
			debug_info = other.debug_info;
			return *this;
		}

		inline Metadata & operator=(Metadata && other) {
			if (this == &other) return *this;
			dr_free_module_data(info);
			base = other.base;
			end = other.end;
			loaded = other.loaded;
			instrument = other.instrument;
			info = other.info;
			modtype = other.modtype;
			debug_info = other.debug_info;

			other.info = nullptr;
			return *this;
		}

		inline bool operator==(const Metadata & other) const {
			return (base == other.base);
		}
		inline bool operator!=(const Metadata & other) const {
			return !(base == other.base);
		}
		inline bool operator<(const Metadata & other) const {
			return (base < other.base);
		}
		inline bool operator>(const Metadata & other) const {
			return (base > other.base);
		}
	};
}