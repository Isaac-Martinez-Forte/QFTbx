#ifndef QFTBX_MESSAGE_H
#define QFTBX_MESSAGE_H

#include <cstddef>
#include <string>
#include <vector>

/**
 * @brief A text meant for the user, kept apart from its arguments so that
 * it can be translated where the user is.
 *
 * The core speaks English and knows no Qt, but what it says in an
 * exception ends up in a dialog. A Message carries the English text with
 * placeholders (%1, %2, ...) and the arguments separately: rendered()
 * gives the English sentence, and the interface translates the text and
 * substitutes the same arguments, with the same tools and the same
 * translation file as its own strings. The text is written where the
 * message is thrown, wrapped in QFTBX_TR so that lupdate finds it.
 */
namespace qftbx {

class Message
{
public:
    Message() = default;
    Message(std::string context, std::string text);

    const std::string & context() const { return m_context; }
    const std::string & text() const { return m_text; }
    const std::vector<std::string> & arguments() const { return m_arguments; }

    /// Adds the next argument: it replaces the lowest-numbered placeholder
    /// still in the text, as QString::arg() does.
    Message & arg(const std::string & value);
    Message & arg(const char * value);
    Message & arg(double value);
    Message & arg(int value);
    Message & arg(long value);
    Message & arg(long long value);
    Message & arg(unsigned long value);
    Message & arg(unsigned long long value);

    /// The English sentence: the text with its arguments in place.
    std::string rendered() const;

    /// A text with no translation: what an exception built from a plain
    /// string carries.
    static Message plain(std::string text);

private:
    std::string m_context;
    std::string m_text;
    std::vector<std::string> m_arguments;
};

/// Marks a text for translation: the same call lupdate reads as
/// QT_TRANSLATE_NOOP. The context groups the core's messages in the
/// translation file; it is "Core" everywhere.
#define QFTBX_TR(context, text) ::qftbx::Message(context, text)

} // namespace qftbx

#endif // QFTBX_MESSAGE_H
