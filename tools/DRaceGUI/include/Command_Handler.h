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

#include <QDir>
#include <QList>
#include <QRegExp>
#include <QString>
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
  static constexpr uint EXECUTABLE_ARGS = 9;
  static constexpr uint REPORT_CMD = 10;

 private:
  QString entire_command;

  QString msr_path;

  /// evaluates and returns the msr command when valid
  QString eval_msr_path(QObject* parent);

  /// sets the external control flag (needed for msr execution)
  void make_extctrl(bool set);

  /// sets the QString at the position in command array and returns the updated
  /// command
  QString updateCommand(const QString& arg1, uint position);

 public:
  Command_Handler(QString default_detector = "tsan");

  /// the command array holds each part of the command as a QString
  QString command[(REPORT_CMD + 1)];

  /// sets the flag entry
  QString make_flag_entry(const QString& arg1);

  /// sets the argument(s) (if any) of the executable under test
  QString make_exe_args_entry(const QString& arg1);

  /// makes a command entry into the command array and updates the
  /// entire_command
  QString make_entry(const QString& path, uint postion, QString prefix = "",
                     bool no_quotes = false);

  /// updates the entire command  and returns it
  QString make_command();

  QString get_command() { return entire_command; }
  QString get_msr_path() { return msr_path; }
  QString* get_command_array() { return command; }

  void set_msr(QString path) { msr_path = path; }

  /// returns true when command is not found invalid
  bool command_is_valid();

  /// searches in the drace path for the msr
  /// if found returns true and sets msr path
  bool validate_and_set_msr(bool on, QObject* parent);
};

#endif  // !COMMAND_HANDLER_H
