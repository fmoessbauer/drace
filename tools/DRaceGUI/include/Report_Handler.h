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
#include "Command_Handler.h"
#include <QProcess>
#include "Executor.h"

class Report_Handler
{
    ///Name of report converter
    static constexpr char* REP_CONV_DEFAULT = "ReportConverter";

    QString rep_conv_cmd = "";

    ///default report name
    QString rep_name = "test_report.xml";

    ///holds the state of the report check box
    bool create_report;

    ///true when Python script of report converter is set
    bool is_python;

    ///pointer to the command handler
    Command_Handler* ch;

    void find_report_converter(QObject* parent);

    QString make_command();

public:
    Report_Handler(Command_Handler* c_handler, QObject* parent);

    QString get_rep_conv_cmd() { return rep_conv_cmd; }
    QString get_rep_name() { return rep_name; }

    void set_report_name(QString name);

    bool set_report_command();

    bool eval_rep_conv(QString name, QObject* parent);

    bool command_valid();

    void set_create_state(bool state);
};

#endif 
