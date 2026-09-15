#include "src/bench/measurement.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <map>
#include <sstream>
#include <thread>

#include <QDir>
#include <QTemporaryFile>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#else
#include <sys/resource.h>
#include <sys/time.h>
#include <unistd.h>
#endif

#include "src/app/project_controller.h"
#include "src/core/common/exception.h"
#include "src/core/loopshaping/loop_shaping_result.h"
#include "src/core/project/settings.h"

namespace qftbx::bench {

namespace {

//The clocks and the memory of this process, read the same way everywhere.
double cpuMilliseconds()
{
#ifdef _WIN32
    FILETIME creation, exit, kernel, user;
    if (!GetProcessTimes(GetCurrentProcess(), &creation, &exit, &kernel, &user)) {
        return 0.0;
    }
    const auto ticks = [](const FILETIME & t) {
        return (static_cast<std::uint64_t>(t.dwHighDateTime) << 32) | t.dwLowDateTime;
    };
    return (ticks(kernel) + ticks(user)) / 10000.0;
#else
    rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return (usage.ru_utime.tv_sec + usage.ru_stime.tv_sec) * 1000.0
           + (usage.ru_utime.tv_usec + usage.ru_stime.tv_usec) / 1000.0;
#endif
}

std::uint64_t peakResidentBytes()
{
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS counters;
    if (!GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof counters)) {
        return 0;
    }
    return counters.PeakWorkingSetSize;
#else
    rusage usage;
    getrusage(RUSAGE_SELF, &usage);
#ifdef __APPLE__
    return static_cast<std::uint64_t>(usage.ru_maxrss);
#else
    return static_cast<std::uint64_t>(usage.ru_maxrss) * 1024ULL;
#endif
#endif
}

std::uint64_t currentResidentBytes()
{
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS counters;
    if (!GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof counters)) {
        return 0;
    }
    return counters.WorkingSetSize;
#elif defined(__linux__)
    std::ifstream statm("/proc/self/statm");
    long total = 0, resident = 0;
    if (statm >> total >> resident) {
        return static_cast<std::uint64_t>(resident) * static_cast<std::uint64_t>(sysconf(_SC_PAGESIZE));
    }
    return 0;
#else
    return 0;
#endif
}

void limitAddressSpace(int megabytes)
{
#ifndef _WIN32
    if (megabytes > 0) {
        rlimit limit;
        limit.rlim_cur = static_cast<rlim_t>(megabytes) * 1024ULL * 1024ULL;
        limit.rlim_max = limit.rlim_cur;
        setrlimit(RLIMIT_AS, &limit);
    }
#else
    (void) megabytes;
#endif
}

//The settings of the plan as a settings file, so the same reader applies
//and the same ranges are enforced.
Settings settingsOf(const Plan & plan)
{
    if (plan.settings.empty()) {
        return Settings();
    }
    std::map<std::string, std::vector<std::pair<std::string, std::string>>> sections;
    for (const auto & [key, value] : plan.settings) {
        const std::size_t dot = key.rfind('.');
        if (dot == std::string::npos) {
            throw InvalidInput("benchmark: a setting key needs its section, as in 'stability.base-grid-points': '" + key + "'");
        }
        sections[key.substr(0, dot)].emplace_back(key.substr(dot + 1), value);
    }
    QTemporaryFile file;
    if (!file.open()) {
        throw FileError("benchmark: cannot write the settings of the plan to a temporary file");
    }
    std::ostringstream text;
    for (const auto & [section, entries] : sections) {
        text << "[" << section << "]\n";
        for (const auto & [key, value] : entries) {
            text << key << " = " << value << "\n";
        }
    }
    const std::string content = text.str();
    file.write(content.data(), static_cast<qint64>(content.size()));
    file.flush();
    return readSettings(file.fileName().toStdString());
}

//Samples the resident size from another thread while the algorithm runs.
class MemoryTracer
{
public:
    explicit MemoryTracer(std::vector<std::pair<double, std::uint64_t>> & trace) : m_trace(trace) {}

