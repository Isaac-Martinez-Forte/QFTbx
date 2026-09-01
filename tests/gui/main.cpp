//Entry point of the GUI smoke suite: the dialogs need a QApplication, and
//the platform plugin is forced to "offscreen" so the suite runs on a
//machine with no display (a build server, or this one).

#include <cstdlib>

#include <gtest/gtest.h>

#include <QApplication>

int main(int argc, char ** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");

    QApplication application(argc, argv);

    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
