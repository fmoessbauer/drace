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

#include <vector>

#include "symbols.h"
#include <detector/Detector.h>

namespace drace {
    namespace race {
        /**
         * \brief Race access entry with symbolized callstack information
         */
        class ResolvedAccess : public Detector::AccessEntry {
        public:
            std::vector<symbol::SymbolLocation> resolved_stack;

            explicit ResolvedAccess(const Detector::AccessEntry & e)
                : Detector::AccessEntry(e)
            {
                std::copy(e.stack_trace, e.stack_trace + e.stack_size, this->stack_trace);
            }
        };
    }
}
