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

#ifndef DATA_HANDLER_H
#define DATA_HANDLER_H

#include <string>
#include <QString>
#include "Report_Handler.h"
#include "Command_Handler.h"

class Data_Handler {

    std::string report_name,
        report_converter,
        dynamorio,
        dr_debug,
        drace,
        detector,
        flags,
        ext_ctrl,
        configuration,
        execuatable,
        msr;

    bool report_is_python,
        create_report;

    friend class boost::serialization::access;

    ///function for the serialization of the class members
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & report_name;
        ar & report_converter;
        ar & dynamorio;
        ar & dr_debug;
        ar & drace;
        ar & detector;
        ar & flags;
        ar & ext_ctrl;
        ar & configuration;
        ar & execuatable;
        ar & msr;
        ar & report_is_python;
        ar & create_report;
    }

public:
    ///loads data from handler classes to members
    void get_data(Report_Handler* rh, Command_Handler* ch);

    ///restores data from members back in the handler classes
    void set_data(Report_Handler* rh, Command_Handler* ch);

};
#endif // !DATA_HANDLER_H
