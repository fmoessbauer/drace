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

#include "About_Dialog.h"
#include "ui_About_Dialog.h"

About_Dialog::About_Dialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::About_Dialog) {
  ui->setupUi(this);
}

About_Dialog::~About_Dialog() { delete ui; }
