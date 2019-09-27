#ifndef DRACE_GUI_H
#define DRACE_GUI_H

#include <QMainWindow>
#include <string>
#include <QFileDialog>
#include <QTextEdit>
#include <QClipboard>
#include <QApplication>

QT_BEGIN_NAMESPACE
namespace Ui { class DRaceGUI; }
QT_END_NAMESPACE

class DRaceGUI : public QMainWindow
{
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


    void on_dr_path_input_textEdited(const QString &arg1);

    void updateCommand(const QString &arg1, uint position);

    void on_copy_button_clicked();



    void on_drace_path_input_textEdited(const QString &arg1);

    void on_exe_input_textEdited(const QString &arg1);

    void on_tsan_btn_clicked();

private:
    Ui::DRaceGUI *ui;
    QString command[5];
    QString entire_command;
    QClipboard *clipboard = QApplication::clipboard();

};
#endif // MAINWINDOW_H
