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

void Executor::execute(QObject* parent, Command_Handler * ch, Report_Handler * rh)
{
    std::string dr_command = (ch->make_command()).toStdString();
    std::string rep_command = "";
    if (rh->command_valid()) {
        rep_command = rh->get_report_command();
    }

    std::string cmd = "start powershell -NoExit " + dr_command + ";" + rep_command;
    system(cmd.c_str());

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


