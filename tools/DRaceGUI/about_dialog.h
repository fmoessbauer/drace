#ifndef ABOUT_DIALOG_H
#define ABOUT_DIALOG_H

#include <QDialog>

namespace Ui {
class about_dialog;
}

class about_dialog : public QDialog
{
    Q_OBJECT

public:
    explicit about_dialog(QWidget *parent = nullptr);
    ~about_dialog();

private slots:


private:
    Ui::about_dialog *ui;
};

#endif // ABOUT_DIALOG_H
