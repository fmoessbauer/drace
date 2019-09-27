#include "mainwindow.h"
#include "./ui_mainwindow.h"

//#include <windows.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_dr_path_btn_clicked()
{
    QString path = QFileDialog::getOpenFileName(this, "Open File", "C:\\");
    ui->dr_path_input->setText(path);
    updateCommand(path, 0);

}

void MainWindow::on_drace_path_btn_clicked()
{
     QString path = QFileDialog::getOpenFileName(this, "Open File", "C:\\");
     ui->drace_path_input->setText(path);
     path = "-c " + path;
     updateCommand(path, 1);
}

void MainWindow::on_exe_path_btn_clicked()
{
     QString path = QFileDialog::getOpenFileName(this, "Open File", "C:\\");
     ui->exe_input->setText(path);
     updateCommand(path, 4);
}

void MainWindow::on_config_browse_btn_clicked()
{
     QString path = QFileDialog::getOpenFileName(this, "Open File", "C:\\");
     ui->config_file_input->setText(path);
     path = "-c " + path;
     updateCommand(path, 3);
}

void MainWindow::on_run_button_clicked()
{

}


void MainWindow::on_dr_path_input_textEdited(const QString &arg1)
{
    MainWindow::updateCommand(arg1, 0);

}

void MainWindow::updateCommand(const QString &arg1, uint position){
    command[position] = arg1;
    //ui->command_output->clear();
    entire_command = "";
    for(int it=0; it < 4; it++){
        entire_command.append(command[it]);
        entire_command.append(" ");
    }
    ui->command_output->setText(entire_command);
}

void MainWindow::on_copy_button_clicked()
{
    clipboard->setText(entire_command);
}


void MainWindow::on_drace_path_input_textEdited(const QString &arg1)
{
    MainWindow::updateCommand(arg1, 1);
}

void MainWindow::on_exe_input_textEdited(const QString &arg1)
{
    MainWindow::updateCommand(arg1, 3);
}

void MainWindow::on_tsan_btn_clicked()
{
    QString detector = "-d tsan";
     MainWindow::updateCommand(detector, 2);
}
