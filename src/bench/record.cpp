/**
 * @file
 * @brief JSON encoding of benchmark records and the environment they ran in.
 *
 * The environment names the host, the operating system, the compiler, the
 * git commit compiled in, the interval backend and the load average at the
 * time. A count absent from a record reads as zero; a lower bound that is
 * missing reads as infinity, the bound of a search that left nothing
 * unproven; a worst excess or a worst real part of the family or of the
 * nominal loop that is missing reads as not-a-number, a nominal verdict that
 * is missing as not checked, and a missing load average as -1. The
 * counts of the certificate are written and read from one table, in the
 * order of the certification funnel. The digest of a design is an FNV-1a
 * hash over the exact bits of the gain, the zeros and the poles, so two runs
 * agree only when their results are identical to the last bit. The phases,
 * the card and the comparison are written only when the case has them, and
 * read back as not run when absent.
 */

#include "src/bench/record.h"

#include <cmath>
#include <limits>

#include <cstring>
#include <sstream>

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSysInfo>
#include <QThread>

#ifndef _WIN32
#include <cstdlib>
#endif

#include "src/core/common/exception.h"
#include "src/core/pipeline/gpu_provider.h"

#ifndef QFTBX_GIT_COMMIT
#define QFTBX_GIT_COMMIT "unknown"
#endif

