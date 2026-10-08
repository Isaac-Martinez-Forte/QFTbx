/**
 * @file
 * @brief The requirement each specification states, as a drawable formula.
 *
 * Declares the functions giving, for each of the seven specification slots,
 * the closed-loop magnitude the literature bounds and the sign of the
 * bound, alone or with the bound the user gave on its other side: the six
 * restrictions of QFT as equations (1.6) to (1.11) of the thesis write
 * them (I. Martinez Forte, 2022; the same list is in Houpis, Rasmussen and
 * Garcia-Sanz, and in Horowitz), with \f$ L = PC \f$ the loop of a plant
 * of the family:
 *
 * - tracking, \f$ \alpha \leq |F L/(1+L)| \leq \beta \f$, one slot per side;
 * - robust stability, \f$ |L/(1+L)| \leq \lambda \f$ (the M circle);
 * - sensor noise, \f$ |L/(1+L)| \leq \delta_n \f$;
 * - output disturbance, \f$ |1/(1+L)| \leq \delta_{po} \f$;
 * - input disturbance, \f$ |P/(1+L)| \leq \delta_{pi} \f$;
 * - control effort, \f$ |C/(1+L)| \leq \delta_{ce} \f$.
 *
 * These are the magnitudes closed_loop_worst_case.h computes, save that
 * tracking carries the prefilter F as the literature does, although F
 * cannot change the spread of the closed loop over the family, which is
 * what the loop shaping fits between the two tracking bounds; the
 * prefilter design itself is left for later (docs/SPECIFICATIONS.md).
 */

#ifndef QFTBX_SPECIFICATION_FORMULA_H
#define QFTBX_SPECIFICATION_FORMULA_H

#include "src/core/math/formula.h"
#include "src/core/specifications/specification.h"

namespace qftbx {

Formula requirementOf(SpecificationType type);

Formula requirementOf(SpecificationType type, Formula bound);

}

#endif
