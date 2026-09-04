# Algorithm MC (2021): the accelerated NT/NK branch & bound

The algorithm of the 2021 paper: NT/NK with two new sources of information for
cutting the parameter box, *phase* information and *feasible-box* information,
assembled into the QS2 reduction. On the two benchmarks of the paper it speeds
NK up by one to two orders of magnitude while keeping its guarantee.

**Reference.** I. Martínez-Forte and J. Cervera, *Accelerated quantitative
feedback theory interval automatic loop shaping algorithm*, International Journal
of Robust and Nonlinear Control 31:4378–4396, 2021, DOI 10.1002/rnc.5499. The
accepted manuscript is in open access in DIGITUM
(http://hdl.handle.net/10201/123363). The doctoral thesis (2022) contains the
same material in chapters 4 and 5, and extends it (see
[loop-shaping-mc.md](loop-shaping-mc.md)).
See [references](references/README.md).

## QS2: the three stages

NK's Quick Solution cuts a parameter where the box's rectangle straddles a bound
from the magnitude side. QS2 adds two stages:

1. **Magnitude information**, which is NK's Quick Solution as it stands.
2. **Phase information.** When a vertical strip of the box's Nichols rectangle
   is certainly forbidden, the phase of L0 is monotonic in every zero and pole
   (a zero contributes atan(ω/z), decreasing in z; a pole the opposite), so the
   same closed-form reasoning cuts the certainly infeasible subrange of each zero
   and pole from the phase side: the horizontal counterpart of stage 1. The
   equations are in `quick_solution.h`, with their derivation.
3. **Feasible-box information.** Over the corner that maximises the controller
   magnitude, the search finds the largest upper subrange [k_f, sup k] of the
   gain whose box is certainly feasible at EVERY design frequency. That subrange
   is a certified solution with gain k_f. Its infimum feeds the prune variable C
   of the paper's step 3bis: boxes whose gain infimum cannot improve C are
   discarded, and every new box's gain range is capped at C.

## How QFTbx departs from the reference

- The paper inserts the feasible box z' into the live list as a triple and
  splits the remainder u = z − z'. QFTbx realises z' as the certified controller
  behind C: the capped boxes ARE u, and the certified point is returned when the
  search exhausts the list without finding anything better. Same prune, same
  fallback, no duplicate list entries.
- Stage 3 finds k_f by logarithmic bisection over the feasibility test; the paper
  leaves the search method unspecified. For closed boundaries feasibility is not
  monotonic in k_f, so the bisection may miss a certificate. It never accepts a
  false one.
- The returned point must pass the nominal closed-loop stability criterion of
  [loop-shaping.md](loop-shaping.md), as in NT and NK.

## Errata found in the reference

- Algorithm 4 (QS2), stage 2, the comment of the inner loop says that y receives
  the value of z[λ] that *maximises* the contribution of x(λ) to ∠L0. The
  assignments of the pseudocode (zero to its supremum, pole to its infimum)
  minimise that contribution, and they are the correct ones for the right-side
  cut the figure illustrates; the comment is the erratum.

## Where it lives

`src/core/loopshaping/algorithm_mc1.h`, `.cpp`; the cuts in
`src/core/loopshaping/quick_solution.h` and `common_functions.h`.

Tests: `tests/backend/quick_solution_test.cpp` (the phase cuts are sound and
tight on both strips), `tests/backend/loopshaping_golden_test.cpp`,
`tests/backend/literature_validation_test.cpp`.
