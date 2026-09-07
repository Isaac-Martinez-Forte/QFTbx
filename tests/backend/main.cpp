#include <gtest/gtest.h>

#include <clocale>

#include <QCoreApplication>

int main(int argc, char **argv)
{
    // Some backend code still relies on Qt facilities that expect an
    // application object to exist.
    QCoreApplication app(argc, argv);

    // Same as the application's main(): QCoreApplication adopts the system
    // locale, and with a decimal-comma locale (es_ES...) the number readers
    // reject literals like "0.1". Numeric code always works with the C locale.
    std::setlocale(LC_NUMERIC, "C");

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
