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
    ui->report_conv_input->setText(r_handler->get_rep_conv_cmd());
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

    //check if filename is valid
    if (boost::filesystem::windows_name(name.toStdString()) && boost::filesystem::is_regular_file(path.toStdString()) && r_handler->eval_rep_conv(path, this)){
        r_handler->set_report_name(name);
        r_handler->set_report_command();
    }
    else {
        QMessageBox msg;
        msg.setIcon(QMessageBox::Warning);
        msg.setText("Filename or ReportConverter is not valid!");
        msg.exec();
        return;
    }

    this->close();
}


void Report_Config::on_report_conv_path_clicked()
{
    QString path = QFileDialog::getOpenFileName(this, "Open File", QDir::currentPath());
    ui->report_conv_input->setText(path);
}

void Report_Config::on_report_name_textChanged(const QString &arg1)
{
    QPalette pal;
    pal.setColor(QPalette::Base, Qt::red);

    if (boost::filesystem::windows_name(arg1.toStdString())) { //if drrun is found set green, otherwise set red    
        pal.setColor(QPalette::Base, Qt::green);
    }

    ui->report_name->setPalette(pal);
}

void Report_Config::on_report_conv_input_textChanged(const QString &arg1)
{
    QPalette pal;
    pal.setColor(QPalette::Base, Qt::red);

    if (boost::filesystem::is_regular_file(arg1.toStdString()) && r_handler->eval_rep_conv(arg1, this)) { //if drrun is found set green, otherwise set red    
        pal.setColor(QPalette::Base, Qt::green);
    }

    ui->report_conv_input->setPalette(pal);
}
