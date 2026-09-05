# Automatic loop shaping: the common machinery

Loop shaping is the last step of a QFT design: choosing the controller C so that
the nominal open loop L0 = P0 C satisfies every boundary at every design
frequency. Doing it by hand on the Nichols chart is the classic QFT skill;
*automatic* loop shaping turns it into an optimisation problem. QFTbx implements
five algorithms of one family, the interval branch & bound algorithms, and this
page describes what they share. Each algorithm then has a page of its own.

**References.** S. Tharewal, *Automated synthesis of QFT controllers and
prefilters using interval global optimization techniques*, PhD thesis, IIT Bombay,
2005, chapter 3; I. Martínez-Forte, *Aceleración de algoritmos intervalares de
ajuste automático del lazo en QFT*, PhD thesis, University of Murcia, 2022,
chapters 1 and 3. See [references](references/README.md).

## The problem

The controller has a fixed structure with real zeros and poles and a gain,

    C(s) = k · Π_i (s + z_i) / Π_j (s + p_j)

and the design variables are the vector x = (k, z_1, ..., p_1, ...). The cost is
the high-frequency gain, the price of feedback: the problem is to minimise k
subject to L0(jω, x) lying on the allowed side of the boundary at every design
frequency ω. It is nonlinear and nonconvex, so local methods and heuristics find
feasible controllers but cannot prove them optimal. Interval methods can.

## Boxes and the natural interval extension

An interval branch & bound works on *boxes*: interval vectors
**x** = (**k**, **z_1**, ..., **p_1**, ...), each a set of controllers. For one
frequency, the loop L0(jω, **x**) is then a region of the Nichols chart, and the
*natural interval extension* encloses it in a rectangle by evaluating the
magnitude and phase of L0 in interval arithmetic:

    |L0| in dB  =  20 log10 |k| + 20 log10 |p0| + Σ 20 log10 |jω + z_i| − Σ 20 log10 |jω + p_j|
    ∠L0         =  ∠p0 + Σ atan(ω / z_i) − Σ atan(ω / p_j)

with each term evaluated over its interval. The fundamental theorem of interval
analysis guarantees that the rectangle contains the true image of every
controller of the box; the rectangle may be larger than the image, never smaller.
The phase is mapped onto the branch (-360°, 0]; a phase set that crosses the
branch cut degrades to the whole branch, which is conservative and keeps the
guarantee. The product is assembled in polar form, as the thesis writes it:
each factor jω + x is a segment of the complex plane whose magnitude and phase
ranges are read rigorously, and the factors multiply their magnitudes and add
their phases, so the enclosure does not grow with the number of factors as a
product of rectangles would. The interval arithmetic underneath is the kv
library in its rounding-emulation mode, which never changes the floating-point
rounding mode; the logarithms and arc tangents of the projection are the C
library's values widened by four ulps (see `src/core/math/interval.h`). A
domain error (a division by an interval containing zero) is reported as an
exception rather than aborting the process.

Only zero-pole-gain controller structures are supported by the interval
algorithms for now; other structures are refused with a message.

## Classifying a box

Comparing the rectangle against the boundary of each frequency gives one of three
verdicts, all of them certificates:

- **certainly infeasible**: the rectangle lies wholly on the forbidden side at
  some frequency, so no controller of the box is feasible. The box is discarded.
- **certainly feasible**: at every frequency the rectangle lies wholly on the
  allowed side. Every controller of the box is feasible, and the optimum over
  the box sits at its lowest gain.
- **ambiguous**: neither. The box is bisected and its halves classified again.

The test is a parity test of the rectangle's corners against the boundary union
of each frequency, and while it runs it also records the extremes of the boundary
magnitude over the rectangle's phase span (B_min and B_max in the papers); those
extremes drive the cutting equations of NT, NK and the MC algorithms. Multi-valued
boundaries are handled by the same test.

## The search

All five algorithms keep a list of live boxes ordered by the infimum of the gain.
The head of the list is the box that could still hold the best controller; when
the head is certainly feasible, its lowest-gain corner is the global optimum and
the search stops. The search also stops when the head is smaller than the
user's epsilon, and extracts its feasible corner. The list is the memory of the
search, and `search.max-live-nodes` in the settings caps its size, because on a
hard problem the list can grow to tens of millions of boxes before the problem
resolves: the search then stops with a message asking for a looser epsilon or a
narrower controller box.

What epsilon measures depends on the algorithm. NT, NK and both MC algorithms
measure the width of the rectangle on the Nichols chart, in dB and degrees, as
their references do. MR measures the width of the controller parameter box, as
its reference does. The same number therefore means different things in
different algorithms; the loop-shaping dialog says which applies.

## Nominal closed-loop stability

The QFT bounds alone do not exclude a loop that encircles the critical point: a
loop with |L0| far above 0 dB beyond -180° satisfies every magnitude bound and is
closed-loop unstable. The references formulate the problem with a nominal
stability requirement, and the historical implementation approximated it with a
hand-made penalty at 2 rad/s that biased the search. QFTbx now completes the
feasibility test with a proper criterion: the Nyquist criterion on the Nichols
chart of Cohen, Chait and Yaniv (1994), the one the MATLAB QFT Toolbox applies.
For a nominal loop with no poles in the open right half-plane, the closed loop is
stable if and only if the signed crossings of the rays
{∠L0 ≡ -180° (mod 360°), |L0| > 0 dB} cancel out. By the boundary crossing
principle (Tharewal 2005, section 3.3.5), satisfied stability bounds plus one
nominally stable controller of a bounds-feasible box make the whole box, and the
whole plant family, robustly stable; an unstable controller discards the box.

The criterion presumes a nominal plant without right-half-plane poles, and it
samples the nominal loop on a logarithmic grid extended three decades beyond the
design frequencies, refined where the phase turns fast. The `stability.*` keys of
the settings tune that sampling; they do not touch the criterion.

## Benchmarks

Two problems from the literature travel with the tests as `.qft` fixtures and are
the reference cases of the thesis: example 2 of the MATLAB QFT Toolbox manual
(`tests/data/qft_toolbox_ex2.qft`) and the ACC'90 benchmark
(`tests/data/acc90.qft`), a marginally unstable plant with an undamped resonance
whose frequency moves with the uncertainty, chosen by the literature precisely
because it is hard. Any change to the algorithms should be tried on both.

## Where it lives

- `src/core/loopshaping/loop_shaping.h`, `.cpp`: the facade that picks and runs
  an algorithm and hands back the controller.
- `src/core/loopshaping/natural_interval_extension.h`, `.cpp`: the Nichols
  rectangle of a box.
- `src/core/loopshaping/boundary_violation_detector.h`, `.cpp`: the parity
  classification and the boundary extremes.
- `src/core/loopshaping/box_classification.h`, `.cpp`: the three verdicts.
- `src/core/loopshaping/nominal_stability_checker.h`, `.cpp`: the Nichols-chart
  Nyquist criterion.
- `src/core/loopshaping/ordered_list.h`, `search_node.h`, `mc_search_node.h`:
  the live list and its nodes.
- `src/core/loopshaping/common_functions.h`: the bisection, the point extracted
  from a box, and the shared Quick Solution cuts.

Tests: `tests/backend/interval_extension_test.cpp`,
`tests/backend/nominal_stability_test.cpp`,
`tests/backend/loopshaping_structures_test.cpp`,
`tests/backend/thesis_benchmark_test.cpp`, and
`tests/backend/literature_validation_test.cpp`, which checks that the controllers
published for the benchmarks are feasible, that lower gains are not, and that the
minimal feasible gain matches Tharewal's.
