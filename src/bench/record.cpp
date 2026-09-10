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

} // namespace

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
    o["statistics"] = s;

    QJsonObject result;
    result["gain"] = r.gain;
    if (std::isfinite(r.worstExcessDb)) {
        result["worst_excess_db"] = r.worstExcessDb;
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
    o["environment"] = e;
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
    //Absent in records written before the split: they read as zero.
    r.statistics.boxesFeasible = static_cast<std::size_t>(s["boxes_feasible"].toInteger());
    r.statistics.boxesInfeasible = static_cast<std::size_t>(s["boxes_infeasible"].toInteger());
    r.statistics.boxesAmbiguous = static_cast<std::size_t>(s["boxes_ambiguous"].toInteger());
    r.statistics.stabilityVerdicts = static_cast<std::size_t>(s["stability_verdicts"].toInteger());
    r.statistics.stabilityProfiles = static_cast<std::size_t>(s["stability_profiles"].toInteger());
    const QJsonObject result = o["result"].toObject();
    r.gain = result["gain"].toDouble();
    r.worstExcessDb = result.contains("worst_excess_db") ? result["worst_excess_db"].toDouble()
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

} // namespace qftbx::bench
