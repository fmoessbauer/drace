#include "draceGUI.h"
#include "./ui_draceGUI.h"


DRaceGUI::DRaceGUI(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::DRaceGUI)
{
    ui->setupUi(this);


//set some default values
    command[detector_pos] = "-d tsan";
    ui->dr_path_input->setText("drrun.exe");
}

DRaceGUI::~DRaceGUI()
{
    delete ui;
}

///Button functions
void DRaceGUI::on_dr_path_btn_clicked()
{
    QString path = QFileDialog::getOpenFileName(this, "Open File", dr_pth_cache);
    dr_pth_cache = path;
    ui->dr_path_input->setText(path);
}

void DRaceGUI::on_drace_path_btn_clicked()
{
     QString path = QFileDialog::getOpenFileName(this, "Open File", drace_pth_cache);
     drace_pth_cache = path;
     ui->drace_path_input->setText(path);
}

void DRaceGUI::on_exe_path_btn_clicked()
{
     QString path = QFileDialog::getOpenFileName(this, "Open File", exe_pth_cache);
     exe_pth_cache = path;
     ui->exe_input->setText(path);
}

void DRaceGUI::on_config_browse_btn_clicked()
{
     QString path = QFileDialog::getOpenFileName(this, "Open File", config_pth_cache);
     config_pth_cache = path;
     ui->config_file_input->setText(path);
}

void DRaceGUI::on_run_button_clicked()
{
    QString command = "start powershell -NoExit \"" + make_command() + "\"";
    std::string str_command = command.toStdString();
    system(str_command.c_str());
}

void DRaceGUI::on_copy_button_clicked()
{
    clipboard->setText(entire_command);
}

void DRaceGUI::on_tsan_btn_clicked()
{
    make_entry("tsan", detector_pos, "-d");
}

void DRaceGUI::on_extsan_btn_clicked()
{
    make_entry("dummy", detector_pos, "-d");
}

void DRaceGUI::on_fasttrack_btn_clicked()
{
    make_entry("fasttrack", detector_pos, "-d");
}

void DRaceGUI::on_dr_debug_stateChanged(int arg1)
{
    if(arg1 == 0){
        updateCommand("", dr_debug_pos);
    }
    else{
        updateCommand("-debug", dr_debug_pos);
    }
}


///helper functions
void DRaceGUI::make_entry(const QString &path, uint position, QString prefix)
{
    if(path != ""){
        QString temp = path;
        if(temp.contains(QRegExp("\\s+"))){
            temp = "\"" + temp + "\"";
        }
        if(prefix != ""){
            temp = prefix + " " + temp;
        }
        updateCommand(temp, position);
    }
    else{
        updateCommand("", position);
    }
}

void DRaceGUI::updateCommand(const QString &arg1, uint position){
    command[position] = arg1;

    QString entire_command = make_command();
    ui->command_output->setText(entire_command);
}

QString DRaceGUI::make_command(){
    QString entire_command = "";
    for(int it=0; it < (exe_pos+1); it++){
        if(command[it] != ""){
            entire_command.append(command[it]);
            entire_command.append(" ");
        }
    }
    return entire_command;
}

void DRaceGUI::make_flag_entry(const QString &arg1) {
    QList<QString> split_string = arg1.split(QRegExp("\\s+")); //regex of one or many whitespaces

    QString to_append;
    for (auto it = split_string.begin(); it != split_string.end(); it++) {
        QString temp = *it;
        if(temp.contains(" ")){
            temp = "\"" + *it + "\"";
        }
        if(it == split_string.begin()){
            to_append = temp;
        }
        else{
            to_append += (" " + temp);
        }
    }
    updateCommand(to_append, flags_pos);
}

///textbox functions
void DRaceGUI::on_dr_path_input_textChanged(const QString &arg1)
{
    make_entry(arg1, dr_pos);

    if(arg1.endsWith("drrun") || arg1.endsWith("drrun.exe")){
       std::string call = arg1.toStdString() + " -version";
       auto ret = system(call.c_str());

        if(ret == 0){
            QPalette pal;
            pal.setColor(QPalette::Base, Qt::green);
            ui->dr_path_input->setPalette(pal);
        }
        else{
            QPalette pal;
            pal.setColor(QPalette::Base, Qt::red);
            ui->dr_path_input->setPalette(pal);
        }
    }
    else{
        QPalette pal;
        pal.setColor(QPalette::Base, Qt::red);
        ui->dr_path_input->setPalette(pal);
    }
}

void DRaceGUI::on_drace_path_input_textChanged(const QString &arg1)
{
    make_entry(arg1, drace_pos, "-c");
}

void DRaceGUI::on_exe_input_textChanged(const QString &arg1)
{
    make_entry(arg1, exe_pos, "--");
}

void DRaceGUI::on_flag_input_textChanged(const QString &arg1)
{
    make_flag_entry(arg1);
}

void DRaceGUI::on_config_file_input_textChanged(const QString & arg1)
{
    make_entry(arg1, config_pos, "-c");
}



///action button functions
void DRaceGUI::on_actionAbout_triggered()
{
    about_dialog newWindow;
    newWindow.exec();
}

void DRaceGUI::on_actionLoad_Config_triggered()
{
    QString path = QFileDialog::getOpenFileName(this, "Open File", "C:\\");

    //open file here

}

void DRaceGUI::on_actionSave_Config_triggered()
{
      QString path = QFileDialog::getSaveFileName(this, "Save file", "C:\\");

     //save file here
}

void DRaceGUI::on_actionConfigure_Report_triggered()
{
    report_config newWindow;
    newWindow.exec();
}
