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

#include "Report_Handler.h"

Report_Handler::Report_Handler(){
    find_report_converter();
}


void Report_Handler::find_report_converter() {

    QDirIterator dir(QDir::currentPath());

    while (dir.hasNext()) {
        if (eval_rep_conv(dir.next())) {
            break;
        }
    }
 }

bool Report_Handler::eval_rep_conv(QString name) {
    auto split_slashes = name.split("/");
    auto split_point = split_slashes.last().split(".");

    if (split_point.first() == REP_CONV_DEFAULT) {
        if (split_point.last() == "exe") {
            rep_conv_cmd = name;
            return true;
        }

        if (split_point.last() == "py") {
            //Note: change to proper call with QProcess -> check for python 3 version
            if (system("python -V") == 0) {
                rep_conv_cmd = "python " + name;
                return true;
            }
        }
    }
    return false;
}


bool Report_Handler::set_report_converter(QString path) {
    return eval_rep_conv(path);
}

bool Report_Handler::set_report_name(QString name) {
    if (name != "") {
       // check here if filename is valid
        rep_name = name;
        return true;
    }
    return false;
}

std::string Report_Handler::get_report_command() {
    return rep_conv_cmd.toStdString() + " -i " + rep_name.toStdString();
}

bool Report_Handler::command_valid() {
    if (rep_conv_cmd == "") {
        return false;
    }
    return true;
}
