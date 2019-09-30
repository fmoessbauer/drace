#ifndef DRACE_GUI_H
#define DRACE_GUI_H
#include <windows.h>
#include <QMainWindow>
#include <string>
#include <QFileDialog>
#include <QTextEdit>
#include <QClipboard>
#include <QApplication>
#include <QDesktopServices>
#include <QMessageBox>
#include "about_dialog.h"
#include "report_config.h"

QT_BEGIN_NAMESPACE
namespace Ui { class DRaceGUI; }
QT_END_NAMESPACE



class DRaceGUI : public QMainWindow
{
    static constexpr uint dr_pos = 0;
    static constexpr uint dr_debug_pos = 1;
    static constexpr uint drace_pos = 2;
    static constexpr uint detector_pos = 3;
    static constexpr uint flags_pos = 4;
    static constexpr uint config_pos = 5;
    static constexpr uint exe_pos = 6;

    Q_OBJECT

public:
    
    DRaceGUI(QWidget *parent = nullptr);
    ~DRaceGUI();

private slots:
    

    void on_dr_path_btn_clicked();

    void on_drace_path_btn_clicked();

    void on_exe_path_btn_clicked();

    void on_config_browse_btn_clicked();

    void on_run_button_clicked();

    void make_entry(const QString & path, uint postion, QString prefix = "");

    void on_dr_path_input_textChanged(const QString &arg1);

    void updateCommand(const QString &arg1, uint position);

    void on_copy_button_clicked();

    void make_flag_entry(const QString & arg1);



    void on_drace_path_input_textChanged(const QString &arg1);

    void on_exe_input_textChanged(const QString &arg1);

    void on_tsan_btn_clicked();
    void on_fasttrack_btn_clicked();
    void on_extsan_btn_clicked();


    void on_flag_input_textChanged(const QString &arg1);

    void on_config_file_input_textChanged(const QString &arg1);

    void on_dr_debug_stateChanged(int arg1);

    QString make_command();

    void on_actionAbout_triggered();

    void on_actionLoad_Config_triggered();

    void on_actionSave_Config_triggered();

    void on_actionConfigure_Report_triggered();

private:

    QString DEFAULT_PTH =  "C:\\";
    QString drace_pth_cache = DEFAULT_PTH;
    QString dr_pth_cache = DEFAULT_PTH;
    QString exe_pth_cache = DEFAULT_PTH;
    QString config_pth_cache = DEFAULT_PTH;

    Ui::DRaceGUI *ui;
    QString command[(exe_pos+1)];
    QString entire_command;
    QClipboard *clipboard = QApplication::clipboard();
};
#endif // MAINWINDOW_H
