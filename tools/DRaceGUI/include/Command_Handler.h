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

#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include <QString>
#include <QRegExp>
#include <QList>
#include <QDir>
#include "Executor.h"

class Command_Handler {
public:
    static constexpr uint DYNAMORIO = 0;
    static constexpr uint DR_DEBUG = 1;
    static constexpr uint DRACE = 2;
    static constexpr uint DETECTOR = 3;
    static constexpr uint FLAGS = 4;
    static constexpr uint REPORT_FLAG = 5;
    static constexpr uint EXT_CTRL = 6;
    static constexpr uint CONFIGURATION = 7;
    static constexpr uint EXECUTABLE = 8;
    static constexpr uint REPORT_CMD = 9;

private:
    QString command[(REPORT_CMD + 1)];
    QString entire_command;
    QString msr_path;
    QString eval_msr_path(QObject* parent);
    void make_extctrl(bool set);

public:
    Command_Handler(QString default_detector = "tsan");

    QString updateCommand(const QString &arg1, uint position);
    QString make_flag_entry(const QString & arg1);
    QString make_entry(const QString & path, uint postion, QString prefix = "", bool no_quotes = false);
    QString make_command();
    QString get_command() { return entire_command; }
    QString get_msr_path() { return msr_path; }

    bool command_is_valid();

    bool validate_and_set_msr(bool on, QObject* parent);

};


#endif // !COMMAND_HANDLER_H
