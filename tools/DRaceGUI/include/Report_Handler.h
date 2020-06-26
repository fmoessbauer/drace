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

#include <QProcess>
#include <QString>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <string>
#include "Command_Handler.h"
#include "Executor.h"
#include "QDirIterator"

class Report_Handler {
  /// Name of report converter
  static constexpr char* REP_CONV_DEFAULT = "ReportConverter";

  /// member which holds the path to the report converter
  QString rep_conv_cmd = "";

  /// default report name
  QString rep_name = "test_report.xml";

  /// general source directories to the application
  QString rep_srcdirs = "";

  /// holds the state of the report check box
  bool create_report = false;

  /// true when Python script of report converter is set
  bool is_python;

  /// pointer to the command handler
  Command_Handler* ch;

  void find_report_converter(QObject* parent);

  QString make_command();

 public:
  Report_Handler(Command_Handler* c_handler, QObject* parent);

  QString get_rep_conv_cmd() { return rep_conv_cmd; }
  QString get_rep_name() { return rep_name; }
  QString get_rep_srcdirs() { return rep_srcdirs; }
  bool get_is_python() { return is_python; }
  bool get_create_report() { return create_report; }

  void set_report_name(QString name);
  /// sets the report converter without validity check
  void set_report_converter(QString path);
  void set_report_srcdirs(QString dirs);
  void set_is_python(bool state);
  bool set_report_command();

  /// evaluates if the report converter is valid, and sets it if so
  bool eval_rep_conv(QString name, QObject* parent);

  bool command_valid();

  void set_create_state(bool state);
};

#endif
