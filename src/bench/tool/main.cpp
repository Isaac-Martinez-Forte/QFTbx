// qftbx-bench: the benchmark tool of QFTbx.
//
//   qftbx-bench run <plan.xml> [--jobs K]   runs every case of the plan, K
//                                           processes at a time, and writes
//                                           the records and the summaries
//   qftbx-bench case <plan.xml> <index>     runs one case in this process
//                                           (what "run" launches)
//   qftbx-bench summarize <plan.xml>        gathers the records already
//                                           written into the summaries
//   qftbx-bench cases <plan.xml>            lists the cases of the plan
//   qftbx-bench example                     prints an example plan
//
// The plan format and the records are described in docs/BENCHMARKING.md.
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>
#include <vector>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QTemporaryFile>

#include "src/bench/measurement.h"
#include "src/bench/plan.h"
#include "src/bench/runner.h"
#include "src/bench/summary.h"

using namespace qftbx::bench;

namespace {

int usage()
{
    std::cerr << "usage:\n"
                 "  qftbx-bench run <plan.xml> [--jobs K]\n"
                 "  qftbx-bench case <plan.xml> <case index>\n"
                 "  qftbx-bench summarize <plan.xml>\n"
                 "  qftbx-bench cases <plan.xml>\n"
                 "  qftbx-bench example\n";
    return 2;
}

//Paths in the plan are relative to the plan file, not to the current
//directory, so a plan can be moved with its project and its results.
Plan loadPlan(const std::string & path)
{
    Plan plan = readPlan(path);
    const QDir base = QFileInfo(QString::fromStdString(path)).absoluteDir();
    plan.projectFile = base.absoluteFilePath(QString::fromStdString(plan.projectFile)).toStdString();
    plan.outputDirectory = base.absoluteFilePath(QString::fromStdString(plan.outputDirectory)).toStdString();
    return plan;
}

int runPlan(const std::string & path, int jobs)
{
    const Plan plan = loadPlan(path);
    const std::vector<Case> cases = expandCases(plan);
    std::cout << "plan '" << plan.name << "': " << cases.size() << " cases, "
              << measuredStructures(plan).size() << " structures, " << plan.algorithms.size()
              << " algorithms, " << plan.epsilons.size() << " epsilons, " << plan.repetitions
              << " repetitions" << (plan.warmUp ? " plus a warm-up" : "") << std::endl;

    Runner runner;
    const std::size_t failures = runner.run(plan, path, QCoreApplication::applicationFilePath().toStdString(), jobs,
        [&](const Runner::Event & event) {
            if (event.kind == Runner::Event::Kind::Finished && event.record != nullptr) {
                const Record & r = *event.record;
                std::cout << "[" << event.done << "/" << event.total << "] " << r.caseId << ": " << r.status;
                if (r.status == "solved") {
                    std::cout << "  k=" << r.gain << "  " << r.wallMilliseconds / 1000.0 << " s";
                    if (r.peakMemoryBytes > 0) {
                        std::cout << "  " << r.peakMemoryBytes / (1024.0 * 1024.0) << " MB";
                    }
                } else if (!r.message.empty()) {
                    std::cout << "  " << r.message;
                }
                std::cout << std::endl;
            }
        });

    const QDir out(QString::fromStdString(plan.outputDirectory));
    std::cout << "records: " << recordsDirectory(plan) << "\n"
              << "summary: " << out.filePath(QString::fromStdString(plan.name) + "-summary.csv").toStdString() << "\n"
              << std::endl;
    std::cout << markdownTable(summarize(readRecords(recordsDirectory(plan))));
    return failures == 0 ? 0 : 1;
}

int runOneCase(const std::string & path, const std::string & indexText)
{
    const Plan plan = loadPlan(path);
    const std::vector<Case> cases = expandCases(plan);
    const std::size_t index = static_cast<std::size_t>(std::strtoul(indexText.c_str(), nullptr, 10));
    if (index >= cases.size()) {
        std::cerr << "qftbx-bench: the plan has " << cases.size() << " cases, there is no case " << indexText << "\n";
        return 2;
    }
    QDir().mkpath(QString::fromStdString(recordsDirectory(plan)));
    const Record record = runCase(plan, cases[index]);
    writeRecord(record, recordPath(plan, cases[index]));
    return record.status == "solved" || record.status == "infeasible" ? 0 : 1;
}

int listCases(const std::string & path)
{
    const Plan plan = loadPlan(path);
    for (const Case & c : expandCases(plan)) {
        std::cout << c.index << "  " << caseId(c) << "  " << structureLabel(plan, c.stepsApplied) << "  "
                  << algorithmName(c.algorithm) << "  epsilon " << c.epsilon << "  repetition " << c.repetition
                  << (c.warmUp ? " (warm-up)" : "") << "\n";
    }
    return 0;
}

int printExample()
{
    QTemporaryFile file;
    if (!file.open()) {
        return 1;
    }
    writePlan(examplePlan(), file.fileName().toStdString());
    file.seek(0);
    std::cout << file.readAll().toStdString();
    return 0;
}

} // namespace

int main(int argc, char ** argv)
{
    QCoreApplication application(argc, argv);
    const std::vector<std::string> args(argv + 1, argv + argc);
    if (args.empty()) {
        return usage();
    }

    try {
        const std::string & command = args[0];
        if (command == "run" && args.size() >= 2) {
            int jobs = 0;
            for (std::size_t i = 2; i + 1 < args.size(); ++i) {
                if (args[i] == "--jobs") {
                    jobs = std::atoi(args[i + 1].c_str());
                }
            }
            return runPlan(args[1], jobs);
        }
        if (command == "case" && args.size() == 3) {
            return runOneCase(args[1], args[2]);
        }
        if (command == "summarize" && args.size() == 2) {
            gather(loadPlan(args[1]));
            std::cout << markdownTable(summarize(readRecords(recordsDirectory(loadPlan(args[1])))));
            return 0;
        }
        if (command == "cases" && args.size() == 2) {
            return listCases(args[1]);
        }
        if (command == "example") {
            return printExample();
        }
    } catch (const std::exception & failure) {
        std::cerr << "qftbx-bench: " << failure.what() << "\n";
        return 1;
    }
    return usage();
}
