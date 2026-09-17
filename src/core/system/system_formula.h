#ifndef QFTBX_SYSTEM_FORMULA_H
#define QFTBX_SYSTEM_FORMULA_H

#include "src/core/math/formula.h"

namespace qftbx {

class LtiSystem;
class Parameter;

/**
 * @brief A system written as the transfer function it is, for a reader.
 *
 * Each family is written the way its own literature writes it: the zeros
 * and poles as factors, the time constants as 1 + s/a, the polynomials by
 * descending powers, the free form as the two expressions the user typed.
 * An uncertain coefficient appears as its NAME - that is the whole point of
 * an uncertain coefficient - and a fixed one as its value at 'digits'
 * significant digits.
 *
 * What LtiSystem::expression() gives as a line of text, given as a shape
 * that can be drawn or turned into LaTeX. The old text is kept for the
 * places that want one line and nothing else.
 */
/**
 * @brief How an uncertain coefficient is written: by the NAME the user gave
 * it, or by the INTERVAL that name stands for.
 *
 * Two readings of the same system, and both are worth having side by side:
 * the first is the plant as it was described, the second is the plant the
 * design actually works over.
 */
enum class ShowUncertain { ByName, ByRange };

Formula formulaOf(LtiSystem & system, int digits, ShowUncertain how = ShowUncertain::ByName);

/// One parameter as it is shown: its name or its interval when uncertain,
/// its nominal value when fixed.
Formula formulaOf(Parameter & parameter, int digits,
                  ShowUncertain how = ShowUncertain::ByName);

/// Whether any coefficient of the system is uncertain, which is whether the
/// two readings above say anything different.
bool hasUncertainty(LtiSystem & system);

} // namespace qftbx

#endif // QFTBX_SYSTEM_FORMULA_H
