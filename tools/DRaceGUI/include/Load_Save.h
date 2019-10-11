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
#include "Report_Handler.h"
#include "Command_Handler.h"
#include "Data_Handler.h"

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>


class Load_Save
{
    Report_Handler* rh;
    Command_Handler* ch;

public:
    Load_Save(Report_Handler* r, Command_Handler* c);

    bool load(std::string path);
    void save(std::string path);
};

#endif // LOAD_SAVE_H
