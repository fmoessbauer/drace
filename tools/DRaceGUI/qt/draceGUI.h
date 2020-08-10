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

#ifndef DRACE_GUI_H
#define DRACE_GUI_H
#include <windows.h>
#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QDirIterator>
#include <QErrorMessage>
#include <QFileDialog>
#include <QMainWindow>
#include <QMessageBox>
#include <QTextEdit>
#include <fstream>
#include <string>
#include "Command_Handler.h"
#include "Executor.h"
#include "Load_Save.h"
#include "Report_Handler.h"
#include "about_dialog.h"
#include "options_dialog.h"
#include "report_config.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class DRaceGUI;
}
QT_END_NAMESPACE

class DRaceGUI : public QMainWindow {
  Q_OBJECT

 public:
  explicit DRaceGUI(QString config, QWidget *parent = nullptr);
  ~DRaceGUI();

 private slots:

  /// Push buttons
  void on_dr_path_btn_clicked();
  void on_drace_path_btn_clicked();
  void on_exe_path_btn_clicked();
  void on_config_browse_btn_clicked();
  void on_run_button_clicked();
  void on_copy_button_clicked();

  /// Text Boxes
  void on_dr_path_input_textChanged(const QString &arg1);
  void on_drace_path_input_textChanged(const QString &arg1);
  void on_exe_input_textChanged(const QString &arg1);
  void on_exe_args_input_textChanged(const QString &arg1);
  void on_flag_input_textChanged(const QString &arg1);
  void on_config_file_input_textChanged(const QString &arg1);

  /// check boxes
  void on_dr_debug_stateChanged(int arg1);
  void on_report_creation_stateChanged(int arg1);
  void on_msr_box_stateChanged(int arg1);
  void on_excl_stack_box_stateChanged(int arg1);

  /// Radio Buttons
  void on_tsan_btn_clicked();
  void on_fasttrack_btn_clicked();
  void on_dummy_btn_clicked();

  /// Action Buttons
  void on_actionAbout_triggered();
  void on_actionLoad_Config_triggered();
  void on_actionSave_Config_triggered();
  void on_actionConfigure_Report_triggered();
  void on_actionHelp_triggered();
  void on_actionOptions_triggered();

 private:
  Ui::DRaceGUI *ui;

  /// path caches for all the browse buttons
  QString DEFAULT_PTH = QDir::currentPath();
  QString drace_pth_cache = DEFAULT_PTH;
  QString dr_pth_cache = DEFAULT_PTH;
  QString exe_pth_cache = DEFAULT_PTH;
  QString config_pth_cache = DEFAULT_PTH;
  QString load_save_path = DEFAULT_PTH;

  QClipboard *clipboard = QApplication::clipboard();

  /// Handler classes
  Report_Handler *rh;
  Command_Handler *ch;

  /// restores the GUI with the data of the loaded config file
  void set_boxes_after_load();
  QString remove_quotes(QString str);
};
#endif  // MAINWINDOW_H
