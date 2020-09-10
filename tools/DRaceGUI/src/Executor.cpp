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
#include <Windows.h>

void Executor::execute(std::string cmd, QObject* parent) {
  std::string shell = "powershell.exe";
  std::string ps_args = "-NoExit " + cmd;

  ShellExecute(NULL, NULL, shell.c_str(), ps_args.c_str(), NULL,
               SW_SHOWDEFAULT);
}

void Executor::execute_embedded(QProcess* proc, QString cmd, QObject* parent) {
  QString shell = "powershell.exe";
  QStringList ps_args;
  ps_args << "-Command" << cmd;
  proc->setProcessChannelMode(QProcess::MergedChannels);
  proc->setProgram(shell);
  proc->setArguments(ps_args);
  proc->start();
}

void Executor::stop_embedded(qint64 pid) {
  if (pid != 0) {
    std::string shell = "powershell.exe";
    std::string ps_args =
        "function Stop-Embedded { Param([int]$ppid)\nGet-CimInstance "
        "Win32_Process | "
        "Where-Object { $_.ParentProcessId -eq $ppid } | ForEach-Object { "
        "Stop-Embedded $_.ProcessId }\nStop-Process -Id $ppid "
        "}\nStop-Embedded " +
        std::to_string(pid);

    ShellExecute(NULL, NULL, shell.c_str(), ps_args.c_str(), NULL, SW_HIDE);
  } else {
    QMessageBox temp;
    temp.setIcon(QMessageBox::Warning);
    temp.setText("No process is currently running.");
    temp.exec();
  }
}

bool Executor::exe_drrun(QString cmd, QObject* parent) {
  if (cmd.endsWith("drrun") || cmd.endsWith("drrun.exe")) {
    QProcess* proc_ovpn = new QProcess(parent);
    proc_ovpn->start(cmd, QStringList() << "-version");
    proc_ovpn->setProcessChannelMode(QProcess::MergedChannels);

    if (proc_ovpn->waitForFinished()) {
      QString str(proc_ovpn->readAllStandardOutput());

      if (str.contains("drrun version 8.", Qt::CaseInsensitive)) {
        return true;
      }
    }
  }
  return false;
}

bool Executor::exe_python3(QObject* parent) {
  QProcess* proc_ovpn = new QProcess(parent);
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

bool Executor::exe_msr(QString cmd, QObject* parent) {
  QProcess* proc_ovpn = new QProcess(parent);
  proc_ovpn->start(cmd, QStringList() << "--version");
  proc_ovpn->setProcessChannelMode(QProcess::MergedChannels);

  if (proc_ovpn->waitForFinished()) {
    QString str(proc_ovpn->readAllStandardOutput());

    if (str.contains("Managed Symbol Resolver", Qt::CaseInsensitive)) {
      return true;
    }
  }

  return false;
}

void Executor::launch_msr(std::string cmd) {
  std::string exe_cmd = "start powershell -NoExit " + cmd + " --once";
  system(exe_cmd.c_str());
  // wait for msr to set up, because drace is started directly afterwards and
  // must connect to msr
  QThread::sleep(1);
}
