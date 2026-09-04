#ifndef QFTBX_EXCEPTION_H
#define QFTBX_EXCEPTION_H

#include <stdexcept>
#include <string>

namespace qftbx {

/**
 * @brief Base class for every error reported by the backend.
 *
 * Backend code must not interact with the user directly: it throws
 * exceptions derived from this class, and the GUI layer catches them at
 * its boundary (the slots) and decides how to present them. Qt aborts if
 * an exception escapes the event loop, so every slot that reaches backend
 * code must catch qftbx::Exception.
 */
class Exception : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

/**
 * @brief Failure opening, reading or writing a project file.
 */
class FileError : public Exception
{
public:
    using Exception::Exception;
};

/**
 * @brief A computation could not produce a result.
 */
class ComputationError : public Exception
{
public:
    using Exception::Exception;
};

/**
 * @brief User-provided data is not valid for the requested operation.
 */
class InvalidInput : public Exception
{
public:
    using Exception::Exception;
};

/**
 * @brief The search stopped because it was asked to, not because anything is
 * wrong.
 *
 * It IS an Exception, because that is how a computation abandons the stack it
 * is forty minutes deep into, and because every catch site in the application
 * already handles the family. But it is not a failure, and whoever catches it
 * should say "cancelled" and not "error": there is no cancel button yet, so it
 * cannot be raised from the interface today, and the button is the moment to
 * tell the two apart on screen.
 */
class Cancelled : public Exception
{
public:
    Cancelled() : Exception("The search was cancelled.") {}
    explicit Cancelled(const std::string & what) : Exception(what) {}
};

/**
 * @brief Malformed content found while parsing a .qft project file.
 */
class ParseError : public FileError
{
public:
    ParseError(const std::string &message, long long line)
        : FileError(message + " (line " + std::to_string(line) + ")"),
          m_line(line)
    {
    }

    /// 1-based line of the project file where parsing failed.
    long long line() const noexcept { return m_line; }

private:
    long long m_line;
};

} // namespace qftbx

#endif // QFTBX_EXCEPTION_H
