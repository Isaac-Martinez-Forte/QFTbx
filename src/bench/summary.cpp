#include "src/bench/summary.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <tuple>

#include "src/core/common/exception.h"

namespace qftbx::bench {

Spread spreadOf(std::vector<double> values)
{
    Spread spread;
    spread.n = values.size();
    if (values.empty()) {
        return spread;
    }
    std::sort(values.begin(), values.end());
    const std::size_t n = values.size();
    spread.min = values.front();
    spread.max = values.back();
    spread.median = n % 2 == 1 ? values[n / 2] : (values[n / 2 - 1] + values[n / 2]) / 2.0;

    double sum = 0.0;
    for (const double v : values) {
        sum += v;
    }
    spread.mean = sum / static_cast<double>(n);

    if (n > 1) {
        double squares = 0.0;
        for (const double v : values) {
            squares += (v - spread.mean) * (v - spread.mean);
        }
        spread.standardDeviation = std::sqrt(squares / static_cast<double>(n - 1));
    }
    spread.coefficientOfVariation = spread.mean != 0.0 ? spread.standardDeviation / spread.mean : 0.0;
    return spread;
}

std::vector<Aggregate> summarize(const std::vector<Record> & records)
{
    using Key = std::tuple<std::size_t, std::string, double>;
    std::map<Key, std::vector<const Record *>> groups;
    for (const Record & record : records) {
        groups[Key(record.stepsApplied, record.algorithm, record.epsilon)].push_back(&record);
    }

    std::vector<Aggregate> aggregates;
    for (const auto & [key, group] : groups) {
        Aggregate a;
        a.stepsApplied = std::get<0>(key);
        a.algorithm = std::get<1>(key);
        a.epsilon = std::get<2>(key);
        a.structure = group.front()->structure;

        std::vector<double> wall, cpu, peak, own;
        bool first = true;
        for (const Record * r : group) {
            if (r->warmUp) {
                continue;
            }
            ++a.runs;
            if (r->status == "infeasible") {
                ++a.infeasible;
                continue;
            }
            if (r->status != "solved") {
                ++a.failed;
                continue;
            }
            ++a.solved;
            wall.push_back(r->wallMilliseconds);
            cpu.push_back(r->cpuMilliseconds);
            peak.push_back(r->peakMemoryBytes / (1024.0 * 1024.0));
            own.push_back(r->peakMemoryBytes > r->baselineMemoryBytes
                          ? (r->peakMemoryBytes - r->baselineMemoryBytes) / (1024.0 * 1024.0) : 0.0);
            if (first) {
                a.statistics = r->statistics;
                a.gain = r->gain;
                a.digest = r->digest;
                first = false;
            } else {
                if (r->digest != a.digest) {
                    a.resultsAgree = false;
                }
                const LoopShapingStatistics & s = r->statistics;
                if (s.peakLiveNodes != a.statistics.peakLiveNodes || s.nodesProcessed != a.statistics.nodesProcessed
                        || s.boxesClassified != a.statistics.boxesClassified
                        || s.boxesFeasible != a.statistics.boxesFeasible
                        || s.boxesInfeasible != a.statistics.boxesInfeasible
                        || s.boxesAmbiguous != a.statistics.boxesAmbiguous
                        || s.stabilityVerdicts != a.statistics.stabilityVerdicts) {
                    a.countersAgree = false;
                }
            }
        }
        a.wallMilliseconds = spreadOf(wall);
        a.cpuMilliseconds = spreadOf(cpu);
        a.peakMemoryMegabytes = spreadOf(peak);
        a.algorithmMemoryMegabytes = spreadOf(own);
        aggregates.push_back(a);
    }
    return aggregates;
}

namespace {

std::string number(double value, int precision = 3)
{
    std::ostringstream out;
    out << std::fixed << std::setprecision(precision) << value;
    return out.str();
}

} // namespace

void writeCsv(const std::vector<Aggregate> & aggregates, const std::string & path)
{
    std::ofstream out(path);
    if (!out) {
        throw FileError(path + ": cannot write the summary");
    }
    out << "steps,structure,algorithm,epsilon,runs,solved,infeasible,failed,"
           "wall_ms_median,wall_ms_mean,wall_ms_std,wall_ms_min,wall_ms_max,wall_ms_cv,"
           "cpu_ms_median,cpu_ms_mean,cpu_ms_std,"
           "peak_memory_mb_median,peak_memory_mb_max,algorithm_memory_mb_median,"
           "peak_live_nodes,nodes_processed,boxes_classified,boxes_feasible,boxes_infeasible,boxes_ambiguous,stability_verdicts,stability_profiles,"
           "counters_agree,gain,digest,results_agree\n";
    out.precision(17);
    for (const Aggregate & a : aggregates) {
        out << a.stepsApplied << "," << a.structure << "," << a.algorithm << "," << a.epsilon << ","
            << a.runs << "," << a.solved << "," << a.infeasible << "," << a.failed << ","
            << a.wallMilliseconds.median << "," << a.wallMilliseconds.mean << "," << a.wallMilliseconds.standardDeviation << ","
            << a.wallMilliseconds.min << "," << a.wallMilliseconds.max << "," << a.wallMilliseconds.coefficientOfVariation << ","
            << a.cpuMilliseconds.median << "," << a.cpuMilliseconds.mean << "," << a.cpuMilliseconds.standardDeviation << ","
            << a.peakMemoryMegabytes.median << "," << a.peakMemoryMegabytes.max << "," << a.algorithmMemoryMegabytes.median << ","
            << a.statistics.peakLiveNodes << "," << a.statistics.nodesProcessed << "," << a.statistics.boxesClassified << ","
            << a.statistics.boxesFeasible << "," << a.statistics.boxesInfeasible << "," << a.statistics.boxesAmbiguous << ","
            << a.statistics.stabilityVerdicts << "," << a.statistics.stabilityProfiles << ","
            << (a.countersAgree ? 1 : 0) << "," << a.gain << "," << a.digest << "," << (a.resultsAgree ? 1 : 0) << "\n";
    }
}

std::string markdownTable(const std::vector<Aggregate> & aggregates)
{
    std::ostringstream out;
    out << "| Structure | Algorithm | Epsilon | Runs | Wall median (s) | Wall CV | CPU median (s) | Peak memory (MB) | Peak live nodes | k | Agree |\n";
    out << "|---|---|---|---|---|---|---|---|---|---|---|\n";
    for (const Aggregate & a : aggregates) {
        out << "| " << a.structure << " | " << a.algorithm << " | " << a.epsilon << " | "
            << a.solved << "/" << a.runs;
        if (a.infeasible > 0) {
            out << " (" << a.infeasible << " infeasible)";
        }
        if (a.failed > 0) {
            out << " (" << a.failed << " failed)";
        }
        out << " | " << number(a.wallMilliseconds.median / 1000.0) << " | "
            << number(100.0 * a.wallMilliseconds.coefficientOfVariation, 1) << " % | "
            << number(a.cpuMilliseconds.median / 1000.0) << " | "
            << number(a.peakMemoryMegabytes.median, 1) << " | "
            << a.statistics.peakLiveNodes << " | "
            << number(a.gain, 4) << " | "
            << (a.resultsAgree && a.countersAgree ? "yes" : "NO") << " |\n";
    }
    return out.str();
}

void writeMarkdown(const std::vector<Aggregate> & aggregates, const std::string & path)
{
    std::ofstream out(path);
    if (!out) {
        throw FileError(path + ": cannot write the summary");
    }
    out << markdownTable(aggregates);
}

} // namespace qftbx::bench
