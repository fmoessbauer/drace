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

class Load_Save
{
public:
    Load_Save();

    bool load(std::string path, std::string content);
    bool save(std::string path, std::string content);
};

#endif // LOAD_SAVE_H