namespace qftbx::bench {

Environment describeEnvironment()
{
    Environment environment;
    environment.hostname = QSysInfo::machineHostName().toStdString();
    environment.operatingSystem = QSysInfo::prettyProductName().toStdString();
#if defined(__clang__)
    environment.compiler = std::string("clang ") + __clang_version__;
#elif defined(__GNUC__)
    environment.compiler = std::string("gcc ") + __VERSION__;
#else
    environment.compiler = "unknown";
#endif
    environment.gitCommit = QFTBX_GIT_COMMIT;
#if defined(QFTBX_INTERVAL_CXSC)
    environment.intervalBackend = "cxsc";
#else
    environment.intervalBackend = "kv";
#endif
#if defined(QFTBX_NATIVE_ARCH)
    environment.nativeArchitecture = true;
#endif
    environment.cores = QThread::idealThreadCount();
#ifndef _WIN32
    double loads[1];
    if (getloadavg(loads, 1) == 1) {
        environment.loadAverage = loads[0];
    }
#endif
    environment.timestamp = QDateTime::currentDateTime().toString(Qt::ISODate).toStdString();
    return environment;
}

void describeGpu(Environment & environment)
{
    const GpuProvider * provider = gpuProvider();
    if (provider == nullptr) {
        environment.gpuState = "no GPU support";
        return;
    }
    const GpuStatus & status = provider->status();
    environment.gpuState = status.usable() ? "usable" : status.reason().rendered();
    environment.gpuName = status.name;
    environment.gpuCompute = std::to_string(status.computeMajor) + "." + std::to_string(status.computeMinor);
    environment.gpuMemoryBytes = status.memory;
    environment.gpuMultiprocessors = status.multiprocessors;
    environment.gpuDoublePrecisionRatio = status.doublePrecisionRatio;
    environment.gpuDriverVersion = status.driverVersion;
    environment.gpuRuntimeVersion = status.runtimeVersion;
}

namespace {

QJsonArray toArray(const std::vector<double> & values)
{
    QJsonArray array;
    for (const double value : values) {
        array.append(value);
    }
    return array;
}

std::vector<double> fromArray(const QJsonArray & array)
{
    std::vector<double> values;
    for (const QJsonValue & value : array) {
        values.push_back(value.toDouble());
    }
    return values;
}

struct CertificateCount {
    const char * key;
    std::size_t LoopShapingStatistics::Certificate::* member;
};

const CertificateCount kCertificateCounts[] = {
    {"residue_nodes", &LoopShapingStatistics::Certificate::residueNodes},
    {"epsilon_resolved", &LoopShapingStatistics::Certificate::epsilonResolved},
    {"unproven_discards", &LoopShapingStatistics::Certificate::unprovenDiscards},
    {"grid_backed_prunes", &LoopShapingStatistics::Certificate::gridBackedPrunes},
    {"family_prunes", &LoopShapingStatistics::Certificate::familyPrunes},
    {"nominal_box_prunes", &LoopShapingStatistics::Certificate::nominalBoxPrunes},
    {"contracted_boxes", &LoopShapingStatistics::Certificate::contractedBoxes},
    {"emptied_by_specifications", &LoopShapingStatistics::Certificate::emptiedBySpecifications},
    {"emptied_by_stability", &LoopShapingStatistics::Certificate::emptiedByStability},
    {"emptied_by_zero_exclusion", &LoopShapingStatistics::Certificate::emptiedByZeroExclusion},
    {"nominal_axis_prunes", &LoopShapingStatistics::Certificate::nominalAxisPrunes},
    {"grid_prunes_kept", &LoopShapingStatistics::Certificate::gridPrunesKept},
    {"proven_infeasible", &LoopShapingStatistics::Certificate::provenInfeasible},
    {"columns_overruled", &LoopShapingStatistics::Certificate::columnsOverruled},
    {"certified_cuts", &LoopShapingStatistics::Certificate::certifiedCuts},
    {"sector_verdicts", &LoopShapingStatistics::Certificate::sectorVerdicts},
    {"certifications", &LoopShapingStatistics::Certificate::certifications},
    {"refused_by_routh", &LoopShapingStatistics::Certificate::refusedByRouth},
    {"refused_by_nominal_routh", &LoopShapingStatistics::Certificate::refusedByNominalRouth},
    {"refused_by_nominal_stability", &LoopShapingStatistics::Certificate::refusedByNominalStability},
    {"refused_by_specifications", &LoopShapingStatistics::Certificate::refusedBySpecifications},
    {"refused_by_roots", &LoopShapingStatistics::Certificate::refusedByRoots},
    {"incumbent_updates", &LoopShapingStatistics::Certificate::incumbentUpdates},
    {"kernel_passes", &LoopShapingStatistics::Certificate::kernelPasses},
    {"gain_searches", &LoopShapingStatistics::Certificate::gainSearches},
    {"exchange_rounds", &LoopShapingStatistics::Certificate::exchangeRounds},
    {"ladder_steps", &LoopShapingStatistics::Certificate::ladderSteps},
    {"ladders_exhausted", &LoopShapingStatistics::Certificate::laddersExhausted},
    {"round_limits_reached", &LoopShapingStatistics::Certificate::roundLimitsReached},
    {"largest_working_set", &LoopShapingStatistics::Certificate::largestWorkingSet},
    {"proven_frequencies", &LoopShapingStatistics::Certificate::provenFrequencies},
    {"feasibility_proofs", &LoopShapingStatistics::Certificate::feasibilityProofs},
    {"proven_skips", &LoopShapingStatistics::Certificate::provenSkips},
};

double numberOr(const QJsonObject & object, const char * key)
{
    return object.contains(key) ? object[key].toDouble() : std::numeric_limits<double>::quiet_NaN();
}

void putNumber(QJsonObject & object, const char * key, double value)
{
    if (std::isfinite(value)) {
        object[key] = value;
    }
}

QJsonObject comparisonToJson(const Comparison & c)
{
    QJsonObject o;
    o["frequencies"] = static_cast<qint64>(c.frequencies);
    o["cloud_sizes_differ"] = static_cast<qint64>(c.cloudSizesDiffer);
    putNumber(o, "cloud_largest_relative", c.cloudLargestRelative);
    o["contour_points_differ"] = static_cast<qint64>(c.contourPointsDiffer);
    putNumber(o, "contour_largest_hausdorff", c.contourLargestHausdorff);
    o["reports_differ"] = static_cast<qint64>(c.reportsDiffer);
    o["epsilons_differ"] = static_cast<qint64>(c.epsilonsDiffer);
    o["columns_differ"] = static_cast<qint64>(c.columnsDiffer);
    putNumber(o, "column_largest_db", c.columnLargestDb);
    o["trace_sets_differ"] = static_cast<qint64>(c.traceSetsDiffer);
    o["unions_differ"] = static_cast<qint64>(c.unionsDiffer);
    if (!c.cpuStatus.empty() || !c.gpuStatus.empty()) {
        o["cpu_status"] = QString::fromStdString(c.cpuStatus);
        o["gpu_status"] = QString::fromStdString(c.gpuStatus);
        putNumber(o, "gain_relative", c.gainRelative);
        o["same_design"] = c.sameDesign;
    }
    return o;
}

Comparison comparisonFromJson(const QJsonObject & o)
{
    Comparison c;
    c.made = true;
    c.frequencies = static_cast<std::size_t>(o["frequencies"].toInteger());
    c.cloudSizesDiffer = static_cast<std::size_t>(o["cloud_sizes_differ"].toInteger());
    c.cloudLargestRelative = numberOr(o, "cloud_largest_relative");
    c.contourPointsDiffer = static_cast<std::size_t>(o["contour_points_differ"].toInteger());
    c.contourLargestHausdorff = numberOr(o, "contour_largest_hausdorff");
    c.reportsDiffer = static_cast<std::size_t>(o["reports_differ"].toInteger());
    c.epsilonsDiffer = static_cast<std::size_t>(o["epsilons_differ"].toInteger());
    c.columnsDiffer = static_cast<std::size_t>(o["columns_differ"].toInteger());
    c.columnLargestDb = numberOr(o, "column_largest_db");
    c.traceSetsDiffer = static_cast<std::size_t>(o["trace_sets_differ"].toInteger());
    c.unionsDiffer = static_cast<std::size_t>(o["unions_differ"].toInteger());
    c.cpuStatus = o["cpu_status"].toString().toStdString();
    c.gpuStatus = o["gpu_status"].toString().toStdString();
    c.gainRelative = numberOr(o, "gain_relative");
    c.sameDesign = o["same_design"].toBool();
    return c;
}

}

QJsonObject toJson(const Record & r)
{
    QJsonObject o;
    o["case"] = QString::fromStdString(r.caseId);
    o["case_index"] = static_cast<qint64>(r.caseIndex);
    o["steps_applied"] = static_cast<qint64>(r.stepsApplied);
    o["structure"] = QString::fromStdString(r.structure);
    o["algorithm"] = QString::fromStdString(r.algorithm);
    o["epsilon"] = r.epsilon;
    o["repetition"] = r.repetition;
    o["warm_up"] = r.warmUp;
    o["status"] = QString::fromStdString(r.status);
    o["message"] = QString::fromStdString(r.message);

    o["wall_ms"] = r.wallMilliseconds;
    o["cpu_ms"] = r.cpuMilliseconds;
    o["peak_memory_bytes"] = static_cast<qint64>(r.peakMemoryBytes);
    o["baseline_memory_bytes"] = static_cast<qint64>(r.baselineMemoryBytes);
    if (!r.memoryTrace.empty()) {
        QJsonArray trace;
        for (const auto & [ms, bytes] : r.memoryTrace) {
            QJsonArray sample;
            sample.append(ms);
            sample.append(static_cast<qint64>(bytes));
            trace.append(sample);
        }
        o["memory_trace"] = trace;
    }

    QJsonObject s;
    s["solve_ms"] = r.statistics.milliseconds;
    s["peak_live_nodes"] = static_cast<qint64>(r.statistics.peakLiveNodes);
    s["nodes_processed"] = static_cast<qint64>(r.statistics.nodesProcessed);
    s["boxes_classified"] = static_cast<qint64>(r.statistics.boxesClassified);
    s["boxes_feasible"] = static_cast<qint64>(r.statistics.boxesFeasible);
    s["boxes_infeasible"] = static_cast<qint64>(r.statistics.boxesInfeasible);
    s["boxes_ambiguous"] = static_cast<qint64>(r.statistics.boxesAmbiguous);
    s["stability_verdicts"] = static_cast<qint64>(r.statistics.stabilityVerdicts);
    s["stability_profiles"] = static_cast<qint64>(r.statistics.stabilityProfiles);
    {
        const LoopShapingStatistics::Certificate & c = r.statistics.certificate;
        QJsonObject certificate;
        certificate["kept"] = c.kept;
        certificate["finished"] = c.finished;
        certificate["exact_points"] = c.exactPoints;
        if (std::isfinite(c.lowerBound)) {
            certificate["lower_bound"] = c.lowerBound;
        }
        if (std::isfinite(c.lowerBoundStrict)) {
            certificate["lower_bound_strict"] = c.lowerBoundStrict;
        }
        for (const CertificateCount & count : kCertificateCounts) {
            certificate[count.key] = static_cast<qint64>(c.*count.member);
        }
        s["certificate"] = certificate;
    }
    if (!r.statistics.byDepth.empty()) {
        QJsonArray depth;
        for (std::size_t d = 0; d < r.statistics.byDepth.size(); ++d) {
            const auto & row = r.statistics.byDepth[d];
            QJsonObject entry;
            entry["depth"] = static_cast<qint64>(d);
            entry["nodes"] = static_cast<qint64>(row.nodes);
            entry["feasible"] = static_cast<qint64>(row.feasible);
            entry["infeasible"] = static_cast<qint64>(row.infeasible);
            entry["ambiguous"] = static_cast<qint64>(row.ambiguous);
            depth.append(entry);
        }
        s["by_depth"] = depth;
        QJsonArray ambiguous;
        for (std::size_t count : r.statistics.ambiguousByFrequency) ambiguous.append(static_cast<qint64>(count));
        s["ambiguous_by_frequency"] = ambiguous;
    }
    o["statistics"] = s;

    QJsonObject result;
    result["gain"] = r.gain;
    if (std::isfinite(r.worstExcessDb)) {
        result["worst_excess_db"] = r.worstExcessDb;
    }
    if (r.familyMembers > 0) {
        result["family_members"] = static_cast<qint64>(r.familyMembers);
        result["family_unstable"] = static_cast<qint64>(r.familyUnstable);
        result["family_worst_real_part"] = r.familyWorstRealPart;
    }
    if (r.nominalChecked) {
        result["nominal_stable"] = r.nominalStable;
        if (std::isfinite(r.nominalWorstRealPart)) {
            result["nominal_worst_real_part"] = r.nominalWorstRealPart;
        }
    }
    result["zeros"] = toArray(r.zeros);
    result["poles"] = toArray(r.poles);
    result["digest"] = QString::fromStdString(r.digest);
    o["result"] = result;

    QJsonObject e;
    e["hostname"] = QString::fromStdString(r.environment.hostname);
    e["os"] = QString::fromStdString(r.environment.operatingSystem);
    e["compiler"] = QString::fromStdString(r.environment.compiler);
    e["git_commit"] = QString::fromStdString(r.environment.gitCommit);
    e["interval_backend"] = QString::fromStdString(r.environment.intervalBackend);
    e["native_architecture"] = r.environment.nativeArchitecture;
    e["cores"] = r.environment.cores;
    e["load_average"] = r.environment.loadAverage;
    e["timestamp"] = QString::fromStdString(r.environment.timestamp);
    if (!r.environment.gpuState.empty()) {
        QJsonObject g;
        g["state"] = QString::fromStdString(r.environment.gpuState);
        g["name"] = QString::fromStdString(r.environment.gpuName);
        g["compute_capability"] = QString::fromStdString(r.environment.gpuCompute);
        g["memory_bytes"] = static_cast<qint64>(r.environment.gpuMemoryBytes);
        g["multiprocessors"] = r.environment.gpuMultiprocessors;
        g["double_precision_ratio"] = r.environment.gpuDoublePrecisionRatio;
        g["driver_version"] = r.environment.gpuDriverVersion;
        g["runtime_version"] = r.environment.gpuRuntimeVersion;
        e["gpu"] = g;
    }
    o["environment"] = e;

    if (!r.processor.empty()) {
        QJsonObject phases;
        phases["processor"] = QString::fromStdString(r.processor);
        putNumber(phases, "templates_ms", r.templatesMilliseconds);
        putNumber(phases, "boundaries_ms", r.boundariesMilliseconds);
        putNumber(phases, "gpu_start_ms", r.gpuStartMilliseconds);
        phases["frequencies"] = static_cast<qint64>(r.frequencies);
        phases["cloud_points"] = static_cast<qint64>(r.cloudPoints);
        phases["contour_points"] = static_cast<qint64>(r.contourPoints);
        phases["grid_cells"] = static_cast<qint64>(r.gridCells);
        QJsonObject steps;
        for (const auto & [step, ms] : r.stepMilliseconds) {
            steps[QString::fromStdString(step)] = ms;
        }
        phases["steps_ms"] = steps;
        if (r.comparison.made) {
            phases["comparison"] = comparisonToJson(r.comparison);
        }
        o["phases"] = phases;
    }
    return o;
}

Record recordFromJson(const QJsonObject & o)
{
    Record r;
    r.caseId = o["case"].toString().toStdString();
    r.caseIndex = static_cast<std::size_t>(o["case_index"].toInteger());
    r.stepsApplied = static_cast<std::size_t>(o["steps_applied"].toInteger());
    r.structure = o["structure"].toString().toStdString();
    r.algorithm = o["algorithm"].toString().toStdString();
    r.epsilon = o["epsilon"].toDouble();
    r.repetition = o["repetition"].toInt();
    r.warmUp = o["warm_up"].toBool();
    r.status = o["status"].toString().toStdString();
    r.message = o["message"].toString().toStdString();
    r.wallMilliseconds = o["wall_ms"].toDouble();
    r.cpuMilliseconds = o["cpu_ms"].toDouble();
    r.peakMemoryBytes = static_cast<std::uint64_t>(o["peak_memory_bytes"].toInteger());
    r.baselineMemoryBytes = static_cast<std::uint64_t>(o["baseline_memory_bytes"].toInteger());
    for (const QJsonValue & sample : o["memory_trace"].toArray()) {
        const QJsonArray pair = sample.toArray();
        if (pair.size() == 2) {
            r.memoryTrace.emplace_back(pair[0].toDouble(), static_cast<std::uint64_t>(pair[1].toInteger()));
        }
    }
    const QJsonObject s = o["statistics"].toObject();
    r.statistics.milliseconds = s["solve_ms"].toDouble();
    r.statistics.peakLiveNodes = static_cast<std::size_t>(s["peak_live_nodes"].toInteger());
    r.statistics.nodesProcessed = static_cast<std::size_t>(s["nodes_processed"].toInteger());
    r.statistics.boxesClassified = static_cast<std::size_t>(s["boxes_classified"].toInteger());
    r.statistics.boxesFeasible = static_cast<std::size_t>(s["boxes_feasible"].toInteger());
    r.statistics.boxesInfeasible = static_cast<std::size_t>(s["boxes_infeasible"].toInteger());
    r.statistics.boxesAmbiguous = static_cast<std::size_t>(s["boxes_ambiguous"].toInteger());
    r.statistics.stabilityVerdicts = static_cast<std::size_t>(s["stability_verdicts"].toInteger());
    r.statistics.stabilityProfiles = static_cast<std::size_t>(s["stability_profiles"].toInteger());
    if (s.contains("certificate")) {
        const QJsonObject certificate = s["certificate"].toObject();
        LoopShapingStatistics::Certificate & c = r.statistics.certificate;
        c.kept = certificate["kept"].toBool();
        c.finished = certificate["finished"].toBool();
        c.exactPoints = certificate["exact_points"].toBool();
        c.lowerBound = certificate.contains("lower_bound") ? certificate["lower_bound"].toDouble()
                                                           : std::numeric_limits<double>::infinity();
        c.lowerBoundStrict = certificate.contains("lower_bound_strict") ? certificate["lower_bound_strict"].toDouble()
                                                                        : std::numeric_limits<double>::infinity();
        for (const CertificateCount & count : kCertificateCounts) {
            c.*count.member = static_cast<std::size_t>(certificate[count.key].toInteger());
        }
    }
    for (const QJsonValue & v : s["by_depth"].toArray()) {
        const QJsonObject entry = v.toObject();
        LoopShapingStatistics::DepthRow row;
        row.nodes = static_cast<std::size_t>(entry["nodes"].toInteger());
        row.feasible = static_cast<std::size_t>(entry["feasible"].toInteger());
        row.infeasible = static_cast<std::size_t>(entry["infeasible"].toInteger());
        row.ambiguous = static_cast<std::size_t>(entry["ambiguous"].toInteger());
        r.statistics.byDepth.push_back(row);
    }
    for (const QJsonValue & v : s["ambiguous_by_frequency"].toArray()) {
        r.statistics.ambiguousByFrequency.push_back(static_cast<std::size_t>(v.toInteger()));
    }
    const QJsonObject result = o["result"].toObject();
    r.gain = result["gain"].toDouble();
    r.worstExcessDb = result.contains("worst_excess_db") ? result["worst_excess_db"].toDouble()
                                                         : std::numeric_limits<double>::quiet_NaN();
    r.familyMembers = static_cast<std::size_t>(result["family_members"].toInteger());
    r.familyUnstable = static_cast<std::size_t>(result["family_unstable"].toInteger());
    r.familyWorstRealPart = result.contains("family_worst_real_part") ? result["family_worst_real_part"].toDouble()
                                                                      : std::numeric_limits<double>::quiet_NaN();
    r.nominalChecked = result.contains("nominal_stable");
    r.nominalStable = result["nominal_stable"].toBool();
    r.nominalWorstRealPart = result.contains("nominal_worst_real_part") ? result["nominal_worst_real_part"].toDouble()
                                                                        : std::numeric_limits<double>::quiet_NaN();
    r.zeros = fromArray(result["zeros"].toArray());
    r.poles = fromArray(result["poles"].toArray());
    r.digest = result["digest"].toString().toStdString();
    const QJsonObject e = o["environment"].toObject();
    r.environment.hostname = e["hostname"].toString().toStdString();
    r.environment.operatingSystem = e["os"].toString().toStdString();
    r.environment.compiler = e["compiler"].toString().toStdString();
    r.environment.gitCommit = e["git_commit"].toString().toStdString();
    r.environment.intervalBackend = e["interval_backend"].toString().toStdString();
    r.environment.nativeArchitecture = e["native_architecture"].toBool();
    r.environment.cores = e["cores"].toInt();
    r.environment.loadAverage = e["load_average"].toDouble(-1.0);
    r.environment.timestamp = e["timestamp"].toString().toStdString();
    if (e.contains("gpu")) {
        const QJsonObject g = e["gpu"].toObject();
        r.environment.gpuState = g["state"].toString().toStdString();
        r.environment.gpuName = g["name"].toString().toStdString();
        r.environment.gpuCompute = g["compute_capability"].toString().toStdString();
        r.environment.gpuMemoryBytes = static_cast<std::uint64_t>(g["memory_bytes"].toInteger());
        r.environment.gpuMultiprocessors = g["multiprocessors"].toInt();
        r.environment.gpuDoublePrecisionRatio = g["double_precision_ratio"].toInt();
        r.environment.gpuDriverVersion = g["driver_version"].toInt();
        r.environment.gpuRuntimeVersion = g["runtime_version"].toInt();
    }
    if (o.contains("phases")) {
        const QJsonObject phases = o["phases"].toObject();
        r.processor = phases["processor"].toString().toStdString();
        r.templatesMilliseconds = numberOr(phases, "templates_ms");
        r.boundariesMilliseconds = numberOr(phases, "boundaries_ms");
        r.gpuStartMilliseconds = numberOr(phases, "gpu_start_ms");
        r.frequencies = static_cast<std::size_t>(phases["frequencies"].toInteger());
        r.cloudPoints = static_cast<std::size_t>(phases["cloud_points"].toInteger());
        r.contourPoints = static_cast<std::size_t>(phases["contour_points"].toInteger());
        r.gridCells = static_cast<std::size_t>(phases["grid_cells"].toInteger());
        const QJsonObject steps = phases["steps_ms"].toObject();
        for (auto it = steps.begin(); it != steps.end(); ++it) {
            r.stepMilliseconds.emplace_back(it.key().toStdString(), it.value().toDouble());
        }
        if (phases.contains("comparison")) {
            r.comparison = comparisonFromJson(phases["comparison"].toObject());
        }
    }
    return r;
}

void writeRecord(const Record & record, const std::string & path)
{
    QFile file(QString::fromStdString(path));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        throw FileError(path + ": cannot write the benchmark record");
    }
    file.write(QJsonDocument(toJson(record)).toJson(QJsonDocument::Indented));
}

