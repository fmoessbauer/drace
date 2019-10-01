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

Report_Config::Report_Config(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Report_Config)
{
    ui->setupUi(this);
}

Report_Config::~Report_Config()
{
    delete ui;
}
