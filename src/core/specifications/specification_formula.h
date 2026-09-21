/**
 * @file
 * @brief The requirement each specification states, as a drawable formula.
 *
 * Declares the functions giving, for each of the seven specification slots,
 * the closed-loop magnitude the literature bounds and the sign of the bound:
 * the six restrictions of QFT as equations (1.6) to (1.11) of the thesis
 * write them, with L = PC the loop of a plant of the family. Tracking
 * carries the prefilter F as the literature does, even though F cannot
 * change the spread of the closed loop over the family, which is what the
 * loop shaping fits between the two tracking bounds; the prefilter design
 * itself is left for later.
 */

#ifndef QFTBX_SPECIFICATION_FORMULA_H
#define QFTBX_SPECIFICATION_FORMULA_H

#include "src/core/math/formula.h"
#include "src/core/specifications/specification.h"

namespace qftbx {

/**
 * @brief What each specification asks of the closed loop, written as the
 * literature writes it.
 *
 * The six restrictions of QFT, with \f$ L = PC \f$ the loop of a plant of
 * the family, exactly as equations (1.6) to (1.11) of the thesis state them
 * (I. Martinez Forte, 2022; the same list is in Houpis, Rasmussen and
 * Garcia-Sanz, and in Horowitz before them):
 *
 * - tracking, \f$ \alpha \leq |F L/(1+L)| \leq \beta \f$, one slot per side;
 * - robust stability, \f$ |L/(1+L)| \leq \lambda \f$ (the M circle);
 * - sensor noise, \f$ |L/(1+L)| \leq \delta_n \f$, the same magnitude with
 *   another bound, because noise at the sensor reaches the output through
 *   the closed loop;
 * - output disturbance, \f$ |1/(1+L)| \leq \delta_{po} \f$;
 * - input disturbance, \f$ |P/(1+L)| \leq \delta_{pi} \f$;
 * - control effort, \f$ |C/(1+L)| \leq \delta_{ce} \f$.
 *
 * These are also the magnitudes the program computes
 * (closed_loop_worst_case.h), with one difference that is the prefilter's:
 * F multiplies the whole closed loop at each frequency, so it cannot change
 * the SPREAD of it over the plant family, and the spread is what the loop
 * shaping has to fit between the two tracking bounds. QFTbx bounds that
 * spread and leaves F to a later design. See
 * docs/SPECIFICATIONS.md.
 */
Formula requirementOf(SpecificationType type);

/// The same, with the bound the user gave on the other side of the sign.
Formula requirementOf(SpecificationType type, Formula bound);

}

#endif
