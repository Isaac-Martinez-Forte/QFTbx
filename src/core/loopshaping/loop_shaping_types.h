/**
 * @file
 * @brief The enumerations the loop-shaping algorithms, the dialog and the file share.
 *
 * The verdict on a parameter box, and which algorithm to run. The algorithm
 * list is positional: the project file and the dialog index it in this
 * order, so a new algorithm goes at the end.
 */

#ifndef QFTBX_LOOP_SHAPING_TYPES_H
#define QFTBX_LOOP_SHAPING_TYPES_H

namespace qftbx {

/// Verdict on a parameter box: proved feasible, proved infeasible, or
/// neither (the only one worth bisecting).
enum BoxFlag{
    feasible,
    infeasible,
    ambiguous
};

/// Which of the six algorithms to run. Positional: the persistence and
/// the dialog both index them in this order, so a new one goes at the end
/// or the projects already written change meaning.
enum LoopShapingAlgorithm {nt, nk, mr,
                       mc1, mc_thesis, mc2, mc3};

}

#endif
