#ifndef QFTBX_BENCH_SUMMARY_H
#define QFTBX_BENCH_SUMMARY_H

#include <cstddef>
#include <string>
#include <vector>

#include "src/bench/record.h"

/**
 * @brief The statistics of a plan's records, one row per (structure,
 * algorithm, epsilon), over its repetitions.
 *
 * Warm-up runs and runs that did not end solved are left out of the
 * numbers and counted apart. The median is the figure to quote for a
 * time: it ignores the one repetition the machine was busy in; the mean
 * and the standard deviation say how steady the measurement was, and the
 * coefficient of variation is that steadiness as a fraction. The counters
 * of the algorithm are deterministic and reported once, with a flag when
 * the repetitions did not agree on them or on the result.
 */
namespace qftbx::bench {

struct Spread
{
    std::size_t n = 0;
    double median = 0.0;
    double mean = 0.0;
    double standardDeviation = 0.0;
    double min = 0.0;
    double max = 0.0;
    /// standardDeviation / mean, 0 when the mean is 0.
    double coefficientOfVariation = 0.0;
};

Spread spreadOf(std::vector<double> values);

struct Aggregate
{
    std::size_t stepsApplied = 0;
    std::string structure;
    std::string algorithm;
    double epsilon = 0.0;

    std::size_t runs = 0;
    std::size_t solved = 0;
    std::size_t infeasible = 0;
    std::size_t failed = 0;

    Spread wallMilliseconds;
    Spread cpuMilliseconds;
    /// Peak resident memory of the process, in megabytes.
    Spread peakMemoryMegabytes;
    /// Peak minus the resident size before the algorithm started.
    Spread algorithmMemoryMegabytes;

    LoopShapingStatistics statistics;
    bool countersAgree = true;

    /// The result of the solved runs, when they agree; and whether they do.
    double gain = 0.0;
    std::string digest;
    bool resultsAgree = true;
};

std::vector<Aggregate> summarize(const std::vector<Record> & records);

void writeCsv(const std::vector<Aggregate> & aggregates, const std::string & path);
void writeMarkdown(const std::vector<Aggregate> & aggregates, const std::string & path);
std::string markdownTable(const std::vector<Aggregate> & aggregates);

} // namespace qftbx::bench

#endif // QFTBX_BENCH_SUMMARY_H
