# Benchmarking the loop-shaping algorithms

QFTbx ships a benchmark tool, `qftbx-bench`, and a planner in the interface
for measuring the loop-shaping algorithms on one problem: the same
project, its templates and boundaries already computed, a sequence of
controller structures of growing order, every algorithm and epsilon you
choose, repeated as many times as the statistics need. A plan can also
compute the templates and the boundaries again, on the processor or on the
GPU, and compare the two ([below](#computing-the-phases-again)). Neither is
part of an ordinary build: configure with `-DQFTBX_BUILD_BENCHMARK=ON` to get them
(see [BUILDING.md](BUILDING.md)).

## How a measurement is made

**Every case is a process.** A case is one run of one algorithm on one
controller structure with one epsilon. The tool launches it as a process of
its own, with OpenMP held to the plan's `threads` (one unless the plan says
otherwise, and the whole machine with 0), so that its peak memory is its own,
a crash or a hang costs one case and not the queue, a time limit can be
enforced by killing it, and several cases can run at once, one per core.
The number of processes at once is the plan's `jobs` (0 means one less than
the machine has cores).

**What is measured** is chosen in the plan, so a measurement cannot disturb
another:

| Measure | What it is | Cost to the run |
|---|---|---|
| `time` | Wall-clock time of the loop shaping, in milliseconds | none: two clock readings |
| `cpu` | User plus system CPU time of the process over the run | none |
| `memory` | Peak resident size of the process at the end, and its resident size just before the algorithm started, so the difference is the algorithm's own | none: read from the operating system at the end |
| `memory-trace` | The resident size sampled every 10 ms from another thread, for a memory-over-time curve | a thread reading `/proc/self/statm`; off by default, do not combine with a timing you care about |
| `counters` | The algorithm's own counts: peak of live nodes, nodes processed, boxes classified, stability verdicts and profiles | none: the algorithms keep them anyway |

Every record also carries the result (gain, zeros, poles and a hash of them),
the status (`solved`, `infeasible`, `computed` for phases without a search,
`error`, `timeout`, `crashed`, `cancelled`), and the environment: host, operating system, compiler, git
commit, interval backend, whether the build is tuned for the machine, the
number of cores, the load average when the case started, and the time.

**Repetitions.** A case is repeated `count` times, plus one warm-up run when
`warm-up` is set; the warm-up is recorded but left out of the statistics,
since the first run pays for what the operating system caches. The summary
gives, per case, the median, mean, standard deviation, minimum, maximum and
coefficient of variation of the wall time, the CPU time and the memory. The
figure to quote is the median: it ignores the repetition the machine was
busy in. The coefficient of variation says how steady the measurement was;
above a few per cent, the machine was not idle, and the run is worth
repeating. The results and the counters of an algorithm are deterministic,
so the repetitions must agree on them, and the summary flags a case where
they do not.

**Comparability.** The other four algorithms stop on the Nichols box of the
leading node; MR stops on the width of the parameter box, as its paper does,
so its epsilon is another quantity. The setting `algorithms.mr-nichols-epsilon`
makes MR stop on the Nichols box too ([CONFIGURATION.md](CONFIGURATION.md)),
which a plan can set for the comparison; on the thesis benchmarks MR's search
is very slow under that criterion.

## The plan

A plan is an XML file. `qftbx-bench example` prints one to start from:

```xml
<QFTbench version="1" name="ex2">
    <project file="qft_toolbox_ex2.qft"/>
    <output directory="results"/>
    <execution jobs="0" timeout-seconds="3600" memory-limit-megabytes="0"/>
    <measure time="1" cpu="1" memory="1" memory-trace="0" counters="1"/>
    <repetitions count="5" warm-up="1"/>
    <algorithms>
        <algorithm name="nt"/> <algorithm name="nk"/> <algorithm name="mc1"/> <algorithm name="mc_thesis"/>
    </algorithms>
    <epsilons> <epsilon value="2"/> </epsilons>
    <structures run-base="1">
        <gain min="1e-6" max="1e6"/>
        <step add="zero" min="0.01" max="1000" run="1"/>
        <step add="pole" min="0.01" max="1000" run="1"/>
    </structures>
    <settings>
        <setting key="stability.base-grid-points" value="3000"/>
    </settings>
</QFTbench>
```

- `project` is a `.qft` with its templates and boundaries computed; paths are
  relative to the plan file.
- `structures` is the thesis' way of measuring: the project's controller
  structure is the base, each `step` adds a zero or a pole with its search
  range, and the cases run after every step whose `run` is 1 (and on the
  base when `run-base` is 1). `gain`, optional, replaces the base's gain
  range. The added parameters are named `z2`, `z3`, ... and `p2`, `p3`, ...
  after the ones the base has. Zero-pole-gain structures only.
- `settings` are overrides of the settings file, by dotted key
  ([CONFIGURATION.md](CONFIGURATION.md)); the same reader applies, with its
  ranges.
- `memory-limit-megabytes` caps the address space of each case; a case that
  exceeds it fails instead of taking the machine down.
- `threads` in `execution` is OpenMP's thread count in each case: 1 when it
  is absent, as the search is measured, and 0 for the whole machine, as the
  program computes the templates and the boundaries. A plan that times them
  wants `threads="0"` and `jobs="1"`, so that one case at a time has the
  machine.

### Computing the phases again

A plan may compute the templates, the boundaries or both before the search,
on the processor, on the GPU, or on both to compare them:

```xml
<phases templates="1" boundaries="1" loop-shaping="0" processors="cpu gpu" compare="1">
    <templates sweep-points="0" frequencies="0" metric="project" epsilon="project"/>
    <boundaries source="contour" phase-points="0" magnitude-points="0"/>
</phases>
```

