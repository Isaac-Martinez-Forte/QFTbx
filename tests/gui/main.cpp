/**
 * @file
 * @brief Entry point of the GUI test suite.
 *
 * The platform plugin is forced to offscreen so the suite runs with no
 * display, and the application object is the program's own, so the tests
 * exercise the net that catches a backend error escaping a slot. The numeric
 * locale is reset to "C" right after it, as the program does: the application
 * adopts the system locale, and under a decimal-comma one the number readers
 * stop accepting literals like "0.01".
 */

#include <clocale>
#include <cstdlib>

#include <gtest/gtest.h>

#include "src/gui/application/application.h"

using namespace qftbx;

int main(int argc, char ** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");

    qftbx::Application application(argc, argv);

    std::setlocale(LC_NUMERIC, "C");

    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
