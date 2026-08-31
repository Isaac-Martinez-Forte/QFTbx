#ifndef QFTBX_LOOP_SHAPING_TYPES_H
#define QFTBX_LOOP_SHAPING_TYPES_H

//Transitional home for the loop-shaping enums, moved out of tools.h.
//They will be modernised with the loop-shaping stage (thesis scope).
namespace tools {

enum flags_box{
    feasible,
    infeasible,
    ambiguous,
    completo
};

enum alg_loop_shaping {nt, nk, mr,
                       mc1, mc_thesis};

} // namespace tools

#endif // QFTBX_LOOP_SHAPING_TYPES_H
