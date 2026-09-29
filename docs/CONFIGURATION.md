# The settings file

QFTbx reads an optional settings file, `qftbx.conf`. Every key has a compiled
default and the file only says what to change: a key left out keeps its
default, and with no file the program runs as shipped.
[`qftbx.conf.example`](../qftbx.conf.example) lists every key commented out;
copy it and uncomment what you change.

## Where it is read from

The first one that exists wins:

1. the file named by the `QFTBX_CONFIG` environment variable (naming a file
   that cannot be read is an error);
2. `./qftbx.conf`, in the directory the program was started from;
3. `$HOME/.config/qftbx/qftbx.conf`.

## Syntax

An INI file. Comments start with `#` or `;`. A key is its section and its
name: `max-grid-cells` under `[limits]` is `limits.max-grid-cells`. A value
that is not a number, or lies outside its range, stops the program with a
message naming the key and the line. Only the application reads the file;
the test suite builds its own settings.

## The sections

**`[interface]`**. The window writes `canvas` and `window` itself, and the
View menu writes `language` and `theme`.

| Key | Default | Values | Meaning |
|---|---|---|---|
| `language` | `system` | `system` or a language code | The language the interface starts in; the codes are those of the compiled translations (`src/gui/translations/README.md`) |
| `canvas` | empty | a list of `phase:size` | The canvas as it was when the application last closed |
| `theme` | `system` | `system`, `light`, `dark` | `system` takes the machine's palette, `light` and `dark` the toolbox's own |
| `window` | empty | `width height`, `maximized` | The window size when it last closed |
| `digits` | `4` | 1 to 17 | Significant digits the forms show; the project file keeps every digit |

**`[log]`**. A record of what the engines did and how long it took, one line
per stage.

| Key | Default | Range | Meaning |
|---|---|---|---|
| `enabled` | `0` | 0 or 1 | Whether the record is written |
| `path` | empty | a path | Empty means `$XDG_STATE_HOME/qftbx/qftbx.log`, or `$HOME/.local/state/qftbx/qftbx.log`; a path that cannot be written leaves the record closed |
| `size-limit-kilobytes` | `1024` | 16 to 1048576 | Past this size the file becomes the previous generation (`.1`); two generations are kept |

**`[limits]`**. Ceilings against typos. They only refuse input, so none
changes a computed result.

| Key | Default | Range | Meaning |
|---|---|---|---|
| `max-grid-cells` | 10000000 | 4 to 1e15 | Cells of the Nichols grid of the boundaries, phase points × magnitude points |
| `max-template-points` | 1000000 | 1 to 1e12 | Points per parameter grid in the template sweep |
| `max-frequency-count` | 1000000 | 1 to 2147483647 | Design frequencies in one set |
| `max-magnitude` | 1e12 | 1 to 1e300 | Largest magnitude the loop-shaping dialog accepts |

**`[search]`**. What the interval search may spend.

| Key | Default | Range | Meaning |
|---|---|---|---|
| `max-live-nodes` | 32000000 | 1 to 1e15 | Live nodes the branch and bound list may hold, a memory budget: 528 bytes per node with two uncertain parameters, 1056 with eight. Every run prints its peak |

**`[defaults.boundary-grid]`**, **`[defaults.templates]`**,
**`[defaults.loop-shaping]`**. What the dialogs are prefilled with. Nothing
here changes a result, since the value is on screen and can be typed over.

| Key | Default | Range | Meaning |
|---|---|---|---|
| `boundary-grid.phase-start`, `phase-end` | -360, 0 | -3600 to 3600 | The phase axis of the Nichols grid, in degrees; it must span at least 360 |
| `boundary-grid.phase-points` | 361 | 2 to 1e6 | Points on the phase axis; 361 over -360..0 is the one-degree grid |
| `boundary-grid.magnitude-start`, `magnitude-end` | -60, 60 | -1000 to 1000 | The magnitude axis, in decibels |
| `boundary-grid.magnitude-points` | 121 | 2 to 1e6 | Points on the magnitude axis |
| `boundary-grid.from-cloud` | 0 | 0 or 1 | Compute the boundaries from the whole template cloud instead of its contour: always safe and more conservative, the cure when a contour comes out wrong |
| `templates.point-count` | 25 | 1 to 1e6 | Points per parameter grid in the template sweep; with n uncertain parameters the sweep evaluates this many to the power of n plants |
| `templates.epsilon-in-nichols` | 1 | 0 or 1 | The plane a new project measures its contour epsilon in: 1 the Nichols plane, 0 the complex plane (the historical reading) |
| `templates.db-per-degree` | 1 | 1e-6 to 1e6 | Decibels that weigh as much as one degree in the Nichols plane |
| `loop-shaping.start`, `end` | 1e-9, 10 | 1e-300 to 1e300 | The frequency range the loop-shaping plot starts with, in rad/s |
| `loop-shaping.point-count` | 100 | 2 to 1e6 | Points over that range |

**`[stability]`**. The frequency grid of the nominal stability check. A
coarser grid is faster and answers "cannot decide" more often; the criterion
itself is not a setting.

| Key | Default | Range | Meaning |
|---|---|---|---|
| `base-grid-points` | 3000 | 10 to 1e7 | Points of the base logarithmic grid |
| `decades-beyond` | 3 | 0 to 20 | Decades sampled beyond the design frequencies, on both sides |
| `max-phase-step-degrees` | 30 | 0.1 to 180 | Phase step above which the grid is refined: the unwrapping tolerance |
| `refinement-budget` | 200000 | 1 to 1e9 | Refinements one verdict may spend before answering "cannot decide" |

**`[algorithms]`**. Figures from the published algorithms. These change what
an algorithm computes, not how long it takes: a value changed here makes the
golden tests and the article validations describe another program.

| Key | Default | Range | Meaning |
|---|---|---|---|
| `template-representatives` | 9 | 2 to 1000 | MR (Rambabu and Nataraj, FDA-10): template points entering the constraint set per design frequency; the paper uses 9 |
| `max-narrowing-passes` | 8 | 1 to 1000 | MR: passes of the HC4 narrowing before a box is accepted as narrowed |
| `mr-nichols-epsilon` | 0 | 0 or 1 | MR: 0 measures the termination epsilon on the parameter box, as the paper does; 1 on the Nichols box, as the other four algorithms do |
| `conservative-boundary-columns` | 1 | 0 or 1 | NT, NK, MC1, MC (thesis), MC2: 1 requires both grid nodes around a phase to allow the point; 0 reads the nearest node, as published, which admits up to half a grid step of violation. See the note below |
| `family-stability-gate` | 1 | 0 or 1 | NT, NK, MC1, MC (thesis), MC2, MC3: 1 closes the loop with every plant of the sweep by the Routh table before a design is returned; 0 reproduces the published algorithms. It acts only when the project records its sweep |
| `point-reading` | columns | columns, exact | MC2: `columns` judges each controller against the boundary columns, as published. `exact` judges it against the specifications over the whole template, reads the best gain at the loop's exact phase, runs the branch and bound until the list is empty, and discards a box only with a proof; the boxes the nominal criterion refuses on its frequency grid alone are counted apart in the strict lower bound. `exact` needs a template at every design frequency |
| `exact-boundary-guide` | published | published, conservative | MC2 under `point-reading = exact`: how the columns that guide the search are read, the nearest node or both nodes around a phase |
| `whole-template-if-no-contour` | 1 | 0 or 1 | Templates: when the contour walk does not close at a frequency, 1 uses the whole template there and 0 stops with an error |
| `alpha-shape-contour` | 0 | 0 or 1 | Templates: 1 extracts the contour as the alpha-shape, which always closes; 0 uses Nordin's walk, as published |
| `border-sweep` | 0 | 0 or 1 | Templates: with exactly two uncertain parameters, 1 sweeps only the border of the parameter box |
| `closed-form-columns` | 0 | 0 or 1 | Boundaries: 1 reads the columns of the five magnitude specifications in closed form (Chait and Yaniv 1993) instead of off the sampled sheet; tracking keeps its sheet |
| `mc.infeasible-magnitude`, `mc.infeasible-phase`, `mc.feasible-magnitude`, `mc.feasible-phase`, `mc.best-gain`, `mc.tree-bisection` | 1 | 0 or 1 | MC (thesis) and MC2: the strategies of chapter 4 of the thesis, one switch each, for measuring what each one buys. They can move the answer |
| `mc.stages` | 1 | 0 or 1 | MC (thesis) only: the execution stages of sec. 4.4 |
| `local-search-budget` | 400 | 1 to 1e7 | NK (Nataraj and Kubal 2007): iterations the local refinement of a candidate may spend |
| `gain-tolerance` | 1.01 | above 1, up to 10 | NK: the ratio at which the gain bisection stops; a pruning bound, not the accuracy of the answer |
| `certified-gain-tolerance` | 1.01 | above 1, up to 10 | MC1 (Martínez-Forte and Cervera 2021): the same ratio for the certified gain search |

Constants of the method, such as 2π, the seven specification slots or the
0 dB ray of the stability criterion, are not settings and are not in the
file.

## The two column readings

The nearest-node reading is the published one, and between two nodes it
admits what the boundary at the point's own phase forbids. The conservative
reading removes that error at any grid, but on the toolbox example 2 NT, NK
and MC1 slow down by about a thousand times on a one-degree grid, and a finer
grid brings that down; there MC2 costs about the same with either reading.
Measured on that example:

| | phase grid | reading | result | total time |
|---|---|---|---|---|
| as published | 361 points (1 degree) | nearest node | k = 557.1, violates by +0.05 dB | about 1 s |
| certified | 1441 or 2881 points (0.25 or 0.125 degrees) | conservative | k = 567.3, meets every specification | 3 to 8 s |

Either way the returned controller is checked against the specifications on
the full templates, and the loop-shaping viewer shows the verdict.
