#include "src/gui/error_message.h"

#include <QMessageBox>

namespace tools {

namespace {

ErrorReporter & reporter()
{
    static ErrorReporter current;
    return current;
}

} // namespace

void errorMessage(QString message, QString title)
{
    if (reporter()) {
        reporter()(message, title);
        return;
    }

    QMessageBox::critical(nullptr, title, message, QMessageBox::Close);
}

ErrorReporter setErrorReporter(ErrorReporter newReporter)
{
    ErrorReporter previous = reporter();
    reporter() = std::move(newReporter);
    return previous;
}

} // namespace tools
