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

/// Utilities and interfaces that are used in multiple components
namespace util
{
    /**
    * \brief RAII wrapper around a native module
    *
    * Abstract class providing an abstraction to load shared modules.
    * The LibraryLoader has to be referenced while the wrapped module is in use.
    * On destruction, the wrapped module is released.
    */
    class LibraryLoader {
    public:
        /// Construct loader but do not load any library
        LibraryLoader() = default;

        /// Construct loader and load requested library
        explicit LibraryLoader(const char * filename)
        { }

        /// Destruct loader and release wrapped module
        ~LibraryLoader()
        { }

        /**
         * load a module
         * \return true if module was loaded
         */
        bool load(const char * filename);

        /**
         * unload the loaded module
         * \return true if a module was loaded and is now unloaded.
         */
        bool unload();

        /**
         * \return true if module is loaded
         */
        bool loaded();

        /// Get a pointer to the requested function in this module
        ProcedurePtr operator[](const char * proc_name) const;

    };
}
