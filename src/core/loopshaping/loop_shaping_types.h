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

/// Which of the six algorithms to run. Positional: the persistence and
/// the dialog both index them in this order, so a new one goes at the end
/// or the projects already written change meaning.
enum LoopShapingAlgorithm {nt, nk, mr,
                       mc1, mc_thesis, mc2};

} // namespace qftbx

#endif // QFTBX_LOOP_SHAPING_TYPES_H
