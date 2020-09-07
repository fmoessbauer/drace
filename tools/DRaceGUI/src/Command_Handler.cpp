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

#include "Command_Handler.h"

Command_Handler::Command_Handler(QString default_detector) {
  make_entry(default_detector, DETECTOR, "-d");
}

QString Command_Handler::make_entry(const QString &path, uint position,
                                    QString prefix, bool no_quotes) {
  if (path != "") {
    QString temp = path;
    if (temp.contains(QRegExp("\\s+")) && !no_quotes) {
      temp = "\'" + temp + "\'";
    }
    if (prefix != "") {
      temp = prefix + " " + temp;
    }
    return updateCommand(temp, position);
  } else {
    return updateCommand("", position);
  }
}

QString Command_Handler::updateCommand(const QString &arg1, uint position) {
  if ((position == REPORT_CMD || position == REPORT_OPEN_CMD) && arg1 != "") {
    // set a semicolon before the report command -> must be executed after the
    // drace finished
    QString cmd = "; " + arg1;
    command[position] = cmd;
  } else {
    command[position] = arg1;
  }

  return make_command();
}

QString Command_Handler::make_command() {
  entire_command = "";
  // if drrun is enclosed in quotation marks, prefix with a '.'
  if (command[0] != "") {
    if (command[0].contains(QRegExp("\\s+"))) {
      entire_command.prepend(".");
    }
  }
  for (int it = 0; it < (_END_OF_COMMAND_TYPE_); it++) {
    if (command[it] != "") {
      entire_command.append(command[it]);
      entire_command.append(" ");
    }
  }
  return entire_command;
}

QString Command_Handler::make_flag_entry(const QString &arg1) {
  if (arg1 != "") {
    return updateCommand(arg1, FLAGS);
  } else {
    return updateCommand("", FLAGS);
  }
}

QString Command_Handler::make_exe_args_entry(const QString &arg1) {
  if (arg1 != "") {
    return updateCommand(arg1, EXECUTABLE_ARGS);
  } else {
    return updateCommand("", EXECUTABLE_ARGS);
  }
}

QString Command_Handler::make_report_auto_open_entry(const int &arg1) {
  if (arg1 != 0) {
    const QString cmd =
        "$report ; If ($report -contains \'Report creation successful\') "
        "{$folder = $report -match \'drace_report_*\' ; Invoke-Item "
        "$folder/index.html}";
    return updateCommand(cmd, REPORT_OPEN_CMD);
  } else {
    return updateCommand("", REPORT_OPEN_CMD);
  }
}

bool Command_Handler::validate_and_set_msr(bool on, QObject *parent) {
  QString cmd = "";
  make_extctrl(false);

  if (on) {
    cmd = eval_msr_path(parent);
    if (cmd != "") {
      make_extctrl(true);
    } else {
      msr_path = cmd;
      return false;
    }
  }
  msr_path = cmd;
  return true;
}

QString Command_Handler::eval_msr_path(QObject *parent) {
  QString str_dir = command[DRACE], cmd = "";  // search for msr in drace path

  if (str_dir != "") {
    str_dir.remove(0, 3);  // remove leading "-c " of drace client command
    QDir dir(str_dir);
    dir.cd("..");
    cmd = dir.absolutePath() + "/msr.exe";
    if (!Executor::exe_msr(cmd, parent)) {
      cmd = "";
    }
  }
  return cmd;
}

void Command_Handler::make_extctrl(bool set) {
  if (set) {  // set case
    command[EXT_CTRL] = "--extctrl";
  } else {  // reset case
    command[EXT_CTRL] = "";
  }
  make_command();
}

bool Command_Handler::command_is_valid() {
  if (command[DYNAMORIO] == "") {
    return false;
  }
  if (command[DRACE] == "") {
    return false;
  }
  if (command[DETECTOR] == "") {
    return false;
  }
  if (command[EXECUTABLE] == "") {
    return false;
  }
  return true;
}
