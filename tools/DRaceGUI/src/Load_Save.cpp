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

Load_Save::Load_Save(Report_Handler* r, Command_Handler* c, Process_Handler* p)
    : rh(r), ch(c), ph(p) {}

void Load_Save::save(std::string path) {
  Data_Handler data;
  // the data handler fetches all data from the other handler classes
  data.get_data(rh, ch, ph);

  std::ofstream save_to_file(path, std::ios::out | std::ios::binary);
  std::vector<uint8_t> output_vector;
  json::to_msgpack(data.get_config(), output_vector);
  save_to_file.write(reinterpret_cast<const char*>(output_vector.data()),
                     output_vector.size() * sizeof(uint8_t));
  save_to_file.close();
}

bool Load_Save::load(std::string path) {
  Data_Handler data;
  std::ifstream load_file(path, std::ios::binary);

  try {
    std::vector<uint8_t> input_vector(
        (std::istreambuf_iterator<char>(load_file)),
        std::istreambuf_iterator<char>());
    data.set_config(json::from_msgpack(input_vector));
    load_file.close();
  } catch (...) {
    return false;
  }

  // the data handler restores all the data from the file to the other handler
  // classes
  data.set_data(rh, ch, ph);
  return true;
}
