#include "GUI/windowsgeneral.h"
#include <QApplication>

qint32 main(qint32 argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    
    QApplication a(argc, argv);

    WindowsGeneral * w = new WindowsGeneral();
    w->show();
    
    qint32 i = a.exec();

    delete w;
    return i;

}


