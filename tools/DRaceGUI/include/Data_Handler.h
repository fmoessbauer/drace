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

#ifndef DATA_HANDLER_H
#define DATA_HANDLER_H

#include <QString>
#include <string>
#include "Command_Handler.h"
#include "Report_Handler.h"

class Data_Handler {
  std::string report_name, report_converter, report_srcdirs, dynamorio,
      dr_debug, drace, detector, flags, excl_stack, ext_ctrl, configuration,
      executable, executable_args, report_auto_open, msr;

  bool report_is_python, create_report;

  friend class boost::serialization::access;

  /// function for the serialization of the class members
  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar& report_name;
    ar& report_converter;
    ar& report_srcdirs;
    ar& dynamorio;
    ar& dr_debug;
    ar& drace;
    ar& detector;
    ar& flags;
    ar& excl_stack;
    ar& ext_ctrl;
    ar& configuration;
    ar& executable;
    ar& executable_args;
    ar& msr;
    ar& report_is_python;
    ar& create_report;
    ar& report_auto_open;
  }

 public:
  /// loads data from handler classes to members
  void get_data(Report_Handler* rh, Command_Handler* ch);

  /// restores data from members back in the handler classes
  void set_data(Report_Handler* rh, Command_Handler* ch);
};
#endif  // !DATA_HANDLER_H
