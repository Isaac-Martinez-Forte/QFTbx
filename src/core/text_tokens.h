#ifndef QFTBX_TEXT_TOKENS_H
#define QFTBX_TEXT_TOKENS_H

#include <QString>
#include <QVector>

namespace qftbx {
namespace text {

/// Splits a string into its whitespace-separated tokens. The caller owns
/// the returned vector.
QVector<QString> * tokens(const QString & line);

/// Parses whitespace-separated reals. Returns nullptr when any token is
/// not a valid real, so a malformed frequency file or coefficient list is
/// rejected as a whole rather than silently truncated. The caller owns the
/// returned vector.
QVector<qreal> * reals(const QString & line);

} // namespace text
} // namespace qftbx

#endif // QFTBX_TEXT_TOKENS_H
