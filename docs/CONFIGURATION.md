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

Every value is a number, and every setting has a stated range. A value that
is not a number, or lies outside its range, stops the program with a
message naming the key and the line; it does not quietly become zero or the
nearest bound.

Only the application reads the file. The test suite builds its own settings,
so no value here can change what a test means.

## The sections

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
| `templates.point-count` | 10 | 1 to 1e6 | Points per parameter grid in the template sweep. It multiplies: with n uncertain parameters the sweep evaluates this many to the power of n plants |
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
| `local-search-budget` | 400 | 1 to 1e7 | NK (Nataraj and Kubal 2007): iterations the local refinement of a candidate may spend |
| `gain-tolerance` | 1.01 | above 1, up to 10 | NK: the ratio at which the gain bisection stops; a pruning bound, not the accuracy of the answer |
| `certified-gain-tolerance` | 1.01 | above 1, up to 10 | MC (Martínez-Forte and Cervera 2021): the same ratio for the certified gain search |

What is not in the file, and will not be: 2π, the two layers of the boundary
union, the seven specification slots, the 0 dB ray of the stability
criterion. Writing those in a file would not configure anything; it would
break the program. A setting is a value with a defensible range.
