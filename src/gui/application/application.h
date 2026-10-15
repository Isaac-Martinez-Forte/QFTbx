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
 *
 * On macOS the system asks to open a document with an event rather than an
 * argument, and it may come before the window exists or after; the
 * application keeps the file asked for, for takeAskedFile(), and announces
 * it with fileOpenAsked().
 */

#ifndef QFTBX_GUI_APPLICATION_H
#define QFTBX_GUI_APPLICATION_H

#include <QApplication>

namespace qftbx {

class Application : public QApplication
{
    Q_OBJECT

public:
    Application(int & argc, char ** argv);

    bool notify(QObject * receiver, QEvent * event) override;

    QString takeAskedFile();

signals:
    void fileOpenAsked(const QString & fileName);

protected:
    bool event(QEvent * event) override;

private:
    QString m_askedFile;
};

}

#endif
