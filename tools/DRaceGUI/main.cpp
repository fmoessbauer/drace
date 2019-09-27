#include "draceGUI.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    DRaceGUI w;
    w.show();
    return a.exec();
}
