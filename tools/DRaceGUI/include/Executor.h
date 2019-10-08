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


class Executor {


public:
    Executor();
    static void execute(QObject* parent, std::string cmd);//, Report_Handler * exe);
    static void exe_custom(std::string cmd, QObject* parent);
    static bool exe_drrun(QString cmd, QObject* parent);
    static bool exe_python3(QObject* parent);
    static bool exe_msr(QString path, QObject * parent);
};




#endif // !EXECUTOR_H
