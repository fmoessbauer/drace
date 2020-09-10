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
#include <QMessageBox>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QThread>
#include <string>

class Executor {
 public:
  /// executes the cmd in a powershell, which  will stay open after finishing
  static void execute(std::string cmd, QObject* parent);

  /// executes the cmd in the embedded window
  static void execute_embedded(QProcess* proc, QString cmd, QObject* parent);

  /// kills the embedded process along with its children
  static void stop_embedded(qint64 pid);

  /// checks if cmd is a valid dynamorio cmd
  static bool exe_drrun(QString cmd, QObject* parent);

  /// checks if python3 is installed
  static bool exe_python3(QObject* parent);

  /// checks if cmd is a valid ManagedSymbol Resolver cmd
  static bool exe_msr(QString cmd, QObject* parent);

  /// launches a powershell with the MSR which will stay open after finishing
  /// the command
  static void launch_msr(std::string cmd);
};

#endif  // !EXECUTOR_H
