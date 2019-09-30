#include "report_config.h"
#include "ui_report_config.h"

report_config::report_config(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::report_config)
{
    ui->setupUi(this);
}

report_config::~report_config()
{
    delete ui;
}
