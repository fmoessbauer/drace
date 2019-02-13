/*
 * DRace, a dynamic data race detector
 *
 * Copyright (c) Siemens AG, 2018
 *
 * Authors:
 *   Felix Moessbauer <felix.moessbauer@siemens.com>
 *
 * This work is licensed under the terms of the MIT license.  See
 * the LICENSE file in the top-level directory.
 */

#include "module/Metadata.h"
#include <windows.h>

/* Global operator to compare module_data_t regarding logic equivalence */
static bool operator==(const module_data_t & d1, const module_data_t & d2)
{
	if (&d1 == &d2) { return true; }

	if (d1.start == d2.start &&
		d1.end == d2.end   &&
		d1.entry_point == d2.entry_point &&
#ifdef WINDOWS
		d1.checksum == d2.checksum  &&
		d1.timestamp == d2.timestamp &&
#endif
		/* treat two modules without name (there are some) as different */
		dr_module_preferred_name(&d1) != NULL &&
		dr_module_preferred_name(&d2) != NULL &&
		strcmp(dr_module_preferred_name(&d1),
			dr_module_preferred_name(&d2)) == 0)
		return true;
	return false;
}
/* Global operator to compare module_data_t regarding logic inequivalence */
static bool operator!=(const module_data_t & d1, const module_data_t & d2) {
	return !(d1 == d2);
}

namespace drace {
	namespace module {
		void Metadata::tag_module() {
			// We want to read the COM_ENTRY from the data directory
			// according to https://docs.microsoft.com/en-us/windows/desktop/api/winnt/ns-winnt-_image_data_directory
			PIMAGE_DOS_HEADER      pidh = (PIMAGE_DOS_HEADER)info->start;
			PIMAGE_NT_HEADERS      pinh = (PIMAGE_NT_HEADERS)((BYTE*)pidh + pidh->e_lfanew);
			//PIMAGE_FILE_HEADER     pifh = (PIMAGE_FILE_HEADER)&pinh->FileHeader;
			PIMAGE_OPTIONAL_HEADER pioh = (PIMAGE_OPTIONAL_HEADER)&pinh->OptionalHeader;

			DWORD clrh = pioh->DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress;
			modtype = (clrh == 0) ? MOD_TYPE_FLAGS::NATIVE : MOD_TYPE_FLAGS::MANAGED;
		}
	}
}