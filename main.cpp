/**
 * @file
 * @brief The program: the application, its settings, and the main window.
 *
 * The numeric side always reads and writes with the decimal point, whatever
 * locale the application adopts. The settings are read once, here, and
 * handed down immutable; with no file anywhere the compiled defaults apply,
 * and a file named in QFTBX_CONFIG that cannot be read stops the program,
 * since naming it says it was meant to be used. The record is opened before
 * anything computes, and the theme and the language are applied before any
 * window is built. The window then opens the project the program was
 * started with, from the command line or, on macOS, from the system.
 */

#include "src/core/common/exception.h"
#include "src/core/common/record.h"
#include "src/core/project/settings.h"
#include "src/gui/application/application.h"
#include "src/gui/application/language.h"
#include "src/gui/application/main_window.h"
#include "src/gui/application/theme.h"
#include <clocale>
#include <memory>
#include <QIcon>
#include <QMessageBox>

#include <iostream>

int main(int argc, char *argv[])
{
    qftbx::Application a(argc, argv);

    std::setlocale(LC_NUMERIC, "C");

    QIcon icon;
    for (const char * size : {"16", "32", "64", "128", "256"}) {
        icon.addFile(QString(":/icons/qftbx_%1.png").arg(size));
    }
    a.setWindowIcon(icon);

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

    qftbx::openRecord(settings);
    if (qftbx::record::isOpen()) {
        std::cout << "record: " << qftbx::record::path() << std::endl;
    }

    qftbx::applyTheme(QString::fromStdString(settings.interface.theme));

    qftbx::applyLanguage(QString::fromStdString(settings.interface.language));

    for (const std::string & unknown : settings.unknownKeys) {
        std::cout << "settings: \"" << unknown
                  << "\" is not a setting this build knows" << std::endl;
    }

    auto w = std::make_unique<qftbx::MainWindow>(settings);
    w->show();

    QObject::connect(&a, &qftbx::Application::fileOpenAsked, w.get(), &qftbx::MainWindow::openProject);
    QString asked = a.takeAskedFile();
    for (const QString & argument : QCoreApplication::arguments().mid(1)) {
        if (!argument.startsWith(QLatin1Char('-'))) {
            asked = argument;
            break;
        }
    }
    if (!asked.isEmpty()) {
        w->openProject(asked);
    }

    return a.exec();
}
