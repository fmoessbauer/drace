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

Report_Handler::Report_Handler(Command_Handler *c_handler, QObject* parent):
ch(c_handler)
{
    find_report_converter(parent);
}

///checks if the report converter is in the current directory or in the ../ReportConverter/ dir
void Report_Handler::find_report_converter(QObject* parent) {

    QDir current_dir = QDir::currentPath();
    QDirIterator dir(current_dir);

    //search in current directory
    while (dir.hasNext()) {
        if (eval_rep_conv(dir.next(), parent)) {
            return;
        }
    }

    //search in ../ReportConverter/ dir
    std::string path_adj = "../" + std::string(REP_CONV_DEFAULT);
    current_dir.cd(QString(path_adj.c_str()));
    QDirIterator other(current_dir);
    while (other.hasNext()) {
        if (eval_rep_conv(other.next(), parent)) {
            return;
        }
    }
 }


///evaluates if name is a valid ReportConverter (must be called with existing files)
bool Report_Handler::eval_rep_conv(QString name, QObject* parent) {
    auto split_slashes = name.split("/");
    auto split_point = split_slashes.last().split(".");

    if (split_point.first() == REP_CONV_DEFAULT) {
        if (split_point.last() == "exe") {
            rep_conv_cmd = name;
            is_python = false;
            return true;
        }

        if (split_point.last() == "py") {
            //Note: change to proper call with QProcess -> check for python 3 version
            if (Executor::exe_python3(parent)) {
                rep_conv_cmd = name;
                is_python = true;
                return true;
            }
        }
    }
    return false;
}

///sets the report name
void Report_Handler::set_report_name(QString name) {
    //function is and must be only called with valid names
    rep_name = name;
}

///sets, when the actual report creation command is valid, the current command in the command handler class
bool Report_Handler::set_report_command() {
    QString cmd = "";
    QString prefix;
    bool is_valid = command_valid();
    if (is_valid) {
        cmd = make_command();
        is_python ? prefix = "python " : prefix = ""; //add python, if is_python        

        ch->make_entry(rep_name, Command_Handler::REPORT_FLAG, "--xml-file");
    }
    else {
        ch->make_entry("", Command_Handler::REPORT_FLAG);
    }

    ch->make_entry(cmd, Command_Handler::REPORT_CMD, prefix, true);
    return is_valid;
}

///returns true when name, report converter is valid and the checkbox "report" is set
bool Report_Handler::command_valid() {
    if (rep_conv_cmd != "" && rep_name != "" && create_report) {
        return true;
    }
    return false;
}

///sets the state of the report check box
void Report_Handler::set_create_state(bool state) {
    create_report = state;
}

QString Report_Handler::make_command() {
    QString converter = rep_conv_cmd, output_name = rep_name;
    if (rep_conv_cmd.contains(QRegExp("\\s+"))) {
        converter = "\"" + rep_conv_cmd + "\"";
    }
    if (rep_name.contains(QRegExp("\\s+"))) {
        output_name = "\"" + rep_name + "\"";
    }

    return (converter + " -i " + output_name);
}
