#include "Report_Handler.h"

Report_Handler::Report_Handler(){
    find_report_converter();
}


void Report_Handler::find_report_converter() {
    QDirIterator it(QDir::currentPath());
    while (it.hasNext()) {
        if (it.next().toStdString() == (std::string(report_converter_name) + ".py")) {
            if (system("python -V") == 0) {  //check for python 3 version
                std::string path = std::string(report_converter_name) + ".py";
                rep_conv_path = QString(path.c_str());
                break;
            }
        }
        if (it.next().toStdString() == (std::string(report_converter_name) + ".exe")) {
            std::string path = std::string(report_converter_name) + ".exe";
            rep_conv_path = QString(path.c_str());
            break;
        }
    }
}


void Report_Handler::set_report_converter(QString path) {
    rep_conv_path = path;
}

void Report_Handler::set_report_name(QString name) {
    rep_name = name;
}
