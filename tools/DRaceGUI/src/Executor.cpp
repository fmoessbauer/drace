#include "Executor.h"

Executor::Executor()
{
}

void Executor::execute(QObject* parent, Command_Handler * ch)
{
  /*  QString command = "powershell";
    QStringList args;
    args << "-NoExit" << ("\"" + ch->make_command() + "\"");

    QProcess proc;

    proc.setProgram(command);
    proc.setArguments(args);
    proc.startDetached();*/
}

void Executor::exe_custom(std::string cmd, QObject* parent)
{
}

bool Executor::exe_drrun(QString cmd, QObject* parent)
{
    if (cmd.endsWith("drrun") || cmd.endsWith("drrun.exe")) {
       
        QProcess *proc_ovpn = new QProcess(parent);
        proc_ovpn->start(cmd, QStringList() << "-version");
        proc_ovpn->setProcessChannelMode(QProcess::MergedChannels);

        if (proc_ovpn->waitForFinished()) {
            QString str(proc_ovpn->readAllStandardOutput());

            if (str.contains("drrun version 7.", Qt::CaseInsensitive)) {
                return true;
            }
        }
    }
    return false;
}


