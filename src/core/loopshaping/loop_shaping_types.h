#ifndef QFTBX_LOOP_SHAPING_TYPES_H
#define QFTBX_LOOP_SHAPING_TYPES_H

//The loop-shaping enums shared by the algorithms, the dialog and the file.
namespace qftbx {

/// Verdict on a parameter box: proved feasible, proved infeasible, or
/// neither (the only one worth bisecting).
enum BoxFlag{
    feasible,
    infeasible,
    ambiguous
};

/// Which of the five algorithms to run. Positional: the persistence and
/// the dialog both index them in this order.
enum LoopShapingAlgorithm {nt, nk, mr,
                       mc1, mc_thesis};

} // namespace qftbx

#endif // QFTBX_LOOP_SHAPING_TYPES_H
