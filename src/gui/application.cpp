#include "src/gui/application.h"

#include "mpParser.h"

#include "src/gui/error_message.h"
#include "src/core/exception.h"

namespace qftbx {

Application::Application(int & argc, char ** argv)
    : QApplication(argc, argv)
{
}

bool Application::notify(QObject * receiver, QEvent * event)
{
    try {
        return QApplication::notify(receiver, event);
    } catch (const qftbx::Exception & error) {
        //Through the GUI's own reporter, like every dialog: a modal box for
        //a user, and something a headless run can capture instead of
        //blocking on it forever.
        tools::errorMessage(QString::fromUtf8(error.what()), tr("QFTbx"));
    } catch (const mup::ParserError & error) {
        tools::errorMessage(tr("The expression could not be evaluated: %1")
                            .arg(QString::fromStdString(error.GetMsg())),
                            tr("QFTbx"));
    } catch (const std::exception & error) {
        //Anything else that can still say what happened.
        tools::errorMessage(QString::fromUtf8(error.what()), tr("QFTbx"));
    }

    //The event is spent either way: reporting it is the handling.
    return true;
}

} // namespace qftbx