    void start()
    {
        m_start = std::chrono::steady_clock::now();
        m_thread = std::thread([this]() {
            while (!m_stop.load()) {
                const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - m_start).count();
                m_trace.emplace_back(ms, currentResidentBytes());
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
    }

    void stop()
    {
        m_stop.store(true);
        if (m_thread.joinable()) {
            m_thread.join();
        }
    }

private:
    std::vector<std::pair<double, std::uint64_t>> & m_trace;
    std::atomic<bool> m_stop{false};
    std::thread m_thread;
    std::chrono::steady_clock::time_point m_start;
};

} // namespace

std::string recordsDirectory(const Plan & plan)
{
    return QDir(QString::fromStdString(plan.outputDirectory)).filePath(QString::fromStdString(plan.name) + "/records").toStdString();
}

std::string recordPath(const Plan & plan, const Case & c)
{
    return QDir(QString::fromStdString(recordsDirectory(plan))).filePath(QString::fromStdString(caseId(c)) + ".json").toStdString();
}

Record failureRecord(const Plan & plan, const Case & c, const std::string & status, const std::string & message)
{
    Record record;
    record.caseId = caseId(c);
    record.caseIndex = c.index;
    record.stepsApplied = c.stepsApplied;
    record.structure = structureLabel(plan, c.stepsApplied);
    record.algorithm = algorithmName(c.algorithm);
    record.epsilon = c.epsilon;
    record.repetition = c.repetition;
    record.warmUp = c.warmUp;
    record.status = status;
    record.message = message;
    record.environment = describeEnvironment();
    return record;
}

Record runCase(const Plan & plan, const Case & c)
{
    Record record = failureRecord(plan, c, "error", "");
    limitAddressSpace(plan.memoryLimitMegabytes);

    try {
        ProjectController controller;
        controller.load(plan.projectFile);
        if (controller.controllerStructure() == nullptr) {
            throw InvalidInput(plan.projectFile + ": the project has no controller structure to start from");
        }
        controller.setControllerStructure(structureAfter(plan, *controller.controllerStructure(), c.stepsApplied));
        controller.applySettings(settingsOf(plan));

        MemoryTracer tracer(record.memoryTrace);
        if (plan.measures.memory) {
            record.baselineMemoryBytes = currentResidentBytes();
        }
        if (plan.measures.memoryTrace) {
            tracer.start();
        }
        const double cpuBefore = plan.measures.cpu ? cpuMilliseconds() : 0.0;
        const auto wallBefore = std::chrono::steady_clock::now();

        bool solved = false;
        std::string failure;
        try {
            solved = controller.computeLoopShaping(c.epsilon, c.algorithm, Range(1e-9, 10.0), 100);
        } catch (const InvalidInput & refused) {
            failure = refused.what();
        }

        if (plan.measures.time) {
            record.wallMilliseconds = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - wallBefore).count();
        }
        if (plan.measures.cpu) {
            record.cpuMilliseconds = cpuMilliseconds() - cpuBefore;
        }
        if (plan.measures.memoryTrace) {
            tracer.stop();
        }
        if (plan.measures.memory) {
            record.peakMemoryBytes = peakResidentBytes();
        }

        if (!failure.empty()) {
            record.status = failure.find("No feasible solution") != std::string::npos ? "infeasible" : "error";
            record.message = failure;
            return record;
        }
        if (!solved) {
            record.status = "error";
            record.message = "the loop shaping produced no result";
            return record;
        }

        LoopShapingResult * result = controller.loopShapingResult();
        if (plan.measures.counters) {
            record.statistics = result->statistics();
        }
        LtiSystem * designed = result->controller();
        record.gain = designed->gain().range().min;
        for (Parameter & zero : designed->numerator()) {
            record.zeros.push_back(zero.range().min);
        }
        for (Parameter & pole : designed->denominator()) {
            record.poles.push_back(pole.range().min);
        }
        record.digest = digestOf(record.gain, record.zeros, record.poles);
        if (result->check().has_value()) {
            record.worstExcessDb = result->check()->worstExcessDb;
        }
        record.status = "solved";
    } catch (const std::exception & failure) {
        record.status = "error";
        record.message = failure.what();
    }

    return record;
}

} // namespace qftbx::bench
