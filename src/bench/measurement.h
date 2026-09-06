#ifndef QFTBX_BENCH_MEASUREMENT_H
#define QFTBX_BENCH_MEASUREMENT_H

#include <string>

#include "src/bench/plan.h"
#include "src/bench/record.h"

/**
 * @brief One case, measured in the calling process.
 *
 * This is what the worker process does: load the project, build the
 * controller structure of the case, apply the plan's settings, run the
 * algorithm and read the clocks, the memory and the counters. The
 * measurements the plan does not ask for are not taken. The caller is
 * expected to be a process of its own, so that the peak memory is the
 * case's and a crash is the case's.
 */
namespace qftbx::bench {

Record runCase(const Plan & plan, const Case & c);

/// Where the records of a plan go: <output directory>/<plan name>/records.
std::string recordsDirectory(const Plan & plan);
std::string recordPath(const Plan & plan, const Case & c);

/// A record for a case that produced none: killed on its timeout, or dead
/// of a crash.
Record failureRecord(const Plan & plan, const Case & c, const std::string & status, const std::string & message);

} // namespace qftbx::bench

#endif // QFTBX_BENCH_MEASUREMENT_H
