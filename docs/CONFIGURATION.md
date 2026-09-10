# The settings file

QFTbx reads an optional settings file, `qftbx.conf`. Every value in it has
a compiled default, and the file only says what to change: a key left out
keeps its default, and with no file at all the program runs exactly as
shipped. Nothing in it is required.

The complete, commented reference is [`qftbx.conf.example`](../qftbx.conf.example)
at the root of the repository; copy it, uncomment what you want to change,
and put it in one of the places below.

## Where it is read from

The first one that exists wins:

1. the path in the `QFTBX_CONFIG` environment variable, which is how a
   variant is tried on a shared machine without touching anyone's home
   directory. Naming a file that cannot be read is an error, because naming
   it says it is meant to be used;
2. `./qftbx.conf`, next to wherever the program was started;
3. `$HOME/.config/qftbx/qftbx.conf`.

## Syntax

An INI file. Comments start with `#` or `;` and run to the end of the line.
Sections group the keys, and a key is its whole path: `max-grid-cells` under
`[limits]` is `limits.max-grid-cells`, and the same name under another
section is a different setting.

Every value but the interface language is a number, and every setting has a stated range. A value that
is not a number, or lies outside its range, stops the program with a
message naming the key and the line; it does not quietly become zero or the
nearest bound.

Only the application reads the file. The test suite builds its own settings,
so no value here can change what a test means.

## The sections

**`[interface]`**: the interface, and the one setting whose value is a text.

