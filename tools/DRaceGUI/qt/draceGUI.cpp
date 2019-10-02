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

#include "draceGUI.h"
#include "./ui_draceGUI.h"


DRaceGUI::DRaceGUI(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::DRaceGUI)
{
    rh = new Report_Handler();
    ch = new Command_Handler;
    exe = new Executor();
    ui->setupUi(this);

    ui->dr_path_input->setText("drrun.exe");
    ui->command_output->setText(ch->get_command());
}

DRaceGUI::~DRaceGUI()
{
    delete rh;
    delete ch;
    delete exe;
    delete ui;
}

//Browse button functions
void DRaceGUI::on_dr_path_btn_clicked()
{
    QString path = QFileDialog::getOpenFileName(this, "Open File", dr_pth_cache, tr("Executable (*.exe)"));
    if (!path.isEmpty()) {
        dr_pth_cache = path;
        ui->dr_path_input->setText(path);
    }
}

void DRaceGUI::on_drace_path_btn_clicked()
{
    QString path = QFileDialog::getOpenFileName(this, "Open File", drace_pth_cache, tr("Dll (*.dll)"));
    if (!path.isEmpty()) {
        drace_pth_cache = path;
        ui->drace_path_input->setText(path);
    }
}

void DRaceGUI::on_exe_path_btn_clicked()
{
    QString selfilter = tr("Executable (*.exe)");
    QString path = QFileDialog::getOpenFileName(
        this, "Open File", exe_pth_cache,
        tr("All files (*.*);;Executable (*.exe)"), &selfilter);
    if (!path.isEmpty()) {
        exe_pth_cache = path;
        ui->exe_input->setText(path);
    }
}

void DRaceGUI::on_config_browse_btn_clicked()
{
    QString selfilter = tr("INI (*.ini)");
    QString path = QFileDialog::getOpenFileName(
        this, "Open File", config_pth_cache,
        tr("All files (*.*);;INI (*.ini)"), &selfilter);

    if (!path.isEmpty()) {
        config_pth_cache = path;
        ui->config_file_input->setText(path);
    }
}


//push button functions
void DRaceGUI::on_run_button_clicked()
{
    exe->execute(this, ch);
}

void DRaceGUI::on_copy_button_clicked()
{
    clipboard->setText(ch->get_command());
}


//radio buttons
void DRaceGUI::on_tsan_btn_clicked()
{
    ui->command_output->setText(ch->make_entry("tsan", Command_Handler::DETECTOR, "-d"));
}

void DRaceGUI::on_extsan_btn_clicked()
{
    ui->command_output->setText(ch->make_entry("dummy", Command_Handler::DETECTOR, "-d"));
}

void DRaceGUI::on_fasttrack_btn_clicked()
{
    ui->command_output->setText(ch->make_entry("fasttrack", Command_Handler::DETECTOR, "-d"));
}


//tick box
void DRaceGUI::on_dr_debug_stateChanged(int arg1)
{
    if(arg1 == 0){
        ui->command_output->setText(ch->updateCommand("", Command_Handler::DR_DEBUG));
    }
    else{
        ui->command_output->setText(ch->updateCommand("-debug", Command_Handler::DR_DEBUG));
    }
}


//textbox functions
void DRaceGUI::on_dr_path_input_textChanged(const QString &arg1)
{
    ui->command_output->setText(ch->make_entry(arg1, Command_Handler::DYNAMORIO));
    QPalette pal;
    pal.setColor(QPalette::Base, Qt::red);

    if (exe->exe_drrun(arg1, this)) { //if drrun is found set green, otherwise set red    
        pal.setColor(QPalette::Base, Qt::green);
    }

    ui->dr_path_input->setPalette(pal);
}

void DRaceGUI::on_drace_path_input_textChanged(const QString &arg1)
{
    ui->command_output->setText(ch->make_entry(arg1, Command_Handler::DRACE, "-c"));

    QPalette pal;
    if (arg1.endsWith("drace-client.dll")) {
        pal.setColor(QPalette::Base, Qt::green);
        if (ui->config_file_input->text().isEmpty()) {
            auto path_to_config = arg1;
            path_to_config.replace(QString("drace-client.dll"), QString("drace.ini"));
            ui->config_file_input->setText(path_to_config);
        }
    }
    else {
        pal.setColor(QPalette::Base, Qt::red);
    }
    ui->drace_path_input->setPalette(pal);

}

void DRaceGUI::on_exe_input_textChanged(const QString &arg1)
{
    ui->command_output->setText(ch->make_entry(arg1, Command_Handler::EXECUTABLE, "--"));
}

void DRaceGUI::on_flag_input_textChanged(const QString &arg1)
{
    ui->command_output->setText(ch->make_flag_entry(arg1));
}

void DRaceGUI::on_config_file_input_textChanged(const QString & arg1)
{
    ui->command_output->setText(ch->make_entry(arg1, Command_Handler::CONFIGURATION, "-c"));

    QPalette pal;
    pal.setColor(QPalette::Base, Qt::red);

    std::ifstream config(arg1.toStdString());

    if (config.good()) {
        pal.setColor(QPalette::Base, Qt::green);
    }

    ui->config_file_input->setPalette(pal);
}


//action button functions
void DRaceGUI::on_actionAbout_triggered()
{
    About_Dialog newWindow;
    newWindow.exec();
}

void DRaceGUI::on_actionLoad_Config_triggered()
{
    QString path = QFileDialog::getOpenFileName(this, "Open File", QDir::currentPath());

    if(ls_handler.load(path.toStdString(), this)){
        QMessageBox msg;
        msg.setText("Loading was successfull");
        msg.exec();
    }
    else{
        QMessageBox msg;
        msg.setIcon(QMessageBox::Warning);
        msg.setText("Loading was not successfull");
        msg.exec();
    }
}

void DRaceGUI::on_actionSave_Config_triggered()
{
      QString path = QFileDialog::getSaveFileName(this, "Save file", QDir::currentPath());

      if(ls_handler.save(path.toStdString(), this)){
          QMessageBox msg;
          msg.setText("Saving was successfull");
          msg.exec();
      }
      else{
          QMessageBox msg;
          msg.setIcon(QMessageBox::Warning);
          msg.setText("Saving was not successfull");
          msg.exec();
      }
}

void DRaceGUI::on_actionConfigure_Report_triggered()
{
    Report_Config* report_window;
    report_window = new Report_Config(rh);

    //QObject::connect(report_window, SIGNAL(SIG_converter_path(QString)), this, SLOT(dh->set_report_converter(QString)));
    //QObject::connect(report_window, SIGNAL(SIG_report_name(QString)), this, SLOT(dh->set_report_name(QString)));
    report_window->exec();

}



















