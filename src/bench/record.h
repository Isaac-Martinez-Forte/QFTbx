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

/**
 * @brief What one measured run leaves behind: one JSON document per run,
 * written by the process that ran it, and the environment it ran in.
 */
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
    /// The one-minute load average when the run started, -1 when unknown.
    double loadAverage = -1.0;
    /// ISO 8601, local time.
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

    /// solved, infeasible, error, timeout, crashed.
    std::string status;
    std::string message;

    double wallMilliseconds = 0.0;
    double cpuMilliseconds = 0.0;
    /// Bytes: the peak resident size of the process, and its resident size
    /// just before the algorithm started, so the difference is the
    /// algorithm's own.
    std::uint64_t peakMemoryBytes = 0;
    std::uint64_t baselineMemoryBytes = 0;
    /// (milliseconds since the start, resident bytes), when traced.
    std::vector<std::pair<double, std::uint64_t>> memoryTrace;

    LoopShapingStatistics statistics;

    double gain = 0.0;
    std::vector<double> zeros;
    std::vector<double> poles;
    /// The controller checked against the specifications over the full
    /// template: the largest excess over any active bound, in dB, positive
    /// when it violates. NaN when the run made no check.
    double worstExcessDb = std::numeric_limits<double>::quiet_NaN();
    /// A hash of the result's numbers: repetitions must agree on it.
    std::string digest;

    Environment environment;
};

QJsonObject toJson(const Record & record);
Record recordFromJson(const QJsonObject & object);

void writeRecord(const Record & record, const std::string & path);
Record readRecord(const std::string & path);

/// Every *.json record in a directory, in file-name order.
std::vector<Record> readRecords(const std::string & directory);

/// One record per line.
void writeJsonLines(const std::vector<Record> & records, const std::string & path);

/// The hash of a result, the same the benchmark driver has always printed:
/// FNV-1a over the bytes of the doubles, gain then zeros then poles.
std::string digestOf(double gain, const std::vector<double> & zeros, const std::vector<double> & poles);

} // namespace qftbx::bench

#endif // QFTBX_BENCH_RECORD_H
