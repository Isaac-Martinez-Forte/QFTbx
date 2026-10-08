/**
 * @file
 * @brief The application object, which reports a backend error instead of
 * dying of it.
 *
 * An exception that leaves a slot propagates out through the event loop,
 * where Qt has nowhere to put it: the process aborts and the user loses
 * the project with only a message on a console they are not looking at.
 * The class declared here overrides event delivery so that a toolbox
 * exception, and after it any other standard exception, becomes an error
 * message: a failure that can still say what happened does. Each case
 * still belongs guarded where it happens, with the offending field marked;
 * this is the net underneath.
 */

#ifndef QFTBX_GUI_APPLICATION_H
#define QFTBX_GUI_APPLICATION_H

#include <QApplication>

namespace qftbx {

class Application : public QApplication
{
public:
    Application(int & argc, char ** argv);

    bool notify(QObject * receiver, QEvent * event) override;
};

}

#endif
