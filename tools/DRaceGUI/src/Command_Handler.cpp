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

#include "Command_Handler.h"


Command_Handler::Command_Handler(QString default_detector) {
    make_entry(default_detector, DETECTOR, "-d");
}

QString Command_Handler::make_entry(const QString &path, uint position, QString prefix)
{
    if (path != "") {
        QString temp = path;
        if (temp.contains(QRegExp("\\s+"))) {
            temp = "\"" + temp + "\"";
        }
        if (prefix != "") {
            temp = prefix + " " + temp;
        }
        return updateCommand(temp, position);
    }
    else {
        return updateCommand("", position);
    }
}

QString Command_Handler::updateCommand(const QString &arg1, uint position) {
    command[position] = arg1;

    entire_command = make_command();
    return entire_command;
}

QString Command_Handler::make_command() {
    entire_command = "";
    for (int it = 0; it < (EXECUTABLE + 1); it++) {
        if (command[it] != "") {
            entire_command.append(command[it]);
            entire_command.append(" ");
        }
    }
    return entire_command;
}

QString Command_Handler::make_flag_entry(const QString &arg1) {
    QList<QString> split_string = arg1.split(QRegExp("\\s+")); //regex of one or many whitespaces

    QString to_append;
    for (auto it = split_string.begin(); it != split_string.end(); it++) {
        QString temp = *it;
        if (temp.contains(" ")) {
            temp = "\"" + *it + "\"";
        }
        if (it == split_string.begin()) {
            to_append = temp;
        }
        else {
            to_append += (" " + temp);
        }
    }
    return updateCommand(to_append, FLAGS);
}


