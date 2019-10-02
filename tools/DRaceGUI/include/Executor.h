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

#ifndef EXECUTOR_H
#define EXECUTOR_H
#include <string>
#include <QString>
#include <QProcess>
#include "Command_Handler.h"
#include "Report_Handler.h"


class Executor {


public:
    Executor();
    void execute(QObject* parent, Command_Handler * ch, Report_Handler * exe);
    void exe_custom(std::string cmd, QObject* parent);
    bool exe_drrun(QString cmd, QObject* parent);


};




#endif // !EXECUTOR_H
