# Algorithm NT: Nataraj and Tharewal

The base algorithm of the family, and the one the other four extend: QFT loop
shaping as an interval branch & bound over the controller parameter box. It reads
as a proof rather than a search, and everything the later algorithms add is a way
of shrinking the tree it explores.

**Reference.** S. Tharewal, *Automated synthesis of QFT controllers and prefilters
using interval global optimization techniques*, PhD thesis, IIT Bombay, 2005:
chapter 3 for the algorithm and chapter 5 for the constraint-propagation
acceleration. The section numbers below are its. The algorithm was first
published by Nataraj and Tharewal in 2002–2004; the thesis is the complete
account. QFTbx thesis (2022), section 3.1, describes it in Spanish.
See [references](references/README.md).

## The algorithm

Over the machinery of [loop-shaping.md](loop-shaping.md):

1. Classify the initial box. Certainly infeasible: no solution, stop.
2. Put it in the live list, ordered by the infimum of the gain.
3. Take the head of the list. If it is certainly feasible, its lowest-gain corner
   is the global optimum: stop. If it is narrower than epsilon at every
   frequency (Remark 3.1), extract its feasible corner and stop.
4. Bisect the head along its widest parameter (section 3.3.3, steps 1–7).
5. Classify both halves; discard the certainly infeasible ones.
6. Cut the gain of the survivors (below).
7. Insert them in the list and go back to 3.

Because the list is ordered by the lowest gain a box can reach, the first
certainly feasible head is the global minimum: no box left in the list can do
better. That is the optimality argument, and it is the reason the list order is
sacred in every algorithm of the family.

## The gain cuts (chapter 5)

The loop magnitude is monotonic in the gain, so when a box is ambiguous the
boundary extremes over its phase span certify subranges of the gain without
bisecting them:

- **C_g−** removes the low-gain subrange whose rectangle lies entirely below the
  minimum boundary magnitude over the box's phase interval, where the forbidden
  side is below.
- **C_g+** splits off the high-gain subrange lying entirely above the maximum
  boundary magnitude, where that side is allowed: it is a certainly feasible box
  of its own and enters the list as such.

Both are certified by the parity classification of the corresponding corner, so
neither relies on a heuristic for correctness.

## How QFTbx departs from the reference

- **Nominal stability** is checked with the Nichols-chart Nyquist criterion, as
  described in [loop-shaping.md](loop-shaping.md), rather than by the zeros of
  1 + L0. Without it an ACC'90-style marginally unstable plant drives the search
  to the bottom of the gain box, which the boundaries alone do not exclude.
- **The C_g+ split** is re-certified by the same box test it came from, so its
  gate cannot compromise the result; degenerate slivers are skipped because they
  would only bloat the list.
- **A penalty outside the paper is gone.** The historical implementation added
  100 to the list key of a box whose phase supremum at ω = 2 rad/s was below
  -180°. It was a hand-made stability rule and it broke the optimality
  guarantee; the nominal stability check replaces it.

## Where it lives

`src/core/loopshaping/algorithm_nt.h`, `.cpp`. The cuts live in
`src/core/loopshaping/common_functions.h`, shared with the other algorithms.

Tests: `tests/backend/literature_validation_test.cpp` (the minimal feasible gain
of the benchmark matches Tharewal's), `tests/backend/loopshaping_golden_test.cpp`.
