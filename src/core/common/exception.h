#ifndef QFTBX_EXCEPTION_H
#define QFTBX_EXCEPTION_H

#include <stdexcept>
#include <string>

#include "src/core/common/message.h"

/**
 * @file
 * @brief The exceptions of the core, and the one way it talks to the user.
 *
 * The core never shows anything: it throws, and the interface catches at
 * its boundary and shows the message. An exception carries a Message, the
 * text with its arguments kept apart, so the interface can translate it;
 * what() is the English sentence, for the logs, the tests and whoever has
 * no translator. The plain-string constructors remain for texts that are
 * not for the user or come from elsewhere already composed.
 *
 * A ParseError carries the file and the line apart from its message:
 * what() composes the three in English, and an interface that translates
 * innerMessage() composes them again with frame(). A MissingPart is an
 * element or attribute a reader looked for and did not find; a file can be
 * incomplete without being broken, as a project saved half way through a
 * design is, so a reader answers an absence by leaving that part unread
 * and only a reader that cannot go on throws it. Malformed content, such
 * as a number that is not a number, stays a ParseError.
 */
namespace qftbx {

class Exception : public std::runtime_error
{
public:
    explicit Exception(const Message & message)
        : std::runtime_error(message.rendered()), m_message(message) {}
    explicit Exception(const std::string & what)
        : std::runtime_error(what), m_message(Message::plain(what)) {}
    explicit Exception(const char * what)
        : std::runtime_error(what), m_message(Message::plain(what)) {}

    const Message & message() const noexcept { return m_message; }

private:
    Message m_message;
};

class FileError : public Exception
{
public:
    using Exception::Exception;
};

class ComputationError : public Exception
{
public:
    using Exception::Exception;
};

class InvalidInput : public Exception
{
public:
    using Exception::Exception;
};

class Cancelled : public Exception
{
public:
    Cancelled() : Exception(QFTBX_TR("Core", "The search was cancelled.")) {}
    using Exception::Exception;
};

class ParseError : public FileError
{
public:
    ParseError(const Message & message, long long line, const std::string & file = std::string())
        : FileError(frame(file, message.rendered(), line)), m_inner(message), m_file(file), m_line(line) {}
    ParseError(const std::string & message, long long line, const std::string & file = std::string())
        : ParseError(Message::plain(message), line, file) {}

    long long line() const noexcept { return m_line; }
    const std::string & file() const noexcept { return m_file; }

    const Message & innerMessage() const noexcept { return m_inner; }

    static Message frame(const std::string & file, const std::string & renderedMessage, long long line)
    {
        if (file.empty()) {
            return QFTBX_TR("Core", "%1 (line %2)").arg(renderedMessage).arg(line);
        }
        return QFTBX_TR("Core", "%1: %2 (line %3)").arg(file).arg(renderedMessage).arg(line);
    }

private:
    Message m_inner;
    std::string m_file;
    long long m_line;
};

class MissingPart : public ParseError
{
public:
    using ParseError::ParseError;
};

}

#endif
