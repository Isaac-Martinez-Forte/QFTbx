#include "muparserx_console.h"

#include <QFileInfo>
#include <QProcess>
#include <QStringList>

namespace qftbx {

namespace {

//Where the two pieces were expected to be, relative to the working
//directory. Neither is built by this project; see the class note.
const char * kTerminal = "./literm/bin/literm";
const char * kExample  = "./muparserx/bin/example";

} // namespace


QString MuParserXConsole::terminalPath()
{
    return QString::fromUtf8(kTerminal);
}


QString MuParserXConsole::parserExamplePath()
{
    return QString::fromUtf8(kExample);
}


bool MuParserXConsole::launch(QString * error)
{
    //Checked before starting so the caller can say WHICH half is missing.
    //The historical version called QProcess::execute() and dropped its
    //return value, so a missing terminal was indistinguishable from a
    //console the user had opened and closed.
    for (const QString & path : {terminalPath(), parserExamplePath()}) {
        if (!QFileInfo::exists(path)) {
            if (error != nullptr) {
                *error = path;
            }
            return false;
        }
    }

    //Detached, not execute(): execute() blocks the event loop until the
    //process exits, which would have frozen the whole application for as
    //long as the console stayed open.
    return QProcess::startDetached(terminalPath(),
                                   QStringList{"-e", parserExamplePath()});
}

} // namespace qftbx
