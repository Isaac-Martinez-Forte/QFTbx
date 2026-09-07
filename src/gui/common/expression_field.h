#ifndef QFTBX_GUI_EXPRESSION_FIELD_H
#define QFTBX_GUI_EXPRESSION_FIELD_H

#include <optional>
#include <stdexcept>
#include <vector>

#include <QString>

#include "src/core/math/expression_tree.h"

/**
 * @brief The number a field of a dialog holds, evaluated as an expression.
 *
 * The dialogs accept an expression wherever they ask for a number, so
 * "2*pi" or "1e3/4" are as good as "6.2831853" or "250". Empty when the
 * text is not an expression the grammar reads, or names a variable: a
 * field has none to give. A value that parses but is not finite ("1/0",
 * "0/0") comes back as it is; the caller decides whether infinity or NaN
 * are acceptable where it stands.
 */
namespace qftbx {

inline std::optional<double> evaluateNumber(const QString & text)
{
    try {
        ExpressionTree expression(text.toStdString());
        expression.bind(std::vector<std::string>());
        return expression.evaluate(std::vector<double>());
    } catch (const std::invalid_argument &) {
        return std::nullopt;
    }
}

} // namespace qftbx

#endif // QFTBX_GUI_EXPRESSION_FIELD_H
