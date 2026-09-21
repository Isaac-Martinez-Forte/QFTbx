/**
 * @file
 * @brief Entry point of the backend test binary.
 *
 * Creates the core application object some backend code expects and sets
 * the numeric locale to C before running the tests: the application object
 * adopts the system locale, and under a decimal-comma locale the number
 * readers would reject literals such as "0.1".
 */

#include <gtest/gtest.h>

#include <clocale>

#include <QCoreApplication>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    std::setlocale(LC_NUMERIC, "C");

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
