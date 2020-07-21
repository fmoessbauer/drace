/*
 * DRace, a dynamic data race detector
 *
 * Copyright 2020 Siemens AG
 *
 * Authors:
 *   Mohamad Kanj <mohamad.kanj@siemens.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "Options_Dialog.h"
#include "ui_Options_Dialog.h"

const QString Options_Dialog::CONFIG_EXT = ".drc";

Options_Dialog::Options_Dialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::Options_Dialog) {
  ui->setupUi(this);
}

Options_Dialog::~Options_Dialog() { delete ui; }

void Options_Dialog::on_buttonBox_clicked() { this->close(); }

void Options_Dialog::on_set_registry_clicked() {
  QSettings scu("HKEY_CURRENT_USER\\SOFTWARE\\CLASSES",
                QSettings::NativeFormat);
  scu.sync();

  if (!scu.childGroups().contains("Siemens.DRace")) {
    // register class: Siemens.DRace to be used as ProgID
    scu.setValue("Siemens.DRace/shell/open/command/.",
                 QDir::toNativeSeparators(qApp->applicationFilePath()) + " %1");
    scu.setValue("Siemens.DRace/DefaultIcon/.",
                 QDir::toNativeSeparators(qApp->applicationFilePath()));
    scu.setValue("Siemens.DRace/.", "DRace Configuration File");
  }

  QSettings slm("HKEY_LOCAL_MACHINE\\SOFTWARE\\CLASSES",
                QSettings::NativeFormat);
  slm.sync();

  // create file extension if it does not exist in the Windows registry
  if (!scu.childGroups().contains(CONFIG_EXT) &&
      !slm.childGroups().contains(CONFIG_EXT)) {
    scu.setValue(CONFIG_EXT + "/OpenWithProgids/Siemens.DRace", "");
    // set DRaceGUI as default application to open config file with ext
    scu.setValue(CONFIG_EXT + "/.", "Siemens.DRace");

    scu.sync();
    auto status = scu.status();
    if (status == QSettings::NoError) {
      QMessageBox temp;
      temp.setWindowTitle("Options");
      temp.setIcon(QMessageBox::Information);
      temp.setText("The DRaceGUI has been set as the default application for " +
                   CONFIG_EXT + " file type.");
      temp.exec();
    } else {
      QMessageBox temp;
      temp.setWindowTitle("Options");
      temp.setIcon(QMessageBox::Warning);
      temp.setText(
          "Problems writing to the Windows system registry were detected.");
      temp.exec();
    }
  }
  // file extension already exists
  else {
    // file extension is already associated with DRace
    if (scu.contains(CONFIG_EXT + "/OpenWithProgids/Siemens.DRace")) {
      QMessageBox temp;
      temp.setWindowTitle("Options");
      temp.setIcon(QMessageBox::Warning);
      temp.setText("The DRace configuration file extension " + CONFIG_EXT +
                   " already exists in the Windows system registry.");
      temp.exec();
    } else {
      // file extension is associated with another application
      QMessageBox temp;
      temp.setWindowTitle("Options");
      temp.setIcon(QMessageBox::Warning);
      temp.setText(
          "The file extension " + CONFIG_EXT +
          " is already associated with another application or program.");
      temp.exec();
    }
  }
}

void Options_Dialog::on_revert_registry_clicked() {
  QSettings scu("HKEY_CURRENT_USER\\SOFTWARE\\CLASSES",
                QSettings::NativeFormat);
  scu.sync();

  // no settings to revert exist
  if (!scu.contains(CONFIG_EXT + "/OpenWithProgids/Siemens.DRace") &&
      !scu.childGroups().contains("Siemens.DRace")) {
    QMessageBox temp;
    temp.setWindowTitle("Options");
    temp.setIcon(QMessageBox::Warning);
    temp.setText("No settings have been set yet.");
    temp.exec();
  } else {
    // remove config file ext associated with DRace
    if (scu.contains(CONFIG_EXT + "/OpenWithProgids/Siemens.DRace")) {
      scu.remove(CONFIG_EXT);
    }
    // remove ProgID class: Siemens.DRace
    if (scu.childGroups().contains("Siemens.DRace")) {
      scu.remove("Siemens.DRace");
    }

    scu.sync();
    auto status = scu.status();
    if (status == QSettings::NoError) {
      QMessageBox temp;
      temp.setWindowTitle("Options");
      temp.setIcon(QMessageBox::Information);
      temp.setText("Your settings have been successfully reverted.");
      temp.exec();
    } else {
      QMessageBox temp;
      temp.setWindowTitle("Options");
      temp.setIcon(QMessageBox::Warning);
      temp.setText(
          "Problems reverting data in the Windows system registry were "
          "detected.");
      temp.exec();
    }
  }
}