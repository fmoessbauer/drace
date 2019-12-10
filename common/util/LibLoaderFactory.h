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

#include <memory>

#include "LibraryLoader.h"
#ifdef WIN32
#include "WindowsLibLoader.h"
#else
#include "UnixLibLoader.h"
#endif

namespace util
{
    class LibLoaderFactory {
        public:
        static std::shared_ptr<LibraryLoader> getLoader() {
            #ifdef WIN32
            return std::make_shared<WindowsLibLoader>();
            #else
            return std::make_shared<UnixLibLoader>();
            #endif
        }
    };
} // namespace util
