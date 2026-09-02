#ifndef QFTBX_LOOP_SHAPING_TYPES_H
#define QFTBX_LOOP_SHAPING_TYPES_H

//Transitional home for the loop-shaping enums, moved out of tools.h.
//They will be modernised with the loop-shaping stage (thesis scope).
namespace tools {

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

} // namespace tools

#endif // QFTBX_LOOP_SHAPING_TYPES_H