| Key | Default | Values | Meaning |
|---|---|---|---|
| `language` | `system` | `system` or a language code (`en`, `es`, ...) | The language the interface starts in. Choosing a language in the View menu writes it here, into the settings file in use (the user's own, `$HOME/.config/qftbx/qftbx.conf`, when the application read none). The codes are those of the translations compiled in; see `src/gui/translations/README.md` |

**`[limits]`**: ceilings that exist to stop a typo, not to express a limit of
the method. They only ever refuse input, so moving them changes no computed
result.

| Key | Default | Range | Meaning |
|---|---|---|---|
| `max-grid-cells` | 10000000 | 4 to 1e15 | Cells of the Nichols grid for the boundaries (phase points × magnitude points) |
| `max-template-points` | 1000000 | 1 to 1e12 | Points per parameter grid in the template sweep |
| `max-frequency-count` | 1000000 | 1 to 2147483647 | Design frequencies in one set |
| `max-magnitude` | 1e12 | 1 to 1e300 | Largest magnitude accepted in the loop-shaping dialog's fields |

**`[search]`**: what the interval search may spend.

| Key | Default | Range | Meaning |
|---|---|---|---|
| `max-live-nodes` | 32000000 | 1 to 1e15 | Live nodes the branch and bound list may hold before it refuses to grow. A memory budget, not a time one: a node measures 528 bytes with two uncertain parameters and 1056 with eight. Every run prints its "peak live nodes", which is the number to size this from |

**`[defaults.boundary-grid]`**, **`[defaults.templates]`**,
**`[defaults.loop-shaping]`**: what the dialogs are prefilled with. Nothing
here changes a computed result, since the value is shown and can be typed
over; it is the group that shows most in daily use.

| Key | Default | Range | Meaning |
|---|---|---|---|
| `boundary-grid.phase-start`, `phase-end` | -360, 0 | -3600 to 3600 | The phase axis of the Nichols grid, in degrees; it must span at least 360 degrees or the boundary union cannot close |
| `boundary-grid.phase-points` | 361 | 2 to 1e6 | Points on the phase axis; 361 over -360..0 is the classic one-degree grid |
| `boundary-grid.magnitude-start`, `magnitude-end` | -60, 60 | -1000 to 1000 | The magnitude axis, in decibels |
| `boundary-grid.magnitude-points` | 121 | 2 to 1e6 | Points on the magnitude axis |
| `templates.point-count` | 25 | 1 to 1e6 | Points per parameter grid in the template sweep. Ten leaves gaps of up to a fifth of a template; twenty-five keeps them under a few per cent. It multiplies: with n uncertain parameters the sweep evaluates this many to the power of n plants, so lower it for many parameters |
| `loop-shaping.start`, `end` | 1e-9, 10 | 1e-300 to 1e300 | The frequency range the loop-shaping plot starts with, in rad/s |
| `loop-shaping.point-count` | 100 | 2 to 1e6 | Points over that range |

**`[stability]`**: the resolution of the nominal stability check. These trade
time against how reliably the check decides; the criterion itself, the
Cohen-Chait-Yaniv crossing count and its 0 dB ray, is the method and not a
setting. Too coarse a grid misses a fast phase turn, the checker answers
"cannot decide" and a candidate that may have been good is discarded,
conservatively; too fine a grid makes every one of millions of candidates
cost more.

| Key | Default | Range | Meaning |
|---|---|---|---|
| `base-grid-points` | 3000 | 10 to 1e7 | Points of the base logarithmic frequency grid |
| `decades-beyond` | 3 | 0 to 20 | Decades sampled beyond the design frequencies, on both sides |
| `max-phase-step-degrees` | 30 | 0.1 to 180 | Phase step above which the grid is refined: the unwrapping tolerance |
| `refinement-budget` | 200000 | 1 to 1e9 | Refinements one verdict may spend before answering "cannot decide" |

**`[algorithms]`**: figures from the published algorithms. This is the group
to be careful with, and the only one where that is true: these change what
the algorithm computes, not how long it takes. Every one is a number from a
paper, and a value changed here makes the golden tests and the article
validations stop describing the program that is running. They are here
anyway, because a research toolbox whose published parameters can only be
explored by recompiling is a worse tool.

| Key | Default | Range | Meaning |
|---|---|---|---|
| `template-representatives` | 9 | 2 to 1000 | MR (Rambabu and Nataraj, FDA-10): template points entering the constraint set per design frequency. The paper uses 9; raising it narrows the known excess of the tracking bound only slightly, at a much higher cost |
| `max-narrowing-passes` | 8 | 1 to 1000 | MR: passes of the HC4 narrowing before a box is accepted as narrowed no further |
| `mr-nichols-epsilon` | 0 | 0 or 1 | MR: with 1 the termination epsilon measures the Nichols box of the leading node, as in the other four algorithms, instead of the width of the parameter box the paper uses; the only way to compare the running time of MR with the others' |
| `whole-template-if-no-contour` | 1 | 0 or 1 | Templates: when the contour walk does not close at a frequency, 1 lets the whole template stand in for its contour there (safe, slower, marked in the viewer); 0 stops with an error naming the frequency. The templates dialog offers the same choice |
| `conservative-boundary-columns` | 0 | 0 or 1 | NT, NK, MC1, MC (thesis), MC2: how a phase between two nodes of the boundary grid is read. 0, the published reading: the nearest node. 1, the conservative reading: both bracketing nodes must allow the point or box. See the note below |
| `local-search-budget` | 400 | 1 to 1e7 | NK (Nataraj and Kubal 2007): iterations the local refinement of a candidate may spend |
| `gain-tolerance` | 1.01 | above 1, up to 10 | NK: the ratio at which the gain bisection stops; a pruning bound, not the accuracy of the answer |
| `certified-gain-tolerance` | 1.01 | above 1, up to 10 | MC (Martínez-Forte and Cervera 2021): the same ratio for the certified gain search |

What is not in the file, and will not be: 2π, the two layers of the boundary
union, the seven specification slots, the 0 dB ray of the stability
criterion. Writing those in a file would not configure anything; it would
break the program. A setting is a value with a defensible range.

**The two readings, measured on the toolbox example 2.** The nearest-node
reading is what the published algorithms do, and it is permissive: between two
nodes the boundary can run higher than at the nearer one, so a box up to half
a grid step away is admitted that the boundary at its own phase forbids. With
a 1-degree phase grid the returned controller violates the stability
specification by 0.05 dB; the error halves every time the grid step halves
and never reaches zero (0.005 dB at 0.125 degrees). The conservative reading
removes it at every grid, but its cost depends on the grid: the strip cuts of
the searches need every column of a span to agree, and at 1 degree the two
bracketing columns disagree so often that NT, NK and MC1 take about a thousand
times longer (70 to 130 s instead of 0.05 s). Each halving of the step divides
that by about five: at 0.25 degrees NT takes 2.4 s, at 0.125 degrees 0.5 s,
less than computing the boundaries themselves (3 and 7 s). So there are two
sensible ways to run the loop shaping:

| | phase grid | reading | result on example 2 | total time |
|---|---|---|---|---|
| as published | 361 points (1 degree) | nearest node | k = 557.1, violates by +0.05 dB | about 1 s |
| certified | 1441 or 2881 points (0.25 or 0.125 degrees) | conservative | k = 567.3, satisfies every specification | 3 to 8 s |

The controller returned is checked against the specifications on the full
templates either way, and the loop-shaping viewer shows the verdict.
