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
   (gain/pole-zero forms, polynomial coefficients, free-format expression evaluated
   with muParserX).
2. **Design frequencies (ω)**: linear/log spacing or manual values.
3. **Templates**: for each ω, the set of possible plant responses in the Nichols plane
   (brute-force sweep over the uncertain parameters) reduced to its contour with the
   ε-hull algorithm (Cervera & Baños).
4. **Specifications**: robust stability and performance bounds.
5. **Boundaries**: for each ω and specification, the allowed/forbidden regions for the
   nominal open loop, computed on a phase×magnitude grid and traced with a contour
   follower; per-frequency boundaries are then merged (1D intersection algorithm).
6. **Loop shaping**: automatic controller synthesis honouring the boundaries (interval
   arithmetic via C-XSC; several algorithms).

## Modules

| Directory | Contents |
|---|---|
| `src/core/system/` | Plant/controller representation: `LtiSystem` hierarchy, transfer functions, parameters |
| `src/core/templates/` | Brute-force template computation and ε-hull contour (`TemplateEngine`) |
| `src/core/specifications/` | Validated specification set (`qftbx::Specification`) |
| `src/core/boundaries/` | Boundary computation: sheets (`BoundaryEngine`), contour tracing (`ContourTracer`), 1D union (`BoundaryUnion1D`), results view (`BoundaryData`) |
| `src/core/math/` | Numeric helpers (`linspace`/`logspace`), expression cache |
| `src/core/frequencies/` | The design frequency set (`Omega`) |
| `src/core/loopshaping/` | The five loop-shaping algorithms and their interval-arithmetic support |
| `src/core/project_controller.h` | `ProjectController`: the single mediator between GUI and core, one method per design step |
| `src/core/project_data.h` | What a project holds, owned by value |
| `src/core/exception.h` | `qftbx::Exception` and its subclasses |
| `src/core/gpu/` | Optional CUDA kernels for templates/boundaries (`USE_CUDA`) |
| `src/persistence/` | Load/save of `.qft` project files (pugixml; versioned English dialect, legacy Spanish files still load) |
| `GUI/` | Qt Widgets HMI: one dialog per stage plus plot viewers (QCustomPlot) |
| `tests/` | GoogleTest suite; golden `.qft` projects in `tests/data/` |

Build targets: `qftbx_core` (the algorithms and the model), `qftbx_persistence`,
`qftbx_app` (the mediator), `qftbx_gui`, and the `QFTbx` executable on top of them;
`qftbx_backend` is an interface target that pulls the non-GUI ones together, which is
what `qftbx_tests` links against.

## Third-party libraries (vendored in `3rd-party/`)

- **muParserX** — evaluation of free-format transfer-function expressions.
- **QCustomPlot** — plotting inside the Qt GUI.
- **C-XSC** — validated interval arithmetic, used by loop shaping.

## Error handling

Backend code never interacts with the user: it throws exceptions derived from
`qftbx::Exception` (`src/core/exception.h`) and the GUI shows the message. An
exception escaping a Qt slot propagates into the event loop and terminates the
process, so slots reaching backend code catch at their boundary — and, since one
forgotten slot is enough to lose the application, `qftbx::Application` overrides
`QApplication::notify()` as a last-resort net that turns anything that got through
into an error dialog. `mup::ParserError` is caught explicitly there because it does
not derive from `std::exception`.

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
