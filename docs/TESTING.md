# Testing

The tests are built with the project (`QFTBX_BUILD_TESTS`, on by default)
into two binaries, both GoogleTest: `qftbx_tests`, the backend suite, and
`qftbx_gui_tests`, the headless smoke suite of the dialogs. CTest knows
every test of both.

## Running

    ctest --test-dir build
    ctest --test-dir build --output-on-failure          # the detail of a failure
    ctest --test-dir build -R Templates                 # by name, a regular expression
    ctest --test-dir build -j 4                         # several at a time

The binaries can also be run directly, which is faster for one suite and
gives GoogleTest's own filters:

    ./build/tests/backend/qftbx_tests --gtest_filter='IntervalArithmetic.*'
    ./build/tests/backend/qftbx_tests --gtest_list_tests

The GUI suite forces Qt's `offscreen` platform plugin, so it runs on a
machine with no display; CTest sets the environment for it.

The parallel code respects `OMP_NUM_THREADS`. The long loop-shaping tests
print their elapsed time and their peak number of live nodes.

## What the suites cover

**The model.** Plants and controllers in their four forms and their
evaluation (`SystemInvoke`, `FreeFormPlant`, `TimeConstantGainValidation`),
parameters and their reparametrisation, the expression tree from the grammar
to the builder (`ExpressionGrammar`, `ExpressionBinding`, `ComplexEvaluation`,
`ExpressionBuilder`, `ExpressionNames`), the frequency set (`Omega`, the
sequences), the specifications and their units, the settings file
(`SettingsFile`, `Settings`).

**Templates and boundaries.** The ε-hull (`EHull`), determinism and
validation of the templates, the golden contours of the fixtures
(`TemplatesGolden`), the boundary sheets and their critical point, the
bucket bounds of the union, the golden boundaries (`BoundariesGolden`).

**Loop shaping.** The interval arithmetic and the natural interval extension
(`IntervalArithmetic`, `ComplexIntervalArithmetic`, `PolarIntervalArithmetic`,
`NaturalIntervalExtension`), the nominal stability check, the ordered list
and the pipeline stages, and the five algorithms: against the published
results where they exist (`LiteratureValidation`, `MrArticleValidation`,
`QuickSolutionPaperExample`), against the thesis benchmarks
(`ThesisBenchmarkGolden`), and against pinned results on the small fixtures
(`LoopShapingGolden`).

**Persistence.** The `.qft` reader and writer, their error paths on
malformed files, round trips through the fixtures, the staleness of results
when their inputs change.

**The application layer.** The design steps and their sequence, cancellation,
the background runner, the failed-computation paths, and the dialogs opened
and driven headless (`GuiSmoke`).

## The fixtures

`tests/data/` holds real projects used as golden data, described in
[its README](../tests/data/README.md): the ACC example of Cervera and Baños,
two example plants, the multivalued-boundaries case of Moreno and Baños, and
the two benchmarks of the thesis, Matlab QFT Toolbox example 2 and the
ACC'90 problem, generated through the real pipeline at a resolution the
published optima need. The malformed `corrupt_*.qft` and `invalid.qft` feed
the reader's error paths.

## Goldens

A golden test pins current behaviour: the contour of a template, the union
of a boundary, the controller an algorithm returns on a fixture. It does not
prove the behaviour correct; correctness is judged against the paper each
algorithm comes from, in the literature validations. A golden that fails
after a change is a question, not necessarily a defect: either the change
was not meant to alter results and the golden caught a regression, or it
was, and the golden is re-pinned deliberately, with the reason in the
commit. The loop-shaping goldens carry a relative tolerance because the
exact optimum moves with the floating-point details of the bisection.

The optimum of a QFT loop-shaping problem is often not unique. On a problem
with stability as its only specification every box that realises the
optimal gain is a solution, and each algorithm returns the first one its
search meets; the zero and pole pinned for such a case are bisection points
of the search domain and move whenever the enclosures change tightness.

## Sanitizers and profiling

A build with `-DQFTBX_SANITIZERS=address,undefined` runs the whole suite
under AddressSanitizer and UndefinedBehaviorSanitizer; run it with
`ASAN_OPTIONS=detect_leaks=0`, since Qt's global objects report leaks that
are not ours. For Valgrind and Callgrind configure with `-DUSE_NATIVE_ARCH=OFF`,
because they cannot execute the AVX-512 instructions of a native build.
