
/*
 * DRace, a dynamic data race detector
 *
 * Copyright 2018 Siemens AG
 *
 * Authors:
 *   Philip Harr <philip.harr@siemens.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef RACEFILTER_H
#define RACEFILTER_H

#include <vector>
#include <string>
#include <detector/Detector.h>
#include <race/DecoratedRace.h>

namespace drace{

    class  RaceFilter{
        std::vector<std::string> filter_list;
        void normalize_string(std::string & expr);
    public:
        RaceFilter(std::string filename);
        bool check_suppress(const drace::race::DecoratedRace & race);
        void print_list();

    };

}

#endif //RACEFILTER_H