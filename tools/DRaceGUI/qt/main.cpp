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

#include "draceGUI.h"

#include <QApplication>

int main(int argc, char *argv[]) {
  QApplication a(argc, argv);
  DRaceGUI w;
  w.show();
  return a.exec();
}
