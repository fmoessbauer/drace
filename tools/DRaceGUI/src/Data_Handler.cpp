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

#include "Data_Handler.h"

void Data_Handler::get_data(Report_Handler* rh, Command_Handler* ch) {
  report_name = rh->get_rep_name().toStdString();
  report_converter = rh->get_rep_conv_cmd().toStdString();
  report_srcdirs = rh->get_rep_srcdirs().toStdString();
  report_is_python = rh->get_is_python();
  create_report = rh->get_create_report();

  auto data = ch->get_command_array();
  dynamorio = data[Command_Handler::DYNAMORIO].toStdString();
  dr_debug = data[Command_Handler::DR_DEBUG].toStdString();
  drace = data[Command_Handler::DRACE].toStdString();
  detector = data[Command_Handler::DETECTOR].toStdString();
  flags = data[Command_Handler::FLAGS].toStdString();
  ext_ctrl = data[Command_Handler::EXT_CTRL].toStdString();
  configuration = data[Command_Handler::CONFIGURATION].toStdString();
  executable = data[Command_Handler::EXECUTABLE].toStdString();
  executable_args = data[Command_Handler::EXECUTABLE_ARGS].toStdString();

  msr = ch->get_msr_path().toStdString();
}

void Data_Handler::set_data(Report_Handler* rh, Command_Handler* ch) {
  // reset report handler members
  rh->set_report_name(QString(report_name.c_str()));
  rh->set_report_converter(QString(report_converter.c_str()));
  rh->set_report_srcdirs(QString(report_srcdirs.c_str()));
  rh->set_create_state(create_report);
  rh->set_is_python(report_is_python);

  // report handler sets now the new report command in the commandhandler
  rh->set_report_command();

  // reset command handler members
  ch->command[Command_Handler::DYNAMORIO] = QString(dynamorio.c_str());
  ch->command[Command_Handler::DR_DEBUG] = QString(dr_debug.c_str());
  ch->command[Command_Handler::DRACE] = QString(drace.c_str());
  ch->command[Command_Handler::DETECTOR] = QString(detector.c_str());
  ch->command[Command_Handler::FLAGS] = QString(flags.c_str());
  ch->command[Command_Handler::EXT_CTRL] = QString(ext_ctrl.c_str());
  ch->command[Command_Handler::CONFIGURATION] = QString(configuration.c_str());
  ch->command[Command_Handler::EXECUTABLE] = QString(executable.c_str());
  ch->command[Command_Handler::EXECUTABLE_ARGS] =
      QString(executable_args.c_str());

  ch->set_msr(QString(msr.c_str()));
}
