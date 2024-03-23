#include "GUI/windowsgeneral.h"
#include <QApplication>
#include <memory>

qint32 main(qint32 argc, char *argv[])
{
    QApplication a(argc, argv);

    auto w = std::make_shared<WindowsGeneral>();
    w->show();
    
    return a.exec();
}


