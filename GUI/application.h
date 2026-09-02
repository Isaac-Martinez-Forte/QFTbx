#ifndef QFTBX_GUI_APPLICATION_H
#define QFTBX_GUI_APPLICATION_H

#include <QApplication>

namespace qftbx {

/**
 * @brief QApplication that reports a backend error instead of dying of it.
 *
 * An exception thrown inside a slot propagates out through the event loop,
 * and Qt has nowhere to put it: the process aborts and the user loses the
 * project with only a terminate message on a console they are not looking
 * at. That is not hypothetical - pressing OK on a freshly opened
 * frequencies dialog did exactly that, because the Omega constructor
 * refuses an empty frequency set.
 *
 * Each individual case belongs guarded where it happens, with the offending
 * field marked; this is the net underneath, so that the next one to be
 * missed is a message rather than a crash.
 *
 * mup::ParserError is caught too, and by name: muParserX's error type does
 * NOT derive from std::exception, so a catch of that would let it through.
 */
class Application : public QApplication
{
public:
    Application(int & argc, char ** argv);

    bool notify(QObject * receiver, QEvent * event) override;
};

} // namespace qftbx

#endif // QFTBX_GUI_APPLICATION_H
