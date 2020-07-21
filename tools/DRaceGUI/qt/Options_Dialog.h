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

#ifndef OPTIONS_DIALOG_H
#define OPTIONS_DIALOG_H

#include <QDialog>
#include <QDir>
#include <QMessageBox>
#include <QSettings>

namespace Ui {
class Options_Dialog;
}

class Options_Dialog : public QDialog {
  Q_OBJECT

 public:
  /// displays the options dialog when called
  explicit Options_Dialog(QWidget *parent = nullptr);
  ~Options_Dialog();
  /// default file extension for configuration file
  static const QString CONFIG_EXT;

 private slots:
  void on_buttonBox_clicked();
  void on_set_registry_clicked();
  void on_revert_registry_clicked();

 private:
  Ui::Options_Dialog *ui;
};

#endif  // OPTIONS_DIALOG_H
