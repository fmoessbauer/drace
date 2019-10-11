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
    ui->setupUi(this);

    ui->dr_path_input->setText("drrun.exe");
    ui->command_output->setText(ch->get_command());
}

DRaceGUI::~DRaceGUI()
{
    delete rh, ch, ui;
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
        QString msr = ch->get_msr_path();
        if (msr != "") {
            Executor::launch_msr(msr.toStdString()); //returns when msr is (should be :D) set up
        }
        Executor::execute((ch->get_command()).toStdString(), this);
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

void DRaceGUI::on_dummy_btn_clicked()
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
        ui->command_output->setText(ch->make_entry("", Command_Handler::DR_DEBUG));
    }
    else{
        ui->command_output->setText(ch->make_entry("-debug", Command_Handler::DR_DEBUG));
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
    temp.setText("ReportConverter is not set correctly");
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
    pal.setColor(QPalette::Base, Qt::red);

    if (arg1.endsWith("drace-client.dll")) {
        pal.setColor(QPalette::Base, Qt::green);
        
        if (ui->config_file_input->text().isEmpty() || ui->config_file_input->palette().color(QPalette::Base) == QColor(Qt::red)){//try to set config file, when empty or red (=wrong)
            auto path_to_config = arg1;
            path_to_config.replace(QString("drace-client.dll"), QString("drace.ini"));
            ui->config_file_input->setText(path_to_config);
        }
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
    QString path = QFileDialog::getOpenFileName(this, "Open File", load_save_path);
    if (path != "") {
        load_save_path = path;
        Load_Save ls(rh, ch);
        if (ls.load(path.toStdString())) {
            set_boxes_after_load();//restore all the data to the boxes
        }
        else {
            QMessageBox temp;
            temp.setIcon(QMessageBox::Warning);
            temp.setText("File is not valid!");
            temp.exec();
        }
    }
}

void DRaceGUI::on_actionSave_Config_triggered()
{
    QString path = QFileDialog::getSaveFileName(this, "Save file", load_save_path);
    if (path != "") {
        load_save_path = path;
        Load_Save ls(rh, ch);
        ls.save(path.toStdString());
    }
}

void DRaceGUI::on_actionConfigure_Report_triggered()
{
    Report_Config* report_window;
    report_window = new Report_Config(rh);
    report_window->exec();
    ui->command_output->setText(ch->get_command());
}

void DRaceGUI::on_actionHelp_triggered() {
    QMessageBox temp;
    temp.setIcon(QMessageBox::Warning);
    temp.setText("This feature is not avaliable!");
    temp.exec();
}


///after a load all necessary data is restored in the handler classes
///this functions puts the data bacak in the text and check boxes
void DRaceGUI::set_boxes_after_load()
{
    //reset text boxes
    ui->dr_path_input->setText(ch->command[Command_Handler::DYNAMORIO]);

    QString drace_cl = ch->command[Command_Handler::DRACE];
    drace_cl.remove(0, 3); //drace cmd is -c DRACECLIENT -> remove "-c "
    ui->drace_path_input->setText(drace_cl);

    QString config = ch->command[Command_Handler::CONFIGURATION];
    config.remove(0, 3); //config cmd is -c CONFIGFILE -> remove "-c "
    ui->config_file_input->setText(config);

    QString exe = ch->command[Command_Handler::EXECUTABLE];
    exe.remove(0, 3); //exe cmd is -- EXECUTEABLE -> remove "-- "
    ui->exe_input->setText(exe);
    
    ui->flag_input->setText(ch->command[Command_Handler::FLAGS]);


    //set debug check box
    if (ch->command[Command_Handler::DR_DEBUG] != ""){
        ui->dr_debug->setChecked(true);
        ui->dr_debug->setCheckState(Qt::Checked);
    }
    else {
        ui->dr_debug->setChecked(false);
        ui->dr_debug->setCheckState(Qt::Unchecked);
    }


    //set report check box
    if(rh->command_valid()){
        ui->report_creation->setChecked(true);
        ui->report_creation->setCheckState(Qt::Checked);
    }
    else {
        ui->report_creation->setChecked(false);
        ui->report_creation->setCheckState(Qt::Unchecked);
    }

    //set msr check box
    if (ch->get_msr_path() != "") {
        ui->msr_box->setChecked(true);
        ui->msr_box->setCheckState(Qt::Checked);
    }
    else {
        ui->msr_box->setChecked(false);
        ui->msr_box->setCheckState(Qt::Unchecked);
    }

    //set detector back end
    QString detector = ch->command[Command_Handler::DETECTOR];
    if (detector.contains("tsan", Qt::CaseInsensitive)) {
        ui->tsan_btn->setChecked(true);
    }
    if (detector.contains("dummy", Qt::CaseInsensitive)) {
        ui->dummy_btn->setChecked(true);
    }
    if (detector.contains("fasttrack", Qt::CaseInsensitive)) {
        ui->fasttrack_btn->setChecked(true);
    }


    ui->command_output->setText(ch->make_command());
}


