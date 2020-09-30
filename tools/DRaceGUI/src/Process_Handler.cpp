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

#include "Process_Handler.h"

Process_Handler::Process_Handler(QObject* parent) {
  drace = new QProcess(parent);
  options[WRAP_TEXT_OUTPUT] = true;
}

Process_Handler::~Process_Handler() { delete drace; }

void Process_Handler::stop() {
  drace->closeReadChannel(QProcess::StandardOutput);
  drace->closeWriteChannel();
  Executor::stop_embedded(drace->processId());
}

void Process_Handler::write(const QString& arg) {
  if (is_running()) {
    drace->write((arg + "\n").toUtf8());
  }
}

void Process_Handler::set_options_array(
    const std::bitset<Misc_Options::_END_OF_MISC_OPTIONS_>& arg) {
  options = arg;
}

void Process_Handler::set_run_separately(bool arg) {
  options[RUN_SEPARATELY] = arg;
}

void Process_Handler::set_wrap_text_output(bool arg) {
  options[WRAP_TEXT_OUTPUT] = arg;
}