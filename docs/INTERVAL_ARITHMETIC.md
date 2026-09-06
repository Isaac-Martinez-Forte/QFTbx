# The interval arithmetic

The loop-shaping algorithms are interval branch and bound methods: a box of
controller parameters is projected onto the Nichols chart as a rectangle
that is guaranteed to contain the response of every controller in the box,
and the rectangle is compared with the boundaries. The guarantee is what
makes the result a certificate rather than a sample, and it rests on the
arithmetic being rigorous: every operation rounds outwards, every function
encloses its true range.

## The layer

All of it goes through `src/core/math/interval.h`, which defines three types
and is the only place the rest of the code sees:

- `Interval`, a closed real interval with the four operations, the
  elementary functions and the set operations (hull, intersection,
  containment).
- `ComplexInterval`, a rectangle of the complex plane, real and imaginary
  intervals, with a rigorous magnitude and a rigorous, continuous phase.
- `PolarInterval`, a magnitude interval and a phase interval, the form in
  which products and quotients keep their shape.

The natural interval extension of a controller (thesis, section 1.2.5)
is assembled in polar form: each factor `jω + x` of the controller is a
horizontal segment of the complex plane whose magnitude and phase ranges are
read exactly, and the factors multiply their magnitudes and add their
phases. A product of rectangles would grow a looser box with every factor.
The result is converted once to decibels and degrees on the branch
(-360°, 0]; a phase set that crosses the branch cut degrades to the whole
branch, which is conservative and keeps the guarantee.

A domain error, such as the square root of an interval reaching below zero,
the logarithm of one that is not positive or a division by an interval
containing zero, throws `std::domain_error` whichever backend is in use; the
algorithms never let one happen, and the background runner reports one
instead of letting it terminate the process.

## The backends

The library underneath is chosen at configuration time with
`QFTBX_INTERVAL_BACKEND` and confined to one section of the header. The
backend supplies the four operations, the square root, the integer power and
an enclosure of π; everything else, the set operations, the square, the
phase of a rectangle, the domain checks and the functions below, is written
once over those. So the two backends must give the same enclosures up to
rounding, and a disagreement beyond that is a bug in one of them.

**kv** (the default) is Masahide Kashiwagi's verified computation library
(https://github.com/mskashi/kv, MIT licence, version 0.4.62). Only the
headers the toolbox needs are vendored in `3rd-party/kv`, with a small shim
for the two Boost utilities they include, so Boost is not required. It is
used in its rounding-emulation mode (`KV_NOHWROUND`): the directed roundings
are computed with error-free transformations instead of switching the
floating-point rounding mode, so its rigour does not depend on compiler
flags, on the optimisation level or on which thread runs it. On a
native-architecture build (`USE_NATIVE_ARCH`) the error-free product uses
the fused multiply-add instruction (`KV_USE_TPFMA`), which halves the cost
of a product.

**C-XSC** (`-DQFTBX_INTERVAL_BACKEND=cxsc`) is the classic library of the
University of Wuppertal, version 2.5.4, LGPL 2.1. It is not vendored: CMake
fetches it at configure time from the fork that builds with a current
compiler, https://github.com/Isaac-Martinez-Forte/cxsc-cpp17, pinned by the
tag `v2.5.4-qftbx1`, and builds it untouched as an external project. The
fork carries the exception specifications C++17 removed, the qualification
of `complex` where it clashes with the standard one, an error reporter that
does not assume the library's own start-up ran, and a CMake build that
installs into the standard library directory. A build without network
access points `FETCHCONTENT_SOURCE_DIR_CXSC` at a local checkout of the
fork. C-XSC switches the rounding mode around each operation, and the
targets that compile interval code are then built with `-frounding-math`;
without it the optimiser assumes round-to-nearest and reorders the
arithmetic across the switches, and the intervals silently stop being
rigorous. The flag is added by `qftbx_interval_arithmetic()` in
`cmake/QftbxFunctions.cmake` for this backend only, and never to the GUI
library, whose constexpr floating-point code it breaks.

## The elementary functions

The exponential, the logarithms, the trigonometric functions and their
inverses do not come from either library. Both libraries compute them with
interval series that cost some microseconds each, and the projection of a
controller box and the constraint trees of algorithm MR call them for every
factor of every box, millions of times in a run. The layer takes the C
library's values instead and widens them by four ulps on each side. glibc
documents its largest known errors for these functions on x86_64 as at most
two ulps (manual, "Known Maximum Errors in Math Functions"), so four ulps
double the largest listed bound, and four ulps of a phase in radians or of a
magnitude in decibels are far below anything the algorithms resolve. The
sine and cosine add the extremes +1 and -1 wherever a maximum or a minimum
may lie inside the interval; the positions of those extremes are located
with the enclosure of π, so a rounding near one can only add an extreme,
never miss one. The tangent returns the whole line when a pole may lie
inside.

## Checking the arithmetic

`tests/backend/interval_test.cpp` states the properties the algorithms rely
on and runs under either backend: outward rounding of the operations, the
rounding mode left untouched, enclosure of every function against
extended-precision values at thousands of arguments, the extremes of the
periodic functions, domain errors as exceptions, and the polar product
enclosing the product set of sampled factors.

The second backend is the cross-check of the first. The loop-shaping
results are deterministic, so a build with each backend can run the same
project and the two controllers can be compared: on the thesis benchmarks
(the ACC'90 problem with the five algorithms, and Matlab QFT Toolbox
example 2 with NT and MR) kv and C-XSC return the same controllers to the
last bit.
