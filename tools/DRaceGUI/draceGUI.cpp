#include "draceGUI.h"
#include "./ui_draceGUI.h"

//#include <windows.h>

DRaceGUI::DRaceGUI(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::DRaceGUI)
{
    ui->setupUi(this);
}

DRaceGUI::~DRaceGUI()
{
    delete ui;
}


void DRaceGUI::on_dr_path_btn_clicked()
{
    QString path = QFileDialog::getOpenFileName(this, "Open File", "C:\\");
    ui->dr_path_input->setText(path);
    updateCommand(path, 0);

}

void DRaceGUI::on_drace_path_btn_clicked()
{
     QString path = QFileDialog::getOpenFileName(this, "Open File", "C:\\");
     ui->drace_path_input->setText(path);
     path = "-c " + path;
     updateCommand(path, 1);
}

void DRaceGUI::on_exe_path_btn_clicked()
{
     QString path = QFileDialog::getOpenFileName(this, "Open File", "C:\\");
     ui->exe_input->setText(path);
     updateCommand(path, 4);
}

void DRaceGUI::on_config_browse_btn_clicked()
{
     QString path = QFileDialog::getOpenFileName(this, "Open File", "C:\\");
     ui->config_file_input->setText(path);
     path = "-c " + path;
     updateCommand(path, 3);
}

void DRaceGUI::on_run_button_clicked()
{

}


void DRaceGUI::on_dr_path_input_textEdited(const QString &arg1)
{
    DRaceGUI::updateCommand(arg1, 0);

}

void DRaceGUI::updateCommand(const QString &arg1, uint position){
    command[position] = arg1;
    //ui->command_output->clear();
    entire_command = "";
    for(int it=0; it < 4; it++){
        entire_command.append(command[it]);
        entire_command.append(" ");
    }
    ui->command_output->setText(entire_command);
}

void DRaceGUI::on_copy_button_clicked()
{
    clipboard->setText(entire_command);
}


void DRaceGUI::on_drace_path_input_textEdited(const QString &arg1)
{
    DRaceGUI::updateCommand(arg1, 1);
}

void DRaceGUI::on_exe_input_textEdited(const QString &arg1)
{
    DRaceGUI::updateCommand(arg1, 3);
}

void DRaceGUI::on_tsan_btn_clicked()
{
    QString detector = "-d tsan";
     DRaceGUI::updateCommand(detector, 2);
}
