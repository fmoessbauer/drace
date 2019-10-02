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


#include "Report_Config.h"
#include "ui_Report_Config.h"

Report_Config::Report_Config(Report_Handler* rh, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Report_Config),
    r_handler(rh)
{
    ui->setupUi(this);
    ui->report_conv_input->setText(r_handler->get_rep_conv_path());
    ui->report_name->setText(r_handler->get_rep_name());
}

Report_Config::~Report_Config()
{
    delete ui;
}

void Report_Config::on_buttonBox_rejected()
{
    this->close();
}

void Report_Config::on_buttonBox_accepted()
{
    QString path = ui->report_conv_input->text();
    QString name = ui->report_name->text();
    r_handler->set_report_converter(path);
    r_handler->set_report_name(name);
    this->close();
}


void Report_Config::on_report_conv_path_clicked()
{
    QString path = QFileDialog::getOpenFileName(this, "Open File", QDir::currentPath());
    ui->report_conv_input->setText(path);
}
