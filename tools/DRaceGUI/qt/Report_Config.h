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
#include "Report_Handler.h"
#include "boost/filesystem.hpp"
#include <QMessagebox>

namespace Ui {
class Report_Config;
}

class Report_Config : public QDialog
{
    Q_OBJECT

public:
    explicit Report_Config(Report_Handler * rh,  QWidget *parent = nullptr);


    ~Report_Config();

private slots:
    void on_buttonBox_rejected();

    void on_buttonBox_accepted();

    void on_report_conv_path_clicked();

    void on_report_name_textChanged(const QString &arg1);


    void on_report_conv_input_textChanged(const QString &arg1);

public slots:

//signals:
   //void SIG_report_name(QString path);
   //void SIG_converter_path(QString path);


private:
    Ui::Report_Config *ui;
    Report_Handler* r_handler;
};

#endif // REPORT_CONFIG_H
