#include "src/core/exception.h"
#include "src/core/settings.h"
#include "src/gui/application.h"
#include "src/gui/main_window.h"
#include <clocale>
#include <memory>
#include <QIcon>
#include <QMessageBox>

#include <iostream>

int main(int argc, char *argv[])
{
    //Reports a backend error instead of letting it kill the process.
    qftbx::Application a(argc, argv);

    //QApplication adopts the system locale (LC_ALL); under a decimal-comma
    //locale (es_ES, de_DE...) muParserX stops accepting literals such as
    //"0.1" and no expression with decimals evaluates. The numeric side of
    //the program always works with the decimal point.
    std::setlocale(LC_NUMERIC, "C");

    a.setWindowIcon(QIcon(":/icons/qftbx_256.png"));

    //The settings are read ONCE, here, and handed down: immutable afterwards,
    //which is what makes them safe next to OpenMP and the search's worker.
    //With no settings file anywhere this succeeds with the compiled defaults,
    //so the program starts as it always has; a file named in QFTBX_CONFIG
    //that cannot be read is an error, and it is worth failing loudly on
    //because naming it says it was meant to be used.
    qftbx::Settings settings;

    try {
        settings = qftbx::loadSettings();
    } catch (const qftbx::Exception & e) {
        QMessageBox::critical(nullptr, QObject::tr("QFTbx settings"), e.what());
        return 1;
    }

    if (!settings.source.empty()) {
        std::cout << "settings: " << settings.source << std::endl;
    }

    for (const std::string & unknown : settings.unknownKeys) {
        std::cout << "settings: \"" << unknown
                  << "\" is not a setting this build knows" << std::endl;
    }

    auto w = std::make_unique<qftbx::MainWindow>(settings);
    w->setWindowIcon(QIcon(":/icons/qftbx_256.png"));
    w->show();

    return a.exec();
}
