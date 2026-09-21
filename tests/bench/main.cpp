/**
 * @file
 * @brief Entry point of the benchmark test suite.
 *
 * A core application is built for the Qt classes the library uses, and the
 * numeric locale is reset to "C" right after it, as the program's own entry
 * point does: the application adopts the system locale, and under a
 * decimal-comma one the number readers reject literals like "0.1". A suite
 * must run under the same numeric locale as the program it tests.
 */

#include <gtest/gtest.h>

#include <clocale>

#include <QCoreApplication>

int main(int argc, char ** argv)
{
    QCoreApplication application(argc, argv);

    std::setlocale(LC_NUMERIC, "C");

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
