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

#include "Executor.h"

Executor::Executor()
{
}

void Executor::execute(QObject* parent, std::string cmd)//, Report_Handler * rh)
{
    std::string ps_cmd = "start powershell -NoExit " + cmd;
    system(ps_cmd.c_str());
}

void Executor::exe_custom(std::string cmd, QObject* parent)
{
}

bool Executor::exe_drrun(QString cmd, QObject* parent)
{
    if (cmd.endsWith("drrun") || cmd.endsWith("drrun.exe")) {
       
        QProcess *proc_ovpn = new QProcess(parent);
        proc_ovpn->start(cmd, QStringList() << "-version");
        proc_ovpn->setProcessChannelMode(QProcess::MergedChannels);

        if (proc_ovpn->waitForFinished()) {
            QString str(proc_ovpn->readAllStandardOutput());

            if (str.contains("drrun version 7.", Qt::CaseInsensitive)) {
                return true;
            }
        }
    }
    return false;
}

bool Executor::exe_python3(QObject* parent) {
   
    QProcess *proc_ovpn = new QProcess(parent);
    proc_ovpn->start("python", QStringList() << "-V");
    proc_ovpn->setProcessChannelMode(QProcess::MergedChannels);

    if (proc_ovpn->waitForFinished()) {
        QString str(proc_ovpn->readAllStandardOutput());

        if (str.contains("Python 3.", Qt::CaseInsensitive)) {
            return true;
        }
    }
    
    return false;
}

bool Executor::exe_msr(QString path, QObject* parent) {

    QProcess *proc_ovpn = new QProcess(parent);
    proc_ovpn->start(path, QStringList() << "--version");
    proc_ovpn->setProcessChannelMode(QProcess::MergedChannels);

    if (proc_ovpn->waitForFinished()) {
        QString str(proc_ovpn->readAllStandardOutput());

        if (str.contains("Managed Symbol Resolver", Qt::CaseInsensitive)) {
            return true;
        }
    }

    return false;
}

void Executor::launch_msr(std::string path) {

    std::string cmd = "start powershell -NoExit " + path + " --once";
    system(cmd.c_str());
    QThread::sleep(1);
}
