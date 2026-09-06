# Benchmarking the loop-shaping algorithms

QFTbx ships a benchmark tool, `qftbx-bench`, and a planner in the interface
for measuring the five loop-shaping algorithms on one problem: the same
project, its templates and boundaries already computed, a sequence of
controller structures of growing order, every algorithm and epsilon you
choose, repeated as many times as the statistics need. Neither is part of
an ordinary build: configure with `-DQFTBX_BUILD_BENCHMARK=ON` to get them
(see [BUILDING.md](BUILDING.md)).

## How a measurement is made

**Every case is a process.** A case is one run of one algorithm on one
controller structure with one epsilon. The tool launches it as a process of
its own, with OpenMP held to one thread, so that its peak memory is its own,
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
the status (`solved`, `infeasible`, `error`, `timeout`, `crashed`,
`cancelled`), and the environment: host, operating system, compiler, git
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
