#pragma once
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
 
#include "LibraryLoader.h"
#include <dlfcn.h>

namespace util {
    class UnixLibLoader : public LibraryLoader
    {
        void* _lib{nullptr};

    public:

        UnixLibLoader() = default;

        /// LibraryLoader specialization for windows dlls
        explicit UnixLibLoader(const char * filename)
        {
            load(filename);
        }

        ~UnixLibLoader() final
        {
            unload();
        }

        /**
         * load a module
         * \return true if module was loaded
         */
        bool load(const char * filename) final
        {
            if (loaded())
                return false;

            _lib = dlopen(filename, RTLD_NOW);
            return nullptr != _lib;
        }

        /**
         * unload the loaded module
         * \return true if a module was loaded and is now unloaded.
         */
        bool unload() final
        {
            if (loaded())
            {
                dlclose(_lib);
                _lib = nullptr;
                return true;
            }
            return false;
        }

        /**
         * \return true if module is loaded
         */
        bool loaded() final
        {
            return _lib != nullptr;
        }

        /// Get a pointer to the requested function in this module
        ProcedurePtr operator[](const char * proc_name) const final
        {
            return ProcedurePtr(reinterpret_cast<void*>(dlsym(_lib, proc_name)));
        }
    };
}
