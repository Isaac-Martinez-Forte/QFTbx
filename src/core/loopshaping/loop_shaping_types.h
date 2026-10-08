/**
 * @file
 * @brief The enumerations the loop-shaping algorithms, the dialog and the
 * file share.
 *
 * The verdict on a parameter box, and which algorithm to run. A box is
 * proved feasible, proved infeasible, or ambiguous, the only verdict worth
 * bisecting. The project file names the algorithm (algorithm_name.h).
 */

#ifndef QFTBX_LOOP_SHAPING_TYPES_H
#define QFTBX_LOOP_SHAPING_TYPES_H

namespace qftbx {

enum BoxFlag{
    feasible,
    infeasible,
    ambiguous
};

enum LoopShapingAlgorithm {nt, nk, mr,
                       mc1, mc_thesis, mc2, mc3};

}

#endif
