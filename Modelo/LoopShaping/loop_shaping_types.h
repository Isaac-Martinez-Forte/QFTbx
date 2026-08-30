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

enum alg_loop_shaping {sachin, nandkishor, rambabu,
                       nandkishor_primeraversion, primer_articulo, segundo_articulo};

} // namespace tools

#endif // QFTBX_LOOP_SHAPING_TYPES_H
