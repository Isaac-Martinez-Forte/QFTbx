#ifndef QFTBX_BENCH_RUNNER_H
#define QFTBX_BENCH_RUNNER_H

#include <atomic>
#include <cstddef>
#include <functional>
#include <string>

#include "src/bench/plan.h"
#include "src/bench/record.h"

/**
 * @brief Runs the cases of a plan, each in a process of its own, several
 * at a time.
 *
 * The runner launches the worker program (the benchmark tool itself, with
 * the "case" command) once per case with OMP_NUM_THREADS=1, keeps as many
 * running as the plan's jobs say, kills the ones that exceed the timeout
 * and writes a record for every case that left none behind. When every
 * case is done it gathers the records into one JSON Lines file and the
 * summary tables next to it.
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
        /// For Finished: the record the case left.
        const Record * record = nullptr;
        std::size_t done = 0;
        std::size_t total = 0;
        std::size_t running = 0;
        std::string text;
    };
    using Listener = std::function<void(const Event &)>;

    /// @param workerProgram the executable that runs one case:
    ///        <program> case <plan file> <case index>.
    /// @param jobs processes at once; 0 takes the plan's value.
    /// @return the number of cases that did not end solved or infeasible.
    std::size_t run(const Plan & plan, const std::string & planPath, const std::string & workerProgram,
                    int jobs, const Listener & listener);

    /// Asks the run to stop: no new case starts, the running ones are
    /// killed. Callable from another thread.
    void cancel() { m_cancel.store(true); }

private:
    std::atomic<bool> m_cancel{false};
};

/// Gathers the records of a plan into <output>/<name>.jsonl and writes the
/// summary tables <output>/<name>-summary.csv and .md.
void gather(const Plan & plan);

} // namespace qftbx::bench

#endif // QFTBX_BENCH_RUNNER_H
