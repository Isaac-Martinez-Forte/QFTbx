#ifndef QFTBX_BENCH_MEASUREMENT_H
#define QFTBX_BENCH_MEASUREMENT_H

#include <string>

#include "src/bench/plan.h"
#include "src/bench/record.h"

/**
 * @file
 * @brief One case, measured in the calling process.
 *
 * This is what the worker process does: load the project, build the
 * controller structure of the case, apply the plan's settings, run the
 * algorithm and read the clocks, the memory and the counters. The
 * measurements the plan does not ask for are not taken. The caller is
 * expected to be a process of its own, so that the peak memory is the
 * case's and a crash is the case's. The records of a plan go to `records`
 * under the plan's name in the output directory; failureRecord() stands for
 * a case that left none, killed on its timeout or dead of a crash.
 */
namespace qftbx::bench {

Record runCase(const Plan & plan, const Case & c);

std::string recordsDirectory(const Plan & plan);
std::string recordPath(const Plan & plan, const Case & c);

Record failureRecord(const Plan & plan, const Case & c, const std::string & status, const std::string & message);

}

#endif
