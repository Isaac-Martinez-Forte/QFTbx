# Algorithm MC (thesis): every strategy assembled

The definitive algorithm of the doctoral thesis: the NT/NK branch & bound
extended with every strategy of chapter 4, assembled as the pseudocode of chapter
5 prescribes. It contains the 2021 algorithm and adds feasible-subrange cuts on
every parameter, a best-gain search, a bisection guided by the stored cuts, and
execution stages that switch strategies as the search matures.

**Reference.** I. Martínez-Forte, *Aceleración de algoritmos intervalares de
ajuste automático del lazo en QFT*, PhD thesis, University of Murcia, 2022
(directed by J. Cervera): chapter 4 for the strategies, chapter 5 for the
algorithm, chapter 6 for the benchmarks. The PDF is in the
[references](references/) folder. See [references](references/README.md).

## The strategies

- **QSInv** (thesis 5.1.1). The certainly INFEASIBLE subranges of every
  controller parameter are cut away with the closed-form magnitude equations of
  NK's Quick Solution and the phase equations of thesis section 4.1.2, on
  whichever sides of the projected rectangle the boundary certifies as
  forbidden. This is QS2 of the 2021 paper.
- **QSFact** (thesis 5.1.2). The certainly FEASIBLE subranges of every parameter,
  per design frequency, with the same equations evaluated at the opposite corner
  and against the opposite boundary extreme (B_max instead of B_min). The
  subrange feasible at EVERY frequency is split off the box and enters the live
  list as a feasible node; the per-frequency thresholds are stored in the node
  and feed the tree bisection.
- **MG, the best-gain search** (thesis 5.2). Fixing the other parameters at the
  corner that maximises the controller magnitude (zeros at their supremum, poles
  at their infimum) and intersecting the per-frequency feasible gain thresholds
  yields a point solution whose gain can be far lower than the box-certified
  one. It feeds the prune variable C. QSFact runs only when MG finds nothing,
  because they overlap in purpose.
- **Tree bisection** (thesis 5.3). In the intermediate stage the box is not
  bisected in half but split at the stored per-frequency feasible threshold that
  covers the largest fraction of its variable's range. The feasible child is
  marked feasible for that frequency, and the mark, the node's history, skips
  its feasibility test from then on.
- **Execution stages** (thesis 4.4). INITIAL: area bisection, until no projected
  rectangle spans the full phase width. INTERMEDIATE: tree bisection, until a
  full pass of MG, QSFact and QSInv produces nothing. FINAL: cuts disabled,
  bisection by the wider of magnitude and phase.

The strategies of thesis sections 4.5 and 4.6, detection and bounding on the
Nyquist plane, were tried and discarded by the thesis itself and are not
implemented.

## How QFTbx departs from the reference

- MG's certified gain and the feasible nodes must pass the nominal closed-loop
  stability criterion of [loop-shaping.md](loop-shaping.md), and MG's candidate is
  verified against the feasibility test before it may prune: the closed form
  alone relies on strip geometry.
- When the live list empties with a certified MG solution standing, that solution
  is returned. The thesis pseudocode would report "no solution" while holding
  one in C (erratum 1.7 below).

## Errata found in the reference

Found while checking the implementation line by line against the thesis; the
implementation follows the sound reading in each case.

1. **Section 4.1.1.** The three equations of the feasible-subrange bound write
   |B_min| where the text of the same section prescribes B_max ("instead of the
   minimum value of the boundary inside the projected box ... the maximum value,
   called B_max, is used"). The equations should carry B_max; the variables are
   already at the correct, opposite corner.
2. **Chapter 5, algorithm MG.** The final line assigns v ← subs(z, 1, [k_f, k_max]),
   the whole box. The feasibility certificate only holds for the corner y (zeros
   at sup, poles at inf), as section 4.3 itself states ("this solution is only
   valid with a = 1"): the solution that may prune through C is the POINT
   (k_f, zeros sup, poles inf), that is subs(y, 1, [k_f, k_max]).
3. **Chapter 5, algorithm QSInv.** The comment of the variable-fixing loop says
   the corner "maximises" the contribution of x(λ) to ∠L0; the assignments (zero
   to sup, pole to inf) minimise it and are the right ones for the right-side
   cut illustrated. Same erratum as in the 2021 paper.
4. **Chapter 5, general scheme, step C.** "If NL is empty, print 'no feasible
   solution' and exit" is a gap: when MG has certified a solution and that same
   C prunes the rest of the space, the list can empty while a valid solution is
   in hand. The certified solution must be returned; only an empty C warrants
   the message.
5. **Chapter 6, example 2 (QFT Toolbox ex2).** The names of the two tracking
   models are swapped: the model with gain 0.6584 is the UPPER bound T_U (β),
   not the lower one. With the swap the tracking band is empty and the boundary
   cannot be built. Tharewal's example 3.1 and `qftex2.m` of the MATLAB QFT
   Toolbox assign it to the upper bound, and the fixture
   `tests/data/qft_toolbox_ex2.qft` follows them.
6. **Sections 3.1 and 3.2.** The direction of the gain cut and of the pole cut of
   NK's Quick Solution is stated the wrong way round; see
   [loop-shaping-nk.md](loop-shaping-nk.md).

## Where it lives

`src/core/loopshaping/algorithm_mc_thesis.h`, `.cpp`;
`src/core/loopshaping/mc_search_node.h` (the node with its history and
thresholds); the cuts in `quick_solution.h` and `common_functions.h`.

Tests: `tests/backend/mc_thesis_strategies_test.cpp` (the accelerations shrink
the search tree), `tests/backend/loopshaping_golden_test.cpp`,
`tests/backend/thesis_benchmark_test.cpp`.
