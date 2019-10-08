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
    ch = new Command_Handler();
    rh = new Report_Handler(ch, this);
    ls = new Load_Save();
    ui->setupUi(this);

    ui->dr_path_input->setText("drrun.exe");
    ui->command_output->setText(ch->get_command());
}

DRaceGUI::~DRaceGUI()
{
    delete rh;
    delete ch;
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
    if (ch->command_is_valid()) {
        Executor::execute(this, (ch->get_command()).toStdString());
        return;
    }
    QMessageBox temp;
    temp.setIcon(QMessageBox::Warning);
    temp.setText("The command is not valid!");
    temp.exec();
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

void DRaceGUI::on_report_creation_stateChanged(int arg1)
{
    rh->set_create_state(arg1);
    if (rh->set_report_command() || !arg1) {
        ui->command_output->setText(ch->get_command());
        return;
    }
    QMessageBox temp;
    temp.setIcon(QMessageBox::Warning);
    temp.setText("ReportConverter is path not set correctly");
    temp.exec();
    ui->msr_box->setChecked(false);
    ui->msr_box->setCheckState(Qt::Unchecked);
}

void DRaceGUI::on_msr_box_stateChanged(int arg1)
{
    if (!(ch->validate_and_set_msr(arg1, this)) && arg1) {
        QMessageBox temp;
        temp.setIcon(QMessageBox::Warning);
        temp.setText("DRace client path is not set or \"msr.exe\" is not at DRace path.");
        temp.exec();
        ui->msr_box->setChecked(false);
        ui->msr_box->setCheckState(Qt::Unchecked);
    }
    ui->command_output->setText(ch->get_command());
}



//textbox functions
void DRaceGUI::on_dr_path_input_textChanged(const QString &arg1)
{
    ui->command_output->setText(ch->make_entry(arg1, Command_Handler::DYNAMORIO));
    QPalette pal;
    pal.setColor(QPalette::Base, Qt::red);

    if (Executor::exe_drrun(arg1, this)) { //if drrun is found set green, otherwise set red    
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
        
        if (ui->config_file_input->text().isEmpty() || ui->config_file_input->palette().color(QPalette::Base) == QColor(Qt::red)){//try to set config file, when empty or red (=wrong)
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
    QMessageBox temp;
    temp.setText("Feature not implemented");
    temp.exec();
    return;

    QString path = QFileDialog::getOpenFileName(this, "Open File", QDir::currentPath());

    QMessageBox msg;
    if(ls->load(path.toStdString(), this)){
        msg.setText("Loading was successfull");
    }
    else{
        msg.setIcon(QMessageBox::Warning);
        msg.setText("Loading was not successfull");
    }
    msg.exec();
}

void DRaceGUI::on_actionSave_Config_triggered()
{
    QMessageBox temp;
    temp.setText("Feature not implemented");
    temp.exec();
    return;

    QString path = QFileDialog::getSaveFileName(this, "Save file", QDir::currentPath());

    QMessageBox msg;
    if(ls->save(path.toStdString(), this)){
        msg.setText("Saving was successfull");
        msg.exec();
    }
    else{
        msg.setIcon(QMessageBox::Warning);
        msg.setText("Saving was not successfull");
    }
    msg.exec();
}

void DRaceGUI::on_actionConfigure_Report_triggered()
{
    Report_Config* report_window;
    report_window = new Report_Config(rh);
    report_window->exec();
    ui->command_output->setText(ch->get_command());
}


