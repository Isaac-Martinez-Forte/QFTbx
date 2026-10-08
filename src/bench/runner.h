#ifndef QFTBX_BENCH_RUNNER_H
#define QFTBX_BENCH_RUNNER_H

#include <atomic>
#include <cstddef>
#include <functional>
#include <string>

#include "src/bench/plan.h"
#include "src/bench/record.h"

/**
 * @file
 * @brief Runs the cases of a plan, each in a process of its own, several
 * at a time.
 *
 * The runner launches the worker program (the benchmark tool itself, with
 * the "case" command) once per case with OMP_NUM_THREADS=1, keeps as many
 * running as the plan's jobs say, kills the ones that exceed the timeout
 * and writes a record for every case that left none behind. When every
 * case is done it gathers the records into one JSON Lines file and the
 * summary tables next to it: `output/name.jsonl`, `name-summary.csv` and
 * `.md`.
 *
 * run() calls the worker as PROGRAM case PLAN-FILE CASE-INDEX, takes the
 * plan's jobs when given 0, and returns how many cases did not end solved
 * or infeasible; a Finished event carries the record the case left.
 * cancel() may be called from another thread: no new case starts and the
 * running ones are killed.
 */
namespace qftbx::bench {

class Runner
{
public:
    struct Event
    {
        enum class Kind { Started, Finished, Message };
        Kind kind = Kind::Message;
        const Case * c = nullptr;
        const Record * record = nullptr;
        std::size_t done = 0;
        std::size_t total = 0;
        std::size_t running = 0;
        std::string text;
    };
    using Listener = std::function<void(const Event &)>;

    std::size_t run(const Plan & plan, const std::string & planPath, const std::string & workerProgram,
                    int jobs, const Listener & listener);

    void cancel() { m_cancel.store(true); }

private:
    std::atomic<bool> m_cancel{false};
};

void gather(const Plan & plan);

}

#endif
