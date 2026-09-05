![QFTbx](qftbx_banner.svg)

# QFTbx architecture

QFTbx is a desktop toolbox for robust controller design with Quantitative Feedback
Theory (QFT). The application walks the user through the standard QFT pipeline; each
stage has a dialog in the GUI and an engine in the model layer.

## The QFT pipeline

```
 plant + parametric     design           templates          specifications
 uncertainty        →   frequencies  →   (value sets +  →   (stability,
                        (ω vector)       ε-hull contour)    tracking, ...)
                                                                 │
                        controller   ←   automatic loop  ←   boundaries
                        structure        shaping             (Nichols plane)
```

1. **Plant**: transfer function with parametric uncertainty. Several input formats
   (gain/pole-zero forms, polynomial coefficients, free-format expression in `s`
   evaluated by the toolbox's own expression tree).
2. **Design frequencies (ω)**: linear/log spacing or manual values.
3. **Templates**: for each ω, the set of possible plant responses in the Nichols plane
   (brute-force sweep over the uncertain parameters) reduced to its contour with the
   ε-hull algorithm (Nordin 1993, in Montoya's implementation).
4. **Specifications**: robust stability and performance bounds.
5. **Boundaries**: for each ω and specification, the allowed/forbidden regions for the
   nominal open loop, computed on a phase×magnitude grid and traced with a contour
   follower; per-frequency boundaries are then merged (1D intersection algorithm).
6. **Loop shaping**: automatic controller synthesis honouring the boundaries (interval
   arithmetic; several algorithms).

Each algorithm, its reference and how the implementation follows it are described
in [docs/algorithms](algorithms/README.md).

## Modules

| Directory | Contents |
|---|---|
| `src/core/common/` | `qftbx::Exception` and its subclasses; number and token text helpers |
| `src/core/math/` | Numeric helpers (`linspace`/`logspace`), ranges and points, constants, and the expression tree: the one expression engine of the toolbox (see below) |
| `src/core/system/` | Plant/controller representation: `LtiSystem` hierarchy, transfer functions, parameters |
| `src/core/frequencies/` | The design frequency set (`Omega`) |
| `src/core/specifications/` | Validated specification set (`qftbx::Specification`) |
| `src/core/templates/` | Brute-force template computation and ε-hull contour (`TemplateEngine`) |
| `src/core/boundaries/` | Boundary computation: sheets (`BoundaryEngine`), contour tracing (`ContourTracer`), 1D union (`BoundaryUnion1D`), results view (`BoundaryData`) |
| `src/core/loopshaping/` | The five loop-shaping algorithms and their interval-arithmetic support |
| `src/core/project/` | What a project holds, owned by value (`ProjectData`); the user settings (`Settings`) |
| `src/core/pipeline/` | The design steps as data (`Step`), one stage per step, background execution and cancellation |
| `src/core/gpu/` | Optional CUDA kernels for templates/boundaries (`USE_CUDA`) |
| `src/persistence/` | Load/save of `.qft` project files (pugixml; versioned English dialect, legacy Spanish files still load) |
| `src/app/` | `ProjectController`: the single mediator between GUI and core, one method per design step |
| `src/gui/` | Qt Widgets HMI: one folder per design step with its dialog and viewers (QCustomPlot), plus `application/` (shell, main window) and `common/` (shared widgets and helpers) |
| `tests/` | GoogleTest suites in `backend/` and `gui/`; golden `.qft` projects in `tests/data/` |

Build targets: `qftbx_core` (the algorithms and the model), `qftbx_persistence`,
`qftbx_app` (the mediator), `qftbx_gui`, and the `QFTbx` executable on top of them;
`qftbx_tests` links the facade. Every folder has a `CMakeLists.txt` that adds its own
files to the target it belongs to (`cmake/QftbxFunctions.cmake`), so no file list is
kept by hand.

## Expressions

Every expression the toolbox reads goes through `ExpressionTree`
(`src/core/math/expression_tree.h`): the free-form plants a user writes in `s`,
the reparametrisation of a parameter (`a*10`), the numbers typed into dialog
fields, and the constraints of algorithm MR, which are built in memory with the
`Expression` builder instead of formatted and parsed. The grammar: identifiers
(`[A-Za-z][A-Za-z0-9_]*`), numbers with a decimal point and scientific notation,
`+ - * / ^` with the power binding to the right, a unary minus, parentheses,
blanks, the constants `pi` and `e` in either case, and the functions `sin`,
`cos`, `tan`, `asin`, `acos`, `atan`, `sinh`, `cosh`, `tanh`, `exp`, `sqrt`,
`abs`, `ln`, `log`, `log10`, `lg`, `log2`. A tree evaluates over reals, over
complex numbers (a plant at `s = jω`) and over intervals, and propagates
constraints with the HC4 filter. The names a parameter cannot take are the
functions, the constants and the Laplace variable `s`; `k` is a name like any
other.

## Third-party libraries (vendored in `3rd-party/`)

- **QCustomPlot** — plotting inside the Qt GUI.
- **kv** — verified interval arithmetic (header-only), behind the toolbox's own
  `Interval`, `ComplexInterval` and `PolarInterval` types in `src/core/math/interval.h`;
  used by loop shaping.

## Error handling

Backend code never interacts with the user: it throws exceptions derived from
`qftbx::Exception` (`src/core/common/exception.h`) and the GUI shows the message. An
exception escaping a Qt slot propagates into the event loop and terminates the
process, so slots reaching backend code catch at their boundary — and, since one
forgotten slot is enough to lose the application, `qftbx::Application` overrides
`QApplication::notify()` as a last-resort net that turns anything that got through
into an error dialog.

## Persistence

Projects are saved as `.qft` files (XML): plant, frequencies, specifications,
computed templates/contours, boundaries, controller structure and loop-shaping
results. `tests/data/` ships real projects used as golden data by the tests.

## The API documentation

The reference for the classes named above is generated with Doxygen:

```
cmake --build build --target docs      # or just: doxygen
```

It lands in `docs/api/html/index.html` (not committed). The configuration is the
`Doxyfile` at the root of the repository, written by hand and commented; the target
only exists when doxygen is installed. Formulas are rendered by MathJax from a CDN,
so the pages need internet for those and nothing else.