Record readRecord(const std::string & path)
{
    QFile file(QString::fromStdString(path));
    if (!file.open(QIODevice::ReadOnly)) {
        throw FileError(path + ": cannot read the benchmark record");
    }
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    if (document.isNull() || !document.isObject()) {
        throw InvalidInput(path + ": malformed benchmark record: " + error.errorString().toStdString());
    }
    return recordFromJson(document.object());
}

std::vector<Record> readRecords(const std::string & directory)
{
    std::vector<Record> records;
    QDir dir(QString::fromStdString(directory));
    for (const QString & name : dir.entryList({"*.json"}, QDir::Files, QDir::Name)) {
        records.push_back(readRecord(dir.filePath(name).toStdString()));
    }
    return records;
}

void writeJsonLines(const std::vector<Record> & records, const std::string & path)
{
    QFile file(QString::fromStdString(path));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        throw FileError(path + ": cannot write the benchmark records");
    }
    for (const Record & record : records) {
        file.write(QJsonDocument(toJson(record)).toJson(QJsonDocument::Compact));
        file.write("\n");
    }
}

std::string digestOf(double gain, const std::vector<double> & zeros, const std::vector<double> & poles)
{
    std::uint64_t hash = 1469598103934665603ULL;
    const auto add = [&](double value) {
        std::uint64_t bits;
        std::memcpy(&bits, &value, sizeof bits);
        for (int i = 0; i < 8; ++i) {
            hash ^= (bits >> (8 * i)) & 0xffU;
            hash *= 1099511628211ULL;
        }
    };
    add(gain);
    for (const double z : zeros) {
        add(z);
    }
    for (const double p : poles) {
        add(p);
    }
    std::ostringstream out;
    out << std::hex << hash;
    return out.str();
}

}
