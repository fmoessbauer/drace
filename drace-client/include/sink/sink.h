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

#include "../race/DecoratedRace.h"

namespace drace {
    namespace sink {
        /**
        * \brief Generic interface of a data-race sink
        */
        class Sink {
        public:
            virtual void process_single_race(const race::DecoratedRace & race) = 0;
            virtual void process_all(const std::vector<race::DecoratedRace> & races) = 0;
        };
    }
}
