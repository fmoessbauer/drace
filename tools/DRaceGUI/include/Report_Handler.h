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

#ifndef REPORT_HANDLER_H
#define REPORT_HANDLER_H

#include "QDirIterator"
#include <string>
#include <QString>

class Report_Handler
{
    static constexpr char* REP_CONV_DEFAULT = "ReportConverter";

    QString rep_conv_cmd = "";
    QString rep_name = "test_report.xml";

    void find_report_converter();

public:
    Report_Handler();

    QString get_rep_conv_cmd() { return rep_conv_cmd; }
    QString get_rep_name() { return rep_name; }

    bool set_report_converter(QString path);
    bool set_report_name(QString name);

    std::string get_report_command();

    bool eval_rep_conv(QString name);

    bool command_valid();

};

#endif 
