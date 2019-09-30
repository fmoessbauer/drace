#ifndef REPORT_CONFIG_H
#define REPORT_CONFIG_H

#include <QDialog>

namespace Ui {
class report_config;
}

class report_config : public QDialog
{
    Q_OBJECT

public:
    explicit report_config(QWidget *parent = nullptr);
    ~report_config();

private:
    Ui::report_config *ui;
};

#endif // REPORT_CONFIG_H
