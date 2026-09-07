#include "src/gui/application/error_message.h"

#include <QCoreApplication>
#include <QMessageBox>

#include "src/core/common/exception.h"

namespace qftbx {

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

QString translated(const Message & message)
{
    QString text = message.context().empty()
            ? QString::fromStdString(message.text())
            : QCoreApplication::translate(message.context().c_str(), message.text().c_str());
    for (const std::string & argument : message.arguments()) {
        text = text.arg(QString::fromStdString(argument));
    }
    return text;
}

QString translated(const std::exception & failure)
{
    if (const auto * parse = dynamic_cast<const ParseError *>(&failure)) {
        const QString inner = translated(parse->innerMessage());
        return translated(ParseError::frame(parse->file(), inner.toStdString(), parse->line()));
    }
    if (const auto * exception = dynamic_cast<const Exception *>(&failure)) {
        return translated(exception->message());
    }
    return QString::fromUtf8(failure.what());
}

} // namespace qftbx
