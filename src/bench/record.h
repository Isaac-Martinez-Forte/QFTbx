/**
 * @file
 * @brief What one measured run leaves behind: one JSON document per run,
 * written by the process that ran it, and the environment it ran in.
 *
 * The environment names the machine, the compiler, the commit and the
 * interval backend, the one-minute load average when the run started (-1
 * when unknown) and the time, ISO 8601 in local time. A record names its
 * case, its status (solved, infeasible, error, timeout or crashed) with the
 * message, the wall and CPU time, and in bytes the peak resident size of the
 * process and its resident size just before the algorithm started, so that
 * the difference is the algorithm's own, with the trace of the resident size
 * over the run, milliseconds since the start against bytes, when traced. It
 * keeps the statistics and the certificate of the run, the controller, and
 * the verifier's verdict on it over the full template: the largest excess
 * over any active bound in dB, positive when it violates and not-a-number
 * when the run made no check; the closed loop with every plant of the sweep,
 * how many plants, how many of them unstable and the largest real part of a
 * closed-loop pole, the members zero when the family was not checked; and
 * the nominal closed loop, whether it was checked, whether it is stable and
 * its largest real part. The digest is a hash of the result's numbers on
 * which repetitions must agree, FNV-1a over the bytes of the doubles, gain
 * then zeros then poles, the same the benchmark driver has always printed.
 * readRecords reads every *.json record of a directory in file-name order,
 * and writeJsonLines writes one record per line.
 */

#ifndef QFTBX_BENCH_RECORD_H
#define QFTBX_BENCH_RECORD_H

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include <QJsonObject>

#include "src/bench/plan.h"
#include "src/core/loopshaping/loop_shaping_statistics.h"

namespace qftbx::bench {

struct Environment
{
    std::string hostname;
    std::string operatingSystem;
    std::string compiler;
    std::string gitCommit;
    std::string intervalBackend;
    bool nativeArchitecture = false;
    int cores = 0;
    double loadAverage = -1.0;
    std::string timestamp;
};

Environment describeEnvironment();

struct Record
{
    std::string caseId;
    std::size_t caseIndex = 0;
    std::size_t stepsApplied = 0;
    std::string structure;
    std::string algorithm;
    double epsilon = 0.0;
    int repetition = 0;
    bool warmUp = false;

    std::string status;
    std::string message;

    double wallMilliseconds = 0.0;
    double cpuMilliseconds = 0.0;
    std::uint64_t peakMemoryBytes = 0;
    std::uint64_t baselineMemoryBytes = 0;
    std::vector<std::pair<double, std::uint64_t>> memoryTrace;

    LoopShapingStatistics statistics;

    double gain = 0.0;
    std::vector<double> zeros;
    std::vector<double> poles;
    double worstExcessDb = std::numeric_limits<double>::quiet_NaN();
    std::size_t familyMembers = 0;
    std::size_t familyUnstable = 0;
    double familyWorstRealPart = std::numeric_limits<double>::quiet_NaN();
    bool nominalChecked = false;
    bool nominalStable = false;
    double nominalWorstRealPart = std::numeric_limits<double>::quiet_NaN();
    std::string digest;

    Environment environment;
};

QJsonObject toJson(const Record & record);
Record recordFromJson(const QJsonObject & object);

void writeRecord(const Record & record, const std::string & path);
Record readRecord(const std::string & path);

std::vector<Record> readRecords(const std::string & directory);

void writeJsonLines(const std::vector<Record> & records, const std::string & path);

std::string digestOf(double gain, const std::vector<double> & zeros, const std::vector<double> & poles);

}

#endif
