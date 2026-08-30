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
