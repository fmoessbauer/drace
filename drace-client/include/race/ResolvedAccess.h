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
#include <detector/detector_if.h>

namespace drace {
    namespace race {
        /**
         * Race access entry with symbolized callstack information
         */
        class ResolvedAccess : public detector::AccessEntry {
        public:
            std::vector<SymbolLocation> resolved_stack;

            ResolvedAccess(const detector::AccessEntry & e)
                : detector::AccessEntry(e)
            {
                std::copy(e.stack_trace, e.stack_trace + e.stack_size, this->stack_trace);
            }
        };
    }
}
