# Algorithm MC2: the thesis strategies with their formulation corrected

MC2 is what the strategies of the doctoral thesis do once their published
equations are made to say what their text says. It is not a new method: it is MC
(thesis) with four corrections to the cutting rules, an exact best gain, a
contraction of the box it is about to return, and the execution stages taken out
in favour of the bisection rule that was hiding inside them.

**Reference.** I. Martínez-Forte, *Aceleración de algoritmos intervalares de
diseño automático de controladores QFT*, doctoral thesis, Universidad de Murcia,
2022, chapters 4 and 5. The page of [MC (thesis)](loop-shaping-mc.md) describes
the strategies as the thesis formulates them; this page is about what they turn
into.

## The errata of the formulation, and what they cost

Four of them, each found by reading the equations against their own text and
each confirmed by a test that fails with the published form:

- **The magnitude cut compares against the wrong extreme.** The equations of
  section 4.1.1 compare against B_min where the text prescribes B_max. That does
  not make the cut conservative, it makes it wrong: it can remove a subrange that
  holds feasible controllers.
- **QSInv cuts the phase at the wrong vertex.** The pseudocode fixes the cut at
  the vertex that minimises the phase where the right-side cut needs the one that
  maximises it.
- **QSFact asks one vertex to do two things.** It asks a single vertex to
  minimise magnitude and phase at once, which no vertex of a box does in general.
- **The best-gain search compares against the wrong set.** It compares against a
  boundary extreme over the whole phase span of the box instead of the allowed
  set at the phase of the point it certifies, so it can return a gain the
  boundary at that phase forbids.

## What MC2 adds

- **The exact best gain of a vertex.** The gain that a point controller may take
  is read off the boundary columns at the point's own phase, as a union of
  intervals, and the best one is the least member of it. No bisection, no
  tolerance.
- **The terminal box is contracted.** When a box is small enough to return, its
  gain interval is contracted to what the columns allow before the corner is
  taken, instead of the corner inheriting the width of the box.
- **No execution stages, and a different bisection.** The stages of section 4.4
  switch the cuts off for a node and all its children the first time a full pass
  improves nothing, and that happens early on some branches, where the cuts still
  pay. Measured over fourteen problems, MC2 without them reaches 1.19 times the
  gain of the best combination per case against 2.30 with them on.

  What the stages were really contributing is not the cuts they switch off but
  the bisection rule of their final stage: it splits the parameter that most
  narrows the **wider side of the projection**, which is the side the termination
  test reads, where the rest of the search splits by the **area** of the
  projection. Shrinking an area can leave the deciding side untouched. Under the
  conservative reading of the boundary columns, where the strip cuts stop
  applying, the area rule does not terminate at all on four of five problems
  while this one answers in milliseconds with an equal or better gain. MC2 uses
  the rule everywhere; MC (thesis) keeps the stages, under its own setting, as
  the published algorithm.

## What it costs and what it buys

On the toolbox example 2, with the published reading of the boundary columns:

| structure | MC2 | MC (thesis) |
|---|---|---|
| k(s+z)/(s+p) | 557.02, 11 ms, 362 nodes | 567.69, 16 ms, 1157 nodes |
| one more pole | 407 879, 784 ms, 13 462 nodes | 406 692, 978 ms, 26 092 nodes |
| one more zero | 293.18, 6.4 s, 176 045 nodes | 293.36, 9.7 s, 511 364 nodes |

And with the conservative reading, which is the default, MC2 returns 567.32 in
0.15 s: the same answer NT, NK and MC (2021) return there in 69, 78 and 131
seconds. That is the point of the bisection rule, and it is what makes a
certified answer affordable at all.

## Where it lives

`src/core/loopshaping/mc2/algorithm_mc2.h`, `.cpp`.

Tests: `tests/backend/thesis_benchmark_test.cpp` (the goldens of the fixture
under both readings), `tests/backend/specification_check_test.cpp` (what it
returns, checked against the specifications themselves),
`tests/backend/mc_thesis_strategies_test.cpp` (every combination of the
strategies reaches the same optimum).
