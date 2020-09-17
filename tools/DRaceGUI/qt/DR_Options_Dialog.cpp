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

#include "DR_Options_Dialog.h"
#include "ui_DR_Options_Dialog.h"

DR_Options_Dialog::DR_Options_Dialog(Command_Handler *ch, QWidget *parent)
    : QDialog(parent), ui(new Ui::DR_Options_Dialog), c_handler(ch) {
  ui->setupUi(this);
  ui->dr_args_input->setText(c_handler->command[Command_Handler::DR_ARGS]);
}

DR_Options_Dialog::~DR_Options_Dialog() { delete ui; }

void DR_Options_Dialog::on_buttonBox_rejected() { this->close(); }

void DR_Options_Dialog::on_buttonBox_accepted() {
  c_handler->make_text_entry(ui->dr_args_input->text(),
                             Command_Handler::DR_ARGS);
  this->close();
}