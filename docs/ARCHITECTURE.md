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
| `Modelo/EstructuraSistema/` | Plant/controller representation: `Sistema` hierarchy, transfer functions, polynomials |
| `Modelo/EstructurasDatos/` | Data holders exchanged between stages (`Var`, template/boundary/loop-shaping data) |
| `Modelo/Objetos/` | Small domain objects (design frequencies `Omega`) |
| `Modelo/Templates/` | Brute-force template computation and ε-hull contour |
| `Modelo/Boundaries/` | Boundary computation: grid ("sábana"), contour tracing, 1D merge |
| `Modelo/LoopShaping/` | Automatic loop-shaping algorithms and interval-arithmetic support |
| `Modelo/Herramientas/` | Shared helpers (`tools.h`, `qftbx::Exception`) |
| `Modelo/controlador.cpp` | `Controlador`: the single mediator between GUI and model |
| `DAO/` | In-memory storage of each stage's results (DAO pattern behind `FDAO`) |
| `XmlParser/` | Load/save of `.qft` project files (Qt XML streaming, isolated here) |
| `GUI/` | Qt Widgets HMI: one dialog per stage plus plot viewers (QCustomPlot) |
| `GPU/CUDA/` | Optional CUDA kernels for templates/boundaries (`USE_CUDA`) |
| `tests/` | GoogleTest suite; golden `.qft` projects in `tests/data/` |

Build targets: the backend (everything except `GUI/` and `main.cpp`) compiles into the
static library `qftbx_backend`; the `QFTbx` executable adds the GUI on top, and
`qftbx_tests` links the backend alone.

## Third-party libraries (vendored in `3rd-party/`)

- **muParserX** — evaluation of free-format transfer-function expressions.
- **QCustomPlot** — plotting inside the Qt GUI.
- **C-XSC** — validated interval arithmetic, used by loop shaping.

## Error handling

Backend code never interacts with the user: it throws exceptions derived from
`qftbx::Exception` (`Modelo/Herramientas/exception.h`) and GUI slots catch them and
show the message. Qt aborts if an exception escapes the event loop, so every slot
reaching backend code catches at its boundary.

## Persistence

Projects are saved as `.qft` files (XML): plant, frequencies, specifications,
computed templates/contours, boundaries, controller structure and loop-shaping
results. `tests/data/` ships real projects used as golden data by the tests.
