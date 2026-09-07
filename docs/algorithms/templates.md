# Templates

At one design frequency ω, an uncertain plant P(jω) is not a point of the
Nichols chart but a set: the *template*, or value set, of the plant at that
frequency. Templates are the raw material of a QFT design, because the
boundaries are computed by sweeping them. QFTbx computes each template by brute
force and then reduces it to its contour, since the boundary computation only
needs the border of the set.

**References.** I. Martínez Forte, *QFTbx, herramienta de diseño QFT:
especificación de requisitos y prototipado*, degree project, University of Murcia,
2013: section 3.9 and appendices B.1 (templates) and B.2 (ε-hull). The ε-hull is
defined in M. Nordin's licentiate thesis (KTH, 1993); the implementation follows
the one F. J. Montoya Dato developed for his doctoral thesis (University of
Murcia, 1998), the MATLAB function `epsh2` of `EPSHULL.M`, which J. Cervera,
A. Baños and I. Horowitz also used for their general plant templates (2001). The
CUDA version is chapter 4 of the master's thesis (2014). See
[references](references/README.md).

## Brute-force templates

Each uncertain parameter of the plant carries a range and a number of grid
points. The engine evaluates the plant over the cartesian product of those grids,
one point of the Nichols chart per parameter combination, and does so for every
design frequency. A plant written as an expression in `s` is parsed once, when
it is entered, and evaluated at s = jω with its parameters bound by position, so
the sweep is an evaluation loop. The result is one point cloud per frequency, in
dB and degrees.

The sweep is limited by `limits.max-template-points` in the settings, because the
product of the grids grows fast with the number of uncertain parameters. The
frequencies are processed in parallel with OpenMP, and the cloud of each
frequency lands in its own slot, so the result does not depend on the thread
schedule.

## The ε-hull contour

The contour of a template is not its convex hull: templates are often
non-convex, and the concavities matter for the boundaries. The ε-hull is a
contour that follows the cloud at resolution ε: a closed walk over the points of
the cloud in which every step is shorter than ε and the walk turns as tightly as
the points allow.

The algorithm, as `EPSHULL.M` implements it:

1. Remove duplicate points and sort them as MATLAB sorts complex values, by
   modulus and then by phase. This order resolves ties exactly as the reference
   does.
2. Start at the point with the largest real part, the rightmost one.
3. Choose the second point among the neighbours within ε of the first: the one
   whose circle of radius ε/2 sticks out of the region covered so far, that is,
   the one of minimum turning angle ψ.
4. From the pair (previous, current), choose the next point by the same rule
   among the neighbours within ε of the current one.
5. Stop when the walk returns to the initial pair, so the contour is closed. The
   number of contour points is capped at three times the number of cloud points.

Each design frequency has its own ε, because the size of the templates changes
along the frequency axis; the ε vector is entered with the frequencies.

### A limitation of the reference, and the fallback

Found while porting: on clouds made of clusters spaced about ε apart, the
reference walk cycles without ever returning to its initial pair. In that case
the engine falls back to the relaxed walk the toolbox used historically, which
starts at the point with the largest imaginary part and returns an open,
deduplicated contour. That fallback is a valid ε-cover of the cloud, not the
canonical hull, and the engine reports it with a warning. Which frequencies fall
back depends on the last digits of the cloud, because the reference walk is that
sensitive; the golden test detects the rule per frequency rather than fixing it.

## Parallelism

The brute-force sweep is embarrassingly parallel. The CPU engine parallelises
over frequencies with OpenMP; the optional CUDA engine (`USE_CUDA`) evaluates the
parameter grid on the GPU and traces the contour there, as designed and measured
in the master's thesis.

## Where it lives

- `src/core/templates/template_engine.h`, `.cpp`: the sweep and the ε-hull.
- `src/core/templates/parameter_grids.h`: the parameter grids.
- `src/core/templates/cloud_set.h`: the clouds and contours per frequency.
- `src/core/gpu/template_contour_cuda.cu`: the CUDA version.

Tests: `tests/backend/e_hull_test.cpp` (the hull on hand-made clouds: a grid
keeps exactly its border, a triangle drops its centre, collinear points are
traversed both ways), `tests/backend/templates_golden_test.cpp` (the sweep and
the contour against a golden project, the start rule, the contour as a subset of
the template) and `tests/backend/templates_determinism_test.cpp` (the same result
whatever the thread count).
