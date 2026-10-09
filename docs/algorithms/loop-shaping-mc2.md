# Algorithm MC2: the thesis strategies with their formulation corrected

MC2 starts from what the strategies of the doctoral thesis do once their
published equations are made to say what their text says: MC (thesis) with four
corrections to the cutting rules, an exact best gain, and the execution stages
taken out in favour of the bisection rule that was hiding inside them. Unlike
the other algorithms, it does not decide on the boundary columns: it judges
every controller it returns against the specifications over the whole template,
at the loop's own phase (see [how the columns are
read](../CONFIGURATION.md#how-the-columns-are-read)).

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

- **The exact best gain of a vertex.** With its zeros and poles fixed, the gains
  a point controller may take form a finite union of intervals, and the best one
  is the least member of it. No bisection, no tolerance.
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

## What it returns

The answers MC2 gives to the published problems, each checked against its
specifications, are in [`examples/`](../../examples/README.md).

## Where it lives

`src/core/loopshaping/mc2/algorithm_mc2.h`, `.cpp`.

Tests: `tests/backend/mc2_exact_test.cpp` (MC2 as it runs by default),
`tests/backend/thesis_benchmark_test.cpp` (the goldens of the fixture under both
readings of the columns), `tests/backend/specification_check_test.cpp` (what it
returns, checked against the specifications themselves),
`tests/backend/mc_thesis_strategies_test.cpp` (every combination of the
strategies reaches the same optimum).
