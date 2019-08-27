/*
 * DRace, a dynamic data race detector
 *
 * Copyright 2018 Siemens AG
 *
 * Authors:
 *   Felix Moessbauer <felix.moessbauer@siemens.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <type_traits>

#include "ProcedurePtr.h"

namespace util
{
    /**
    * Abstract class providing an abstraction to load shared modules
    */
    class LibraryLoader {
    public:
        LibraryLoader() = default;

        explicit LibraryLoader(const char * filename)
        { }

        ~LibraryLoader()
        { }

        bool load(const char * filename);
        bool unload();

        ProcedurePtr operator[](const char * proc_name) const;

    };
}
