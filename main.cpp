#include "GUI/main_window.h"
#include <QApplication>
#include <clocale>
#include <memory>
#include <QIcon>

qint32 main(qint32 argc, char *argv[])
{
    QApplication a(argc, argv);

    //QApplication adopta el locale del sistema (LC_ALL); con locales de coma
    //decimal (es_ES, de_DE...) muParserX deja de aceptar literales como
    //"0.1" y ninguna expresion con decimales evalua. La parte numerica del
    //programa trabaja siempre con punto decimal.
    std::setlocale(LC_NUMERIC, "C");

    a.setWindowIcon(QIcon(":/icons/qftbx_256.png"));

    auto w = std::make_shared<MainWindow>();
    w->setWindowIcon(QIcon(":/icons/qftbx_256.png"));
    w->show();

    return a.exec();
}