- `processors` lists where to compute them, `cpu`, `gpu` or both; the cases
  of each are their own, with their ids starting `cpu-` or `gpu-`.
- `compare="1"` adds one case, `compare-...`, that computes the phases on the
  processor and then on the GPU, with the search after each when the plan
  searches, and records how far apart they came out (below).
- `loop-shaping="0"` stops after the phases: the plan then needs no
  algorithm and no epsilon, and its cases end `computed`.
- `templates`: `sweep-points` regrids every uncertain parameter with that
  many points over its span, `frequencies` resamples the design frequencies
  log-spaced over theirs (and then the epsilon is proposed), `metric` is
  `project`, `complex` or `nichols` (with `db-per-degree`), and `epsilon` is
  the project's or `propose`. Zero keeps the project's.
- `boundaries`: `source` is the `contour` or the whole `cloud`, and
  `phase-points` and `magnitude-points` change the project's Nichols grid
  (or the settings' default for a project without boundaries).
- The contour method, the border sweep, the whole cloud for a contour that
  does not close and the closed-form columns are settings
  (`algorithms.alpha-shape-contour`, `algorithms.border-sweep`,
  `algorithms.whole-template-if-no-contour`,
  `algorithms.closed-form-columns`), set in `settings` as any other.

## Running

    qftbx-bench cases plan.xml           # what the plan expands to
    qftbx-bench run plan.xml --jobs 4    # run it, four cases at a time
    qftbx-bench summarize plan.xml       # rebuild the summaries from the records

`run` prints each case as it finishes and, at the end, the summary table. It
leaves under the output directory:

- `<name>/records/<case>.json`: one document per run, written by the process
  that ran it. A run that was killed or crashed gets a record written by the
  runner, with its status. The records are the raw data; nothing is ever
  deleted from them.
- `<name>.jsonl`: every record, one per line, for pandas, R or a script.
- `<name>-summary.csv` and `<name>-summary.md`: one row per (structure,
  algorithm, epsilon) with the statistics above.

A run left to itself on another machine:

    nohup qftbx-bench run plan.xml > run.log 2>&1 &

Since every finished case is already on disk, a run interrupted halfway keeps
what it measured; `summarize` gathers it.

## The planner

With the same build option the application gains a Tools menu with the
benchmark planner: the plan, the queue and the results in one window.

- **Plan.** The form of everything above: the project (with a summary of
  what it holds, so a project without boundaries is noticed before the run),
  the plan name and output directory, the structure sequence as a table of
  steps to add and reorder with the structures that will run listed under
  it, the algorithms, the epsilons, the repetitions and the warm-up, the
  measures with a word on the cost of each, the execution limits, and the
  settings overrides with the known keys offered as you type. The status bar
  counts the cases the plan expands to as you edit.
- **Queue.** One row per case with its state, time, gain and memory as the
  processes finish, a progress bar and a log. Stop kills the running cases
  and keeps what finished.
- **Results.** The summary table and a figure of one measure (wall or CPU
  time, memory, live nodes) against the controller structure, one curve per
  algorithm, logarithmic by default, exportable as CSV or Markdown.

The planner saves the plan to its file before it runs, since the worker
processes read it from there, and it launches `qftbx-bench` from next to the
application. "Load results" reads the records already on disk for the
current plan, so a run made unattended elsewhere with the same plan file is
read here.

## Reading the results

The `.jsonl` is the source of truth; the summaries are derived from it. The
columns of the CSV are the fields of the summary table: the status counts,
the spread of the wall time (median, mean, standard deviation, minimum,
maximum, coefficient of variation), of the CPU time and of the memory (peak,
and the algorithm's own), the counters, and the agreement flags.

Peak memory is the whole process, including the project loaded from the
file; `algorithm_memory_mb` is the growth during the run, which for a
branch and bound is its live list, so the peak of live nodes and the memory
should move together. A wall time well above the CPU time means the machine
was shared.

A case with phases also keeps, under `phases` in its record:
- its processor;
- the time of the templates and of the boundaries;
- the time of every step the engines write to the program's record, under
  its stage and detail (`templates/sweep`, `contours/walk`,
  `boundaries/sheets (OpenMP)`, `boundaries/union`…; on the GPU
  `templates/sweep (GPU)`, `boundaries/sheets (GPU)` and
  `boundaries/traces (OpenMP)`, the part still read on the processor);
- what the card's first use cost, apart;
- the size of what it computed: frequencies, plants, contour points and
  cells of the Nichols grid.

The environment of a case on the GPU describes the card. The summary gives
one row per processor, with the median and the variation of the two
phases, and leaves the comparisons out. A comparison keeps, frequency by
frequency, what differs:

| Field | What it counts |
|---|---|
| `cloud_sizes_differ`, `cloud_largest_relative` | templates whose size differs, and the largest distance between plants of the same index, relative to the plant |
| `contour_points_differ`, `contour_largest_hausdorff` | points in one contour and not the other, and the largest Hausdorff distance between them over the frequency's epsilon |
| `reports_differ`, `epsilons_differ` | contour reports and epsilons that differ |
| `columns_differ`, `column_largest_db` | columns whose allowed intervals differ in number or in an infinite end, and the largest gap between finite ends, in dB |
| `trace_sets_differ`, `trace_largest` | specifications whose traces differ in shape (how many traces, how many points), and the largest gap between points that match, in degrees or dB |
| `unions_differ`, `union_largest` | the same for the union of each frequency |
| `cpu_status`, `gpu_status`, `gain_relative`, `same_design` | when it searches: how each search ended, the relative gap between the gains and whether the designs agree to the bit |
