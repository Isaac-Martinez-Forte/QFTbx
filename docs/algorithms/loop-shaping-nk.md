# Algorithm NK: Nataraj and Kubal

NT with two additions: closed-form cuts on every controller parameter, not only
the gain, and a local optimisation whose feasible results prune the tree. It is
the algorithm the QFTbx accelerations are measured against.

**Reference.** S. V. Paluri (Nataraj) and N. Kubal, *Automatic loop shaping in QFT
using hybrid optimization and constraint propagation techniques*, International
Journal of Robust and Nonlinear Control 17:251–264, 2007, DOI 10.1002/rnc.1085.
The paper is essentially Kubal's master's thesis. QFTbx thesis (2022), section
3.2, describes it in Spanish. See [references](references/README.md).

## Quick Solution (paper section 3.3)

For a box straddling a bound whose forbidden side is below, the loop magnitude is
monotonic in every parameter: it grows with the gain and with each zero and
shrinks with each pole. Fixing all parameters but one at the corner that
maximises |L0| (gain and zeros at their supremum, poles at their infimum) and
solving |L0| = |B|_min for the remaining one gives the point where its range stops
being certainly infeasible:

    k'   = |B|_min · |D(jω, p_inf)| / ( |N(jω, z_sup)| · |p0| )                      cut k to [k', sup k]
    z_i' = sqrt( ( |B|_min · |D| / ( k_sup · |N_-i| · |p0| ) )² − ω² )                cut z_i to [z_i', sup z_i]
    p_j' = sqrt( ( k_sup · |N| / ( |B|_min · |D_-j| · |p0| ) )² − ω² )                cut p_j to [inf p_j, p_j']

where N and D are the numerator and denominator products, N_-i the numerator
without the i-th zero, and D_-j the denominator without the j-th pole. The cuts
are applied per design frequency with the latest updated values before a box
enters the live list. A negative or non-real solution means no cut at that
frequency.

The paper's worked example (a DC motor, section 3.3.1) is reproduced in
`tests/backend/quick_solution_test.cpp`: the gain cut from [0.1, 2010597.38] to
[439429.20, 2010597.38] and the pole cut from [1025.5, 4834.5] to
[1025.5, 4692.15] come out as printed.

## Local optimisation (paper section 3.2)

A local search is launched from the leading box whenever its gain infimum
differs by more than 10 % from every previous launch point. A feasible local
solution prunes every node whose gain infimum cannot beat it, clips the gain
range of new boxes, and stands in as the answer if the list ever empties. The
paper leaves the local method unspecified ("call any nonlinear constrained local
optimization routine"), so this part is QFTbx's own: a logarithmic bisection of
the gain followed by a Hooke–Jeeves pattern search in log space, bounded by
`algorithms.local-search-budget`.

## How QFTbx departs from the reference

- The feasibility test is completed with the nominal closed-loop stability
  check, which the paper's problem formulation demands (zeros of 1 + L0) and
  QFTbx performs on the Nichols chart; see [loop-shaping.md](loop-shaping.md).
- The paper's step 22 terminates on a parameter r it never defines; QFTbx uses
  the termination of NT (an epsilon-small leading box), common to the family.
- The historical implementation mixed decibels into the quotients of the Quick
  Solution equations and subtracted the logarithm of ω², producing dimensionless
  noise. All quantities are linear magnitudes now, and the local optimisation,
  which had been commented out of the main loop, runs.

## Errata found in the references

- QFTbx thesis, sections 3.1 and 3.2: the text gives the surviving interval of
  the gain cut and of the pole cut the wrong way round. The cut point k' marks
  where even the |L0|-maximising corner falls below B_min, so the LOW part of the
  gain range is what goes ([k', sup k] survives); a larger pole lowers the loop
  towards the forbidden side, so the pole loses its UPPER end ([inf p, p']
  survives). The paper's worked example settles both.

## Where it lives

`src/core/loopshaping/algorithm_nk.h`, `.cpp`; the cutting equations in
`src/core/loopshaping/quick_solution.h`.

Tests: `tests/backend/quick_solution_test.cpp`,
`tests/backend/literature_validation_test.cpp`,
`tests/backend/loopshaping_golden_test.cpp`.
