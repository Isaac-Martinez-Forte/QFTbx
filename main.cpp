#include "GUI/windowsgeneral.h"
#include <QApplication>
#include <memory>
#include <QIcon>

qint32 main(qint32 argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setWindowIcon(QIcon(":/icons/qftbx_256.png"));

    auto w = std::make_shared<WindowsGeneral>();
    w->setWindowIcon(QIcon(":/icons/qftbx_256.png"));
    w->show();

    return a.exec();
}
