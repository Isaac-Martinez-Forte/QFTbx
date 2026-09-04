# Algorithm MR: synthesis as an interval constraint satisfaction problem

The other branch of the Nataraj family. Instead of classifying boxes against
Nichols boundaries, MR writes the specifications as inequalities in the
controller parameters and solves the resulting interval constraint satisfaction
problem (ICSP) by branch & prune, narrowing each box with a consistency filter
before bisecting it. It needs no boundaries at all: the constraints come straight
from the specifications and the templates.

**Reference.** R. Kalla and P. S. V. Nataraj, *Synthesis of fractional-order QFT
controllers using interval constraint satisfaction technique*, 4th IFAC Workshop
on Fractional Differentiation and its Applications (FDA'10), Badajoz, 2010. The
line begins with Nataraj and Deshpande's ICSP formulation for fixed-structure
controllers (IFAC World Congress, Seoul, 2008), and this paper is the one with
the HC4 filter; QFTbx implements it for integer-order controllers, where the
mechanics are the same. See [references](references/README.md).

## The constraints

Write the controller value at ω as g·e^{jφ} and one plant of the template as
p·e^{jθ}. Each specification becomes, for every template representative and
design frequency, a quadratic inequality in g and φ (the paper's equations
(9)–(11)), for example:

    robust stability margin λ:      g²p²(1 − 1/λ²) + 2gp·cos(φ+θ) + 1 ≥ 0
    output disturbance bound δ:     g²p² + 2gp·cos(φ+θ) + (1 − 1/δ²) ≥ 0

and the tracking spread yields one inequality per ordered pair of
representatives. Since g and φ are themselves functions of the controller
parameters, every inequality is an expression tree over the box
(**k**, **z_i**, **p_j**).

## Branch & prune with HC4

At each node every constraint is applied to the box with the HC4 hull-consistency
filter: a forward evaluation of the expression tree in interval arithmetic,
then a backward pass that projects the constraint's allowed range onto each
variable, narrowing its domain. The filters are iterated to a fixpoint, bounded
by `algorithms.max-narrowing-passes`. An emptied domain proves the box holds no
solution and discards it; otherwise the narrowed box is bisected along its widest
variable and both halves join the list.

## How QFTbx departs from the reference

- **Termination.** The paper collects every solution box of the target width and
  sorts them afterwards. QFTbx orders the live list by the gain infimum and stops
  at the first certainly feasible box (every constraint non-negative over the
  box) or at the epsilon-small head, which is the box the sort would have
  picked. Epsilon here measures the width of the CONTROLLER PARAMETER box, as
  the paper does, not the Nichols rectangle of the other four algorithms.
- **Representatives.** The template contour is subsampled to a handful of
  representatives per frequency (`algorithms.template-representatives`; the
  paper uses nine plants), because the tracking constraints square in their
  number.
- **Equation (9)**, plain robust stability, is |1 + L|² ≥ 0: a square bounded
  below by zero. It holds always and never contracts anything, so it is
  omitted.
- The returned controller must pass the nominal closed-loop stability
  criterion of [loop-shaping.md](loop-shaping.md).

## What was fixed in the port

The historical implementation of MR was unfinished: the frequency masks of the
stability and disturbance rules could never fire, the units of the template
representative were left as a TODO, and the algorithm had never been checked
against the paper. The current implementation was rewritten against the paper
and validated on its example: `tests/backend/mr_article_validation_test.cpp`
checks that the controller published in the paper meets the paper's
specifications and that MR designs a feasible controller under the stability
margin.

## The expression tree

The HC4 filter works on the toolbox's expression tree: constants, π and e,
variables, the four operations and the power, with the usual elementary
functions, compared with one of ≥, >, ≤, <, =. The constraints are built in
memory with the `Expression` builder, not written as text and parsed; the
propagation honours the comparison by intersecting the root with its allowed
interval before the backward pass. The tree, which also evaluates the free-form
plants and every number a dialog reads, is the historical work of Roberto C.
Cruz Rodríguez, reviewed and completed during the port.

## Where it lives

`src/core/loopshaping/algorithm_mr.h`, `.cpp`;
`src/core/math/expression_tree.h`, `.cpp`.

Tests: `tests/backend/mr_article_validation_test.cpp`,
`tests/backend/loopshaping_structures_test.cpp` (the expression tree).
