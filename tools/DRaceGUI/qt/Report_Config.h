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

#ifndef REPORT_CONFIG_H
#define REPORT_CONFIG_H

#include <QDialog>
#include <QFileDialog>
#include <QListWidget>
#include <QMessagebox>
#include "Report_Handler.h"
#include "boost/filesystem.hpp"

namespace Ui {
class Report_Config;
}

class Report_Config : public QDialog {
  Q_OBJECT

 public:
  /// ui box to set the path of the report converter
  explicit Report_Config(Report_Handler *rh, QWidget *parent = nullptr);
  ~Report_Config();

 private slots:
  void on_buttonBox_rejected();
  void on_buttonBox_accepted();
  void on_report_conv_path_clicked();
  void on_report_srcdirs_add_clicked();
  void on_report_srcdirs_discard_clicked();
  void on_report_name_textChanged(const QString &arg1);
  void on_report_conv_input_textChanged(const QString &arg1);

 private:
  Ui::Report_Config *ui;
  Report_Handler *r_handler;
  QString path_cache;
};

#endif  // REPORT_CONFIG_H
