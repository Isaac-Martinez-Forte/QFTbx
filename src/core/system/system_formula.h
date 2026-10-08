/**
 * @file
 * @brief A system written as the transfer function it is, for a reader.
 *
 * Declares the functions that turn a system, or one parameter, into a
 * formula that can be drawn or turned into LaTeX, the shape of what
 * LtiSystem::expression() gives as one line of text. Each family is written
 * as its own literature writes it: zeros and poles as factors, time
 * constants as 1 + s/a, polynomials by descending powers, the free form as
 * the two expressions the user typed. An uncertain coefficient is shown by
 * its name, the plant as it was described, or by the interval it stands
 * for, the plant the design works over; a fixed one by its nominal value
 * at the requested significant digits. hasUncertainty() says whether the
 * two readings differ at all.
 */

#ifndef QFTBX_SYSTEM_FORMULA_H
#define QFTBX_SYSTEM_FORMULA_H

#include "src/core/math/formula.h"

namespace qftbx {

class LtiSystem;
class Parameter;

enum class ShowUncertain { ByName, ByRange };

Formula formulaOf(LtiSystem & system, int digits, ShowUncertain how = ShowUncertain::ByName);

Formula formulaOf(Parameter & parameter, int digits,
                  ShowUncertain how = ShowUncertain::ByName);

bool hasUncertainty(LtiSystem & system);

}

#endif
