//Entry point of the GUI smoke suite: the dialogs need a QApplication, and
//the platform plugin is forced to "offscreen" so the suite runs on a
//machine with no display (a build server, or this one).

#include <clocale>
#include <cstdlib>

#include <gtest/gtest.h>

#include "GUI/application.h"

int main(int argc, char ** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");

    //The same Application the program uses, so the suite exercises the
    //safety net that catches a backend error escaping a slot.
    qftbx::Application application(argc, argv);

    //The same reset the application does in main.cpp, and for the same
    //reason: QApplication adopts the system locale, and with a decimal-comma
    //one (es_ES, de_DE...) muParserX stops accepting literals like "0.01",
    //so no expression with decimals evaluates. Without this the suite runs
    //under a different numeric locale than the program it is testing, which
    //both invents failures and hides real ones.
    std::setlocale(LC_NUMERIC, "C");

    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
