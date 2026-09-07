#include "src/core/common/message.h"

#include <utility>

#include "src/core/common/text_tokens.h"

namespace qftbx {

Message::Message(std::string context, std::string text)
    : m_context(std::move(context)), m_text(std::move(text))
{
}

Message Message::plain(std::string text)
{
    return Message(std::string(), std::move(text));
}

Message & Message::arg(const std::string & value)
{
    m_arguments.push_back(value);
    return *this;
}

Message & Message::arg(const char * value)
{
    m_arguments.emplace_back(value);
    return *this;
}

Message & Message::arg(double value)
{
    return arg(text::number(value));
}

Message & Message::arg(int value)
{
    return arg(std::to_string(value));
}

Message & Message::arg(long value)
{
    return arg(std::to_string(value));
}

Message & Message::arg(long long value)
{
    return arg(std::to_string(value));
}

Message & Message::arg(unsigned long value)
{
    return arg(std::to_string(value));
}

Message & Message::arg(unsigned long long value)
{
    return arg(std::to_string(value));
}

namespace {

//The next occurrence of "%n" that is a whole placeholder: "%1" is not
//the "%1" of "%10".
std::size_t findPlaceholder(const std::string & text, const std::string & placeholder, std::size_t from)
{
    std::size_t at = from;
    while ((at = text.find(placeholder, at)) != std::string::npos) {
        const std::size_t after = at + placeholder.size();
        if (after < text.size() && text[after] >= '0' && text[after] <= '9') {
            at = after;
            continue;
        }
        return at;
    }
    return std::string::npos;
}

} // namespace

std::string Message::rendered() const
{
    //Each argument replaces every occurrence of the lowest-numbered
    //placeholder left, which is what QString::arg() does one call at a time.
    std::string out = m_text;
    for (const std::string & value : m_arguments) {
        std::string placeholder;
        for (int n = 1; n <= 99 && placeholder.empty(); ++n) {
            const std::string candidate = "%" + std::to_string(n);
            if (findPlaceholder(out, candidate, 0) != std::string::npos) {
                placeholder = candidate;
            }
        }
        if (placeholder.empty()) {
            break;
        }
        std::size_t at = 0;
        while ((at = findPlaceholder(out, placeholder, at)) != std::string::npos) {
            out.replace(at, placeholder.size(), value);
            at += value.size();
        }
    }
    return out;
}

} // namespace qftbx
