/**
 * @file
 * @brief Error reporting with a replaceable destination, and the translation of core messages.
 *
 * The current reporter is a function object kept in a static; when none is
 * set, the message opens a parentless critical box. A core message is
 * looked up under its own context and its arguments are substituted after
 * translation. A parse error is translated inside out: its inner message
 * first, then the frame that puts the file and line around it.
 */

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

}

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

}
