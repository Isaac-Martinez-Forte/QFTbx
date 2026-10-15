/**
 * @file
 * @brief Event dispatch that turns an escaped exception into a message.
 *
 * The exception is reported through the GUI's own error reporter, so a
 * headless run captures it instead of blocking on a dialog. A toolbox
 * exception is translated first; any other standard exception is shown
 * with the text it carries. The event counts as handled either way, since
 * reporting it is the handling.
 */

#include "src/gui/application/application.h"

#include <QFileOpenEvent>

#include "src/gui/application/error_message.h"
#include "src/core/common/exception.h"

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
        qftbx::errorMessage(translated(error), tr("QFTbx"));
    } catch (const std::exception & error) {
        qftbx::errorMessage(translated(error), tr("QFTbx"));
    }

    return true;
}

QString Application::takeAskedFile()
{
    QString asked;
    asked.swap(m_askedFile);
    return asked;
}

bool Application::event(QEvent * event)
{
    if (event->type() == QEvent::FileOpen) {
        m_askedFile = static_cast<QFileOpenEvent *>(event)->file();
        emit fileOpenAsked(m_askedFile);
        return true;
    }
    return QApplication::event(event);
}

}
