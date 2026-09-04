#ifndef QFTBX_MUPARSERX_CONSOLE_H
#define QFTBX_MUPARSERX_CONSOLE_H

#include <QString>

/**
 * @brief Opens muParserX's own interactive example in a terminal, as a way
 * of trying expressions by hand against the very parser the toolbox uses.
 *
 * PARKED, and deliberately kept. It never worked outside the machine it
 * was written on: it launches two hardcoded relative paths,
 * ./literm/bin/literm and ./muparserx/bin/example, neither of which the
 * repository ships nor the build produces. Because the historical version
 * ignored the result of a blocking QProcess::execute(), the menu entry did
 * nothing at all and said nothing about it - the terminal emulator was
 * simply never there. The menu action is disabled now, so nobody clicks a
 * no-op; launch() reports honestly when it cannot start.
 *
 * To bring it back: build muParserX's example target, decide on a terminal
 * (or host a console inside the application, which would be the better
 * answer), and point the two paths below at real files.
 */

namespace qftbx {

class MuParserXConsole
{
public:

    /**
     * @brief Launches the console, without blocking the event loop.
     *
     * @param error when it fails, receives what could not be started.
     * @return whether the process was started.
     */
    static bool launch(QString * error = nullptr);

    /// The terminal emulator, relative to the working directory.
    static QString terminalPath();

    /// muParserX's interactive example, relative to the working directory.
    static QString parserExamplePath();
};

} // namespace qftbx

#endif // QFTBX_MUPARSERX_CONSOLE_H
