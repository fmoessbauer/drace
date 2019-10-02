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

#ifndef LOAD_SAVE_H
#define LOAD_SAVE_H
#include <string>
#include <fstream>
#include <QString>
#include <vector>

#ifdef USE_BOOST
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#endif

class DRaceGUI;

class Load_Save
{
    std::string return_serialized_data(DRaceGUI* instance);
    bool set_data(std::vector<std::string> content, DRaceGUI* instance);
public:
    Load_Save();

    bool load(std::string path, DRaceGUI* instance);
    bool save(std::string path, DRaceGUI* instance);
};

#endif // LOAD_SAVE_H
