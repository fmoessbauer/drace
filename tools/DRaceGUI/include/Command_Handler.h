#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H
#include <QString>
#include <QRegExp>
#include <QList>

class Command_Handler {
public:
    static constexpr uint DYNAMORIO = 0;
    static constexpr uint DR_DEBUG = 1;
    static constexpr uint DRACE = 2;
    static constexpr uint DETECTOR = 3;
    static constexpr uint FLAGS = 4;
    static constexpr uint CONFIGURATION = 5;
    static constexpr uint EXECUTABLE = 6;

private:
    QString command[(EXECUTABLE + 1)];
    QString entire_command;


public:
    Command_Handler(QString default_detector = "tsan");

    QString updateCommand(const QString &arg1, uint position);
    QString make_flag_entry(const QString & arg1);
    QString make_entry(const QString & path, uint postion, QString prefix = "");
    QString make_command();

    QString get_command() { return entire_command; }

};


#endif // !COMMAND_HANDLER_H
