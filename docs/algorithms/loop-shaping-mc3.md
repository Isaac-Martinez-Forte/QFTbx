# Algorithm MC3: the gain out of the search tree

Every algorithm on these pages searches a box of controller parameters that
includes the gain: **x** = (**k**, **z**, **p**, ...), bisected like any other
coordinate. MC3 does not. It searches over the zeros and poles alone and treats
the gain exactly, as a set rather than an interval to be cut down.

It is a prototype. It is here because it is the natural end of what the other
algorithms do a piece at a time, and because measuring it says where the cost of
an interval search really is.

## Why the gain can leave the tree

The phase of the open loop does not depend on the gain, and its magnitude in
decibels is the gain in decibels plus a term that does not depend on it either:

```
∠L0(jω) = ∠C1(jω) + ∠P0(jω)          |L0(jω)|dB = 20 log10 k + |C1(jω) P0(jω)|dB
```

with C1 the controller at unit gain. So a box **B** of zeros and poles, projected
at unit gain onto a rectangle of the Nichols chart at each design frequency,
becomes the whole family of loops for every gain by *sliding that rectangle
vertically*. Reading the boundary columns against a sliding rectangle gives
exactly, as unions of intervals of g = 20 log10 k:

- **G_in(B)**, the gains for which the whole box is certainly feasible at every
  frequency: the shifted rectangle fits inside an allowed interval of every
  column its phase span covers;
- **G_inf(B)**, the gains for which the whole box is certainly infeasible at some
  frequency: the shifted rectangle lies inside a forbidden gap of every column of
  the span.

A node therefore carries its box and the set K of gains still admissible for it.
Visiting it contracts K by G_inf(B); the node's lower bound is min K, which is
the least gain any controller in the box could still achieve; and G_in(B) ∩ K
certifies a controller for the **whole box** as soon as the box is narrower than
the corridor the boundaries leave. Nodes whose lower bound cannot beat the best
certified gain by the tolerance are discarded. Only the zeros and poles are
bisected, logarithmically, the widest first.

This generalises what the published searches already do in pieces: the gain
contractors C_g- and C_g+ of [NT](loop-shaping-nt.md), the Quick Solution of
[NK](loop-shaping-nk.md) and its mirror in the thesis, and the exact best gain of
[MC2](loop-shaping-mc2.md) at a point. Those read one side, or one gap, or one
point; this reads both sides, every gap of every column, over a whole box.

## Nominal stability as a gain contractor

The other algorithms test nominal stability on one point controller of a box and,
where a whole region of the parameter space is unstable, bisect it down to the
tolerance to reject it corner by corner. MC3 tests a **slice** of the admissible
gains, 20 dB wide, for the whole box at once; a slice whose every member is
closed-loop unstable is removed and the next one tested, until one survives or
nothing is left. That raises the bound instead of splitting the box.

## What it is worth, measured

On the toolbox example 2 with four parameters it visits less than half the nodes
MC2 does. Over the battery it visits 2.4 times fewer nodes and takes 1.08 times
**longer**: its node is dearer than theirs, and four fifths of that time is in
the box-wide stability test. Fewer, more expensive nodes is the trade it makes,
and the arithmetic of that trade is what the next work on it has to change.

## Where it lives

`src/core/loopshaping/mc3/algorithm_mc3.h`, `.cpp`.

Tests: `tests/backend/mc3_test.cpp` (the answer on the fixture under both
readings of the boundary columns, and that what it returns satisfies the
specifications themselves).
