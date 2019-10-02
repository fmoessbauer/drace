#ifndef EXECUTOR_H
#define EXECUTOR_H
#include <string>
#include <QString>
#include <QProcess>
#include "Command_Handler.h"


class Executor {


public:
    Executor();
    void execute(QObject* parent, Command_Handler * ch);
    void exe_custom(std::string cmd, QObject* parent);
    bool exe_drrun(QString cmd, QObject* parent);


};




#endif // !EXECUTOR_H
