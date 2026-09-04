# Boundaries

A specification says how the closed loop must behave for every plant of the
family. On the Nichols chart, at one design frequency, that translates into a
curve: the nominal open loop L0(jω) must stay on the allowed side of it. QFTbx
computes those curves numerically, one per specification and frequency, and merges
them into a single boundary per frequency, which is what the loop-shaping
algorithms consume.

**Reference.** I. Martínez Forte, *QFTbx, herramienta de diseño QFT:
especificación de requisitos y prototipado*, degree project, University of Murcia,
2013: section 3.12 (the boundary computation use case) and appendix B.3 (the
algorithm), which follows the grid approach of J. C. Moreno, A. Baños and
M. Berenguel, *Improvements on the computation of boundaries in QFT*, 2006. The
CUDA version is chapter 4 of the master's thesis (2014). See
[references](references/README.md).

## The sheets

The Nichols window is discretised into a grid of phases and magnitudes. For every
design frequency ω and every grid point L = m·e^{jθ}, the engine sweeps the
template {P} of that frequency and evaluates the worst case of each closed-loop
quantity over the template, in dB:

| Specification family | Sheet value at L |
|---|---|
| Robust stability | max over P of \|L / (P0/P + L)\| |
| Tracking | max over P of \|T\| minus min over P of \|T\|, the spread of the closed loop |
| Output disturbance rejection | max over P of \|(P0/P) / (P0/P + L)\| |
| Input disturbance rejection | max over P of \|P0 / (P0/P + L)\| |
| Control effort | max over P of \|(L/P) / (P0/P + L)\| |

Each family yields one *sheet*, a surface D(θ, m) over the grid. Because the
template enters only through the quotient P0/P, the sweep runs over the template
contour when one has been computed and over the full template otherwise; the
result is the same, and the golden test checks it.

Every specification carries its bound as a function of frequency. Cutting a sheet
at the bound of its specification gives the level curve where the closed loop
sits exactly at the limit: that curve is the boundary. The side of the curve that
satisfies the specification is recorded with it as a label, because the union
below needs to know whether the allowed region of each curve lies above or below.

## Tracing the level curves

The cut is performed on the grid: every 8-connected region of cells where the sheet
is at or above the cut height is found, and its border is walked with a Moore
boundary trace, which visits the border cells in order around the region. Each
region gives one trace, mapped back from grid indices to phase and magnitude. A
trace that reaches the window frame is extended with one synthetic point before
its first sample and one after its last, so the union can close open contours
against the frame.

The GPU sheets are read column by column and the CPU sheets row by row; the walk is
the same code for both, given a cell accessor. This matters because the two paths
used to trace differently and produced different boundaries.

## The 1D union

The boundary of a frequency is the worst case of all its specifications. The union
buckets the traces of every specification by phase and keeps, per phase bucket,
only the magnitudes that remain binding: the most restrictive one on each side,
honouring the label of the curve it came from. The result is a flat point set per
frequency and, equivalently, the phase-bucketed layout sorted by magnitude that
the `.qft` files have always stored.

Multi-valued boundaries, where a phase column meets the allowed region more than
once, survive the union as they are. The fixture `tests/data/multivaluados.qft`
exercises that case.

## Guards

A sheet value that is not a number would read as "allowed" in the trace; the
engine refuses it. The classic cause is an undamped resonance inside the plant
uncertainty, whose frequency moves with the parameters (the ACC'90 benchmark is
built on exactly that): the error message says so and suggests damping it
lightly. A grid with fewer than two phases or magnitudes, or a window of zero
width, is refused before any sheet is built.

## Parallelism

The sweep over the grid is embarrassingly parallel. The CPU engine parallelises
the grid with OpenMP; the optional CUDA engine (`USE_CUDA`) computes the sheets on
the GPU, one thread per grid cell, as designed in the master's thesis.

## Where it lives

- `src/core/boundaries/boundary_engine.h`, `.cpp`: the sheets and the driver.
- `src/core/boundaries/contour_tracer.h`, `.cpp`: the level-curve trace.
- `src/core/boundaries/boundary_union_1d.h`, `.cpp`: the union.
- `src/core/boundaries/boundary_data.h`, `boundary_types.h`: the results.
- `src/core/gpu/boundary_sheets_cuda.cu`: the CUDA sheets.

Tests: `tests/backend/boundaries_golden_test.cpp` (grid metadata, traces and union
against a golden project, contour input equivalent to the full template, the
critical-point and resonance guards) and `tests/backend/boundary_bucket_bounds_test.cpp`.
