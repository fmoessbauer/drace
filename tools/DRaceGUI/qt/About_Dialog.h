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

#ifndef ABOUT_DIALOG_H
#define ABOUT_DIALOG_H

#include <QDialog>

namespace Ui {
class About_Dialog;
}

class About_Dialog : public QDialog {
  Q_OBJECT

 public:
  /// displays the about dialog when called
  explicit About_Dialog(QWidget *parent = nullptr);
  ~About_Dialog();

 private slots:

 private:
  Ui::About_Dialog *ui;
};

#endif  // ABOUT_DIALOG_H
