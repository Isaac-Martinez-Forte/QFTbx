#ifndef QFTBX_TEXT_TOKENS_H
#define QFTBX_TEXT_TOKENS_H

#include <optional>

#include <QString>
#include <QVector>

namespace qftbx {
namespace text {

/// Splits a string into its whitespace-separated tokens.
QVector<QString> tokens(const QString & line);

/// Parses whitespace-separated reals, or nothing when any token is not a
/// valid real, so a malformed frequency file or coefficient list is
/// rejected as a whole rather than silently truncated.
std::optional<QVector<qreal>> reals(const QString & line);

} // namespace text
} // namespace qftbx

#endif // QFTBX_TEXT_TOKENS_H
