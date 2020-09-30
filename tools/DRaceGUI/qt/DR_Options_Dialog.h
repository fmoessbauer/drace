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

#ifndef DR_OPTIONS_DIALOG_H
#define DR_OPTIONS_DIALOG_H

#include <QDialog>
#include "Command_Handler.h"

namespace Ui {
class DR_Options_Dialog;
}

class DR_Options_Dialog : public QDialog {
  Q_OBJECT

 public:
  /// displays the about dialog when called
  explicit DR_Options_Dialog(Command_Handler *ch, QWidget *parent = nullptr);
  ~DR_Options_Dialog();

 private slots:
  void on_buttonBox_rejected();
  void on_buttonBox_accepted();

 private:
  Ui::DR_Options_Dialog *ui;
  Command_Handler *c_handler;
};

#endif  // DR_OPTIONS_DIALOG_H