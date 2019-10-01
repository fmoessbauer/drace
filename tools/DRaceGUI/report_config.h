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

namespace Ui {
class Report_Config;
}

class Report_Config : public QDialog
{
    Q_OBJECT

public:
    explicit Report_Config(QWidget *parent = nullptr);
    ~Report_Config();

private:
    Ui::Report_Config *ui;
};

#endif // REPORT_CONFIG_H
