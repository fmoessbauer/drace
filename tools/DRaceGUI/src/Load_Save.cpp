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

#include "Load_Save.h"

Load_Save::Load_Save(Report_Handler* r, Command_Handler* c) : rh(r), ch(c) {}

void Load_Save::save(std::string path) {
  Data_Handler data;
  // the data handler fetches all data from the other handler classes
  data.get_data(rh, ch);

  std::ofstream save_to_file(path);
  {
    boost::archive::text_oarchive oa(save_to_file);
    oa << data;  /// just save all the members as serialized stream to the file
  }
}

bool Load_Save::load(std::string path) {
  Data_Handler data;
  std::ifstream load_file(path);

  try {
    boost::archive::text_iarchive ia(load_file);
    ia >> data;
  } catch (boost::archive::archive_exception) {
    return false;
  }

  // the data handler restores all the data from the file to the other handler
  // classes
  data.set_data(rh, ch);
  return true;
}
