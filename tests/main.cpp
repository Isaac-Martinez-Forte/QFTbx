#include <gtest/gtest.h>

#include <QCoreApplication>

int main(int argc, char **argv)
{
    // Some backend code still relies on Qt facilities that expect an
    // application object to exist.
    QCoreApplication app(argc, argv);

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
