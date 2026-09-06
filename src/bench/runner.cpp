#include "src/bench/runner.h"

#include <chrono>
#include <memory>
#include <thread>
#include <vector>

#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>
#include <QThread>

#include "src/bench/measurement.h"
#include "src/bench/summary.h"
#include "src/core/common/exception.h"

namespace qftbx::bench {

namespace {

struct Running
{
    std::unique_ptr<QProcess> process;
    const Case * c = nullptr;
    std::chrono::steady_clock::time_point started;
};

int jobsOf(const Plan & plan, int override)
{
    int jobs = override > 0 ? override : plan.jobs;
    if (jobs <= 0) {
        jobs = QThread::idealThreadCount() - 1;
    }
    return jobs < 1 ? 1 : jobs;
}

} // namespace

std::size_t Runner::run(const Plan & plan, const std::string & planPath, const std::string & workerProgram,
                        int jobs, const Listener & listener)
{
    m_cancel.store(false);

    const std::vector<Case> cases = expandCases(plan);
    const std::string records = recordsDirectory(plan);
    if (!QDir().mkpath(QString::fromStdString(records))) {
        throw FileError(records + ": cannot create the records directory");
    }

    const int atOnce = jobsOf(plan, jobs);
    std::vector<Running> running;
    std::vector<Record> results;
    results.reserve(cases.size());
    std::size_t next = 0;
    std::size_t done = 0;
    std::size_t failures = 0;

    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert("OMP_NUM_THREADS", "1");

    const auto notify = [&](Event::Kind kind, const Case * c, const Record * record, const std::string & text) {
        if (listener) {
            Event event;
            event.kind = kind;
            event.c = c;
            event.record = record;
            event.done = done;
            event.total = cases.size();
            event.running = running.size();
            event.text = text;
            listener(event);
        }
    };

    const auto finish = [&](Running & job, const std::string & failureStatus, const std::string & failureMessage) {
        const std::string path = recordPath(plan, *job.c);
        Record record;
        if (failureStatus.empty() && QFileInfo::exists(QString::fromStdString(path))) {
            record = readRecord(path);
        } else {
            record = failureRecord(plan, *job.c, failureStatus.empty() ? "crashed" : failureStatus,
                                   failureMessage.empty() ? "the case left no record" : failureMessage);
            writeRecord(record, path);
        }
        if (record.status != "solved" && record.status != "infeasible") {
            ++failures;
        }
        ++done;
        results.push_back(record);
        notify(Event::Kind::Finished, job.c, &results.back(), record.status);
    };

    while (next < cases.size() || !running.empty()) {
        //Fill the free slots.
        while (!m_cancel.load() && next < cases.size() && static_cast<int>(running.size()) < atOnce) {
            const Case & c = cases[next++];
            Running job;
            job.c = &c;
            job.process = std::make_unique<QProcess>();
            job.process->setProcessEnvironment(environment);
            job.process->setProcessChannelMode(QProcess::ForwardedErrorChannel);
            job.process->setStandardOutputFile(QProcess::nullDevice());
            job.process->start(QString::fromStdString(workerProgram),
                               {"case", QString::fromStdString(planPath), QString::number(c.index)});
            job.started = std::chrono::steady_clock::now();
            running.push_back(std::move(job));
            notify(Event::Kind::Started, &c, nullptr, caseId(c));
        }

        if (running.empty()) {
            if (m_cancel.load()) {
                break;
            }
            continue;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        for (std::size_t i = 0; i < running.size();) {
            Running & job = running[i];
            job.process->waitForFinished(0);
            const bool finished = job.process->state() == QProcess::NotRunning;
            const double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - job.started).count();

            if (m_cancel.load() && !finished) {
                job.process->kill();
                job.process->waitForFinished(5000);
                finish(job, "cancelled", "the run was cancelled");
            } else if (!finished && plan.timeoutSeconds > 0.0 && elapsed > plan.timeoutSeconds) {
                job.process->kill();
                job.process->waitForFinished(5000);
                finish(job, "timeout", "killed after " + std::to_string(plan.timeoutSeconds) + " seconds");
            } else if (finished) {
                if (job.process->exitStatus() == QProcess::CrashExit) {
                    finish(job, "crashed", "the process ended abnormally");
                } else if (job.process->error() == QProcess::FailedToStart) {
                    finish(job, "error", "the worker program could not be started: " + workerProgram);
                } else {
                    finish(job, "", "");
                }
            } else {
                ++i;
                continue;
            }
            running.erase(running.begin() + static_cast<std::ptrdiff_t>(i));
        }
    }

    gather(plan);
    notify(Event::Kind::Message, nullptr, nullptr, "gathered");
    return failures;
}

void gather(const Plan & plan)
{
    const std::vector<Record> records = readRecords(recordsDirectory(plan));
    const QDir out(QString::fromStdString(plan.outputDirectory));
    const QString stem = QString::fromStdString(plan.name);
    writeJsonLines(records, out.filePath(stem + ".jsonl").toStdString());
    const std::vector<Aggregate> aggregates = summarize(records);
    writeCsv(aggregates, out.filePath(stem + "-summary.csv").toStdString());
    writeMarkdown(aggregates, out.filePath(stem + "-summary.md").toStdString());
}

} // namespace qftbx::bench
