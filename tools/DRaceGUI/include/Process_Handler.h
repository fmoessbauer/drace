/*
 * DRace, a dynamic data race detector
 *
 * Copyright 2020 Siemens AG
 *
 * Authors:
 *   Mohamad Kanj <mohamad.kanj@siemens.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef PROCESS_HANDLER_H
#define PROCESS_HANDLER_H

#include <QObject>
#include <QProcess>
#include <bitset>
#include "Executor.h"

class Process_Handler {
 public:
  enum Misc_Options : size_t {
    RUN_SEPARATELY,
    WRAP_TEXT_OUTPUT,
    _END_OF_MISC_OPTIONS_
  };

 private:
  /// DRace process running in embedded window
  QProcess* drace;
  /// the options array holds misc. checkbox states
  std::bitset<Misc_Options::_END_OF_MISC_OPTIONS_> options;

 public:
  Process_Handler(QObject* parent);
  ~Process_Handler();

  /// kill process
  void stop();
  /// communicate data to process via standard input
  void write(const QString& arg);

  /// get pointer to the process
  inline QProcess* get_process() const { return drace; }
  /// get options bitset
  inline const std::bitset<Misc_Options::_END_OF_MISC_OPTIONS_>&
  get_options_array() const {
    return options;
  }
  /// checks whether process is running
  inline bool is_running() const { return drace->state() == QProcess::Running; }
  /// checks whether process is starting or running
  inline bool is_starting_or_running() const {
    return is_running() || drace->state() == QProcess::Starting;
  }
  /// get state of run separately checkbox
  inline bool get_run_separately() const { return options[RUN_SEPARATELY]; }

  void set_options_array(
      const std::bitset<Misc_Options::_END_OF_MISC_OPTIONS_>& arg);
  void set_run_separately(bool arg);
  void set_wrap_text_output(bool arg);
};

#endif  // PROCESS_HANDLER_H