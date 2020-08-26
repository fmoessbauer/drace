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

#include <nlohmann/json.hpp>

using json = nlohmann::json;

class Data_Handler {
 private:
  json config = {
      {"report_name", ""},
      {"report_converter", ""},
      {"report_srcdirs", ""},
      {"dynamorio", ""},
      {"dr_debug", ""},
      {"drace", ""},
      {"detector", ""},
      {"flags", ""},
      {"ext_ctrl", ""},
      {"configuration", ""},
      {"executable", ""},
      {"executable_args", ""},
      {"msr", ""},
      {"report_is_python", false},
      {"create_report", false},
  };

 public:
  /// loads data from handler classes to members
  void get_data(Report_Handler* rh, Command_Handler* ch);

  /// restores data from members back in the handler classes
  void set_data(Report_Handler* rh, Command_Handler* ch);

  /// getter for config json object
  const json& get_config() const { return config; }

  /// setter for config json object
  void set_config(json& j) { config = j; }
};
#endif  // !DATA_HANDLER_H
