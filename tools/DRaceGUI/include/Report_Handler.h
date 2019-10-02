#ifndef REPORT_HANDLER_H
#define REPORT_HANDLER_H

#include "QDirIterator"
#include <string>
#include <QString>

class Report_Handler
{
    static constexpr char* report_converter_name = "ReportConverter";

    QString rep_conv_path;
    QString rep_name = "test_report.xml";

    void find_report_converter();

public:
    Report_Handler();


    QString get_rep_conv_path() { return rep_conv_path; }
    QString get_rep_name() { return rep_name; }

    void set_report_converter(QString path);
    void set_report_name(QString name);

};

#endif 
