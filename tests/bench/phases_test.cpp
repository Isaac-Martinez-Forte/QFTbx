/**
 * @file
 * @brief Tests of the phases a benchmark plan can compute before the search.
 *
 * The phases survive the plan's file, and a section that computes nothing,
 * names an unknown processor or metric, or names no processor and compares
 * nothing is refused. They expand into the repetitions of every processor
 * named and one comparison, and a plan without them expands as it always
 * did. A case on toolbox-1 times each phase and keeps the steps the engines
 * record and the size of what it computed, on the project's grids or denser
 * ones. A comparison whose GPU engines are the processor's own finds
 * nothing apart, the design included; without GPU support it fails with the
 * reason. comparePhases notices a plant moved, a contour point added and a
 * column end displaced; it measures how far traces that keep their shape
 * moved and counts those that change it. The phases and the comparison
 * survive their JSON.
 */

#include <gtest/gtest.h>

#include <cmath>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <QFile>
#include <QTemporaryDir>

#include "src/bench/comparison.h"
#include "src/bench/measurement.h"
#include "src/bench/plan.h"
#include "src/bench/record.h"
#include "src/core/boundaries/boundary_engine.h"
#include "src/core/common/exception.h"
#include "src/core/pipeline/gpu_provider.h"
#include "src/core/templates/template_engine.h"

using namespace qftbx;
using namespace qftbx::bench;

namespace {

class ProcessorEnginesProvider : public GpuProvider
{
public:
    ProcessorEnginesProvider() { m_status.availability = GpuAvailability::Usable; }

    const GpuStatus & status() const override { return m_status; }

    std::unique_ptr<TemplateComputation> templateComputation() const override
    {
        return std::make_unique<TemplateEngine>();
    }

    std::unique_ptr<BoundaryComputation> boundaryComputation() const override
    {
        return std::make_unique<BoundaryEngine>();
    }

private:
    GpuStatus m_status;
};

class ProviderInstalled
{
public:
    explicit ProviderInstalled(std::unique_ptr<GpuProvider> provider) { installGpuProvider(std::move(provider)); }
    ~ProviderInstalled() { installGpuProvider(nullptr); }
};

Plan phasesPlan(const std::string & outputDirectory)
{
    Plan plan;
    plan.name = "phases";
    plan.projectFile = std::string(QFTBX_EXAMPLES_DIR "/toolbox-1.qft");
    plan.outputDirectory = outputDirectory;
    plan.repetitions = 2;
    plan.warmUp = true;
    plan.threads = 0;
    Phases phases;
    phases.templates = true;
    phases.boundaries = true;
    phases.loopShaping = false;
    phases.processors = {Processor::Cpu, Processor::Gpu};
    phases.compare = true;
    plan.phases = phases;
    return plan;
}

void write(const std::string & path, const char * text)
{
    QFile file(QString::fromStdString(path));
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    file.write(text);
}

std::map<std::string, double> stepsOf(const Record & record)
{
    return std::map<std::string, double>(record.stepMilliseconds.begin(), record.stepMilliseconds.end());
}

}

TEST(BenchmarkPhases, SurviveTheirFile)
{
    QTemporaryDir directory;
    Plan plan = phasesPlan(directory.path().toStdString());
    plan.phases->sweepPoints = 7;
    plan.phases->frequencyCount = 4;
    plan.phases->metric = EpsilonMetric{HullMetric::Nichols, 0.25};
    plan.phases->proposeEpsilon = true;
    plan.phases->fromContour = false;
    plan.phases->phasePoints = 721;
    plan.phases->magnitudePoints = 241;
    const std::string path = directory.filePath("plan.xml").toStdString();
    writePlan(plan, path);
    const Plan back = readPlan(path);

    EXPECT_EQ(back.threads, 0);
    ASSERT_TRUE(back.phases.has_value());
    EXPECT_TRUE(back.phases->templates);
    EXPECT_TRUE(back.phases->boundaries);
    EXPECT_FALSE(back.phases->loopShaping);
    EXPECT_EQ(back.phases->processors, (std::vector<Processor>{Processor::Cpu, Processor::Gpu}));
    EXPECT_TRUE(back.phases->compare);
    EXPECT_EQ(back.phases->sweepPoints, 7);
    EXPECT_EQ(back.phases->frequencyCount, 4);
    ASSERT_TRUE(back.phases->metric.has_value());
    EXPECT_EQ(*back.phases->metric, (EpsilonMetric{HullMetric::Nichols, 0.25}));
    EXPECT_TRUE(back.phases->proposeEpsilon);
    EXPECT_FALSE(back.phases->fromContour);
    EXPECT_EQ(back.phases->phasePoints, 721);
    EXPECT_EQ(back.phases->magnitudePoints, 241);
    EXPECT_TRUE(back.algorithms.empty()) << "a plan of phases without the search needs no algorithm";
}

TEST(BenchmarkPhases, AMalformedSectionIsRefusedWithItsReason)
{
    QTemporaryDir directory;
    const std::string path = directory.filePath("bad.xml").toStdString();
    const char * malformed[] = {
        "<QFTbench version=\"1\"><project file=\"x.qft\"/><phases loop-shaping=\"0\"/></QFTbench>",
        "<QFTbench version=\"1\"><project file=\"x.qft\"/><phases templates=\"1\" loop-shaping=\"0\" processors=\"tpu\"/></QFTbench>",
        "<QFTbench version=\"1\"><project file=\"x.qft\"/><phases templates=\"1\" loop-shaping=\"0\" processors=\"\"/></QFTbench>",
        "<QFTbench version=\"1\"><project file=\"x.qft\"/><phases templates=\"1\" loop-shaping=\"0\"><templates metric=\"polar\"/></phases></QFTbench>",
        "<QFTbench version=\"1\"><project file=\"x.qft\"/><phases boundaries=\"1\" loop-shaping=\"0\"><boundaries source=\"sheet\"/></phases></QFTbench>",
        "<QFTbench version=\"1\"><project file=\"x.qft\"/><phases templates=\"1\" loop-shaping=\"0\"><templates sweep-points=\"-3\"/></phases></QFTbench>",
        "<QFTbench version=\"1\"><project file=\"x.qft\"/><phases templates=\"1\"/></QFTbench>",
    };
    for (const char * text : malformed) {
        write(path, text);
        EXPECT_THROW(readPlan(path), InvalidInput) << text;
    }
}

TEST(BenchmarkPhases, ExpandIntoEachProcessorAndOneComparison)
{
    Plan plan = phasesPlan(".");
    std::vector<Case> cases = expandCases(plan);
    ASSERT_EQ(cases.size(), 7u);
    EXPECT_EQ(caseId(cases[0]), "cpu-phases-r0");
    EXPECT_TRUE(cases[0].warmUp);
    EXPECT_EQ(caseId(cases[2]), "cpu-phases-r2");
    EXPECT_EQ(caseId(cases[3]), "gpu-phases-r0");
    EXPECT_EQ(cases[3].processor, Processor::Gpu);
    EXPECT_EQ(caseId(cases[6]), "compare-phases-r1");
    EXPECT_TRUE(cases[6].comparison);
    EXPECT_EQ(caseKind(cases[6]), "compare");

    plan.phases->loopShaping = true;
    plan.algorithms = {mc2, nt};
    plan.epsilons = {0.5};
    cases = expandCases(plan);
    ASSERT_EQ(cases.size(), 14u);
    EXPECT_EQ(caseId(cases[1]), "cpu-s0-mc2-e0_5-r1");
    EXPECT_EQ(caseId(cases[12]), "compare-s0-mc2-e0_5-r1");
    EXPECT_EQ(caseId(cases[13]), "compare-s0-nt-e0_5-r1");

    plan.phases.reset();
    cases = expandCases(plan);
    ASSERT_EQ(cases.size(), 6u);
    EXPECT_EQ(caseId(cases[1]), "s0-mc2-e0_5-r1");
    EXPECT_EQ(caseKind(cases[1]), "");
}

TEST(BenchmarkPhases, ACaseTimesEachPhaseAndKeepsItsSteps)
{
    QTemporaryDir directory;
    const Plan plan = phasesPlan(directory.path().toStdString());
    const Record record = runCase(plan, expandCases(plan)[1]);

    EXPECT_EQ(record.status, "computed") << record.message;
    EXPECT_EQ(record.processor, "cpu");
    EXPECT_EQ(record.algorithm, "");
    EXPECT_GT(record.templatesMilliseconds, 0.0);
    EXPECT_GT(record.boundariesMilliseconds, 0.0);
    EXPECT_TRUE(std::isnan(record.gpuStartMilliseconds));
    const std::map<std::string, double> steps = stepsOf(record);
    EXPECT_EQ(steps.count("templates/sweep"), 1u);
    EXPECT_EQ(steps.count("boundaries/union"), 1u);
    EXPECT_GT(record.frequencies, 0u);
    EXPECT_GT(record.cloudPoints, record.contourPoints);
    EXPECT_EQ(record.gridCells, 361u * 441u) << "the grid of the project";
}

TEST(BenchmarkPhases, DenserGridsAreComputedOnTheProjectsSpans)
{
    QTemporaryDir directory;
    Plan plan = phasesPlan(directory.path().toStdString());
    plan.phases->sweepPoints = 4;
    plan.phases->frequencyCount = 3;
    plan.phases->phasePoints = 181;
    plan.phases->magnitudePoints = 61;
    const Record record = runCase(plan, expandCases(plan)[1]);

    EXPECT_EQ(record.status, "computed") << record.message;
    EXPECT_EQ(record.frequencies, 3u);
    EXPECT_EQ(record.gridCells, 181u * 61u);
    EXPECT_EQ(record.cloudPoints % 3u, 0u);
    EXPECT_EQ((record.cloudPoints / 3u) % 4u, 0u) << "a grid of four points per parameter";
}

TEST(BenchmarkPhases, AComparisonWithTheSameEnginesFindsNothingApart)
{
    const ProviderInstalled installed(std::make_unique<ProcessorEnginesProvider>());

    QTemporaryDir directory;
    Plan plan = phasesPlan(directory.path().toStdString());
    plan.phases->loopShaping = true;
    plan.algorithms = {mc2};
    plan.epsilons = {0.5};
    const std::vector<Case> cases = expandCases(plan);
    const Record record = runCase(plan, cases.back());

    ASSERT_EQ(record.status, "solved") << record.message;
    EXPECT_EQ(record.processor, "compare");
    EXPECT_EQ(record.environment.gpuState, "usable");
    const Comparison & c = record.comparison;
    ASSERT_TRUE(c.made);
    EXPECT_GT(c.frequencies, 0u);
    EXPECT_EQ(c.cloudSizesDiffer, 0u);
    EXPECT_EQ(c.cloudLargestRelative, 0.0);
    EXPECT_EQ(c.contourPointsDiffer, 0u);
    EXPECT_EQ(c.contourLargestHausdorff, 0.0);
    EXPECT_EQ(c.reportsDiffer, 0u);
    EXPECT_EQ(c.epsilonsDiffer, 0u);
    EXPECT_EQ(c.columnsDiffer, 0u);
    EXPECT_EQ(c.columnLargestDb, 0.0);
    EXPECT_EQ(c.traceSetsDiffer, 0u);
    EXPECT_EQ(c.traceLargest, 0.0);
    EXPECT_EQ(c.unionsDiffer, 0u);
    EXPECT_EQ(c.unionLargest, 0.0);
    EXPECT_EQ(c.cpuStatus, "solved");
    EXPECT_EQ(c.gpuStatus, "solved");
    EXPECT_EQ(c.gainRelative, 0.0);
    EXPECT_TRUE(c.sameDesign);
}

TEST(BenchmarkPhases, AComparisonWithoutGpuSupportFailsWithTheReason)
{
    ASSERT_EQ(gpuProvider(), nullptr);

    QTemporaryDir directory;
    const Plan plan = phasesPlan(directory.path().toStdString());
    const Record record = runCase(plan, expandCases(plan).back());

    EXPECT_EQ(record.status, "error");
    EXPECT_EQ(record.message, "This build of QFTbx has no GPU support: compute on the processor.");
    EXPECT_EQ(record.environment.gpuState, "no GPU support");
    EXPECT_FALSE(record.comparison.made);
}

TEST(BenchmarkPhases, TheComparisonNoticesWhatMoved)
{
    PhaseOutputs cpu;
    cpu.clouds = {{{1.0, 1.0}, {2.0, 0.0}}, {{0.5, -0.5}}};
    cpu.contours = {{{1.0, 1.0}, {2.0, 0.0}}, {{0.5, -0.5}}};
    cpu.epsilon = {0.5, 0.5};
    cpu.reports = {ContourReport{}, ContourReport{}};
    PhaseOutputs gpu = cpu;

    Comparison same;
    comparePhases(cpu, gpu, same);
    EXPECT_EQ(same.cloudLargestRelative, 0.0);
    EXPECT_EQ(same.contourPointsDiffer, 0u);
    EXPECT_TRUE(std::isnan(same.columnLargestDb)) << "no boundaries, no columns compared";

    gpu.clouds[0][1] = {2.0, 1e-12};
    gpu.contours[1].push_back({0.75, -0.5});
    gpu.reports[1].relaxed = true;
    gpu.epsilon[0] = 0.6;
    Comparison moved;
    comparePhases(cpu, gpu, moved);
    EXPECT_NEAR(moved.cloudLargestRelative, 0.5e-12, 1e-24);
    EXPECT_EQ(moved.contourPointsDiffer, 1u);
    EXPECT_DOUBLE_EQ(moved.contourLargestHausdorff, 0.25 / 0.5);
    EXPECT_EQ(moved.reportsDiffer, 1u);
    EXPECT_EQ(moved.epsilonsDiffer, 1u);

    gpu.clouds.pop_back();
    Comparison fewer;
    comparePhases(cpu, gpu, fewer);
    EXPECT_EQ(fewer.cloudSizesDiffer, 1u);
}

TEST(BenchmarkPhases, TracesAreComparedByTheirShapeAndTheirLargestGap)
{
    const auto boundariesWith = [](const Trace & trace) {
        BoundarySet set(1);
        set[0]["Stability"] = {trace};
        return BoundaryData(set, {false}, {true}, 3, Range(-360.0, 0.0), {trace}, {}, 3, Range(-20.0, 20.0));
    };
    const Trace trace{{-180.0, 1.0}, {-170.0, 2.0}, {-160.0, 3.0}};
    PhaseOutputs cpu;
    cpu.boundaries = boundariesWith(trace);

    PhaseOutputs gpu;
    Trace moved = trace;
    moved[1].magnitude += 1e-8;
    gpu.boundaries = boundariesWith(moved);
    Comparison near;
    comparePhases(cpu, gpu, near);
    EXPECT_EQ(near.traceSetsDiffer, 0u);
    EXPECT_NEAR(near.traceLargest, 1e-8, 1e-15);
    EXPECT_EQ(near.unionsDiffer, 0u);
    EXPECT_NEAR(near.unionLargest, 1e-8, 1e-15);

    Trace longer = trace;
    longer.push_back({-150.0, 4.0});
    gpu.boundaries = boundariesWith(longer);
    Comparison reshaped;
    comparePhases(cpu, gpu, reshaped);
    EXPECT_EQ(reshaped.traceSetsDiffer, 1u);
    EXPECT_EQ(reshaped.unionsDiffer, 1u);
}

TEST(BenchmarkPhases, ThePhasesAndTheComparisonSurviveTheirJson)
{
    Record record;
    record.caseId = "compare-phases-r1";
    record.status = "computed";
    record.processor = "compare";
    record.templatesMilliseconds = 12.5;
    record.gpuStartMilliseconds = 180.0;
    record.stepMilliseconds = {{"templates/sweep", 3.5}, {"boundaries/sheets (OpenMP)", 9.0}};
    record.frequencies = 9;
    record.cloudPoints = 900;
    record.contourPoints = 120;
    record.gridCells = 43681;
    record.environment.gpuState = "usable";
    record.environment.gpuName = "Stand-in";
    record.environment.gpuCompute = "8.6";
    record.environment.gpuDoublePrecisionRatio = 64;
    record.comparison.made = true;
    record.comparison.frequencies = 9;
    record.comparison.contourPointsDiffer = 2;
    record.comparison.cloudLargestRelative = 3e-16;
    record.comparison.columnsDiffer = 1;
    record.comparison.cpuStatus = "solved";
    record.comparison.gpuStatus = "solved";
    record.comparison.gainRelative = 1e-15;

    const Record back = recordFromJson(toJson(record));
    EXPECT_EQ(back.processor, "compare");
    EXPECT_EQ(back.templatesMilliseconds, 12.5);
    EXPECT_TRUE(std::isnan(back.boundariesMilliseconds));
    EXPECT_EQ(back.gpuStartMilliseconds, 180.0);
    EXPECT_EQ(stepsOf(back), stepsOf(record));
    EXPECT_EQ(back.frequencies, 9u);
    EXPECT_EQ(back.cloudPoints, 900u);
    EXPECT_EQ(back.contourPoints, 120u);
    EXPECT_EQ(back.gridCells, 43681u);
    EXPECT_EQ(back.environment.gpuName, "Stand-in");
    EXPECT_EQ(back.environment.gpuCompute, "8.6");
    EXPECT_EQ(back.environment.gpuDoublePrecisionRatio, 64);
    ASSERT_TRUE(back.comparison.made);
    EXPECT_EQ(back.comparison.contourPointsDiffer, 2u);
    EXPECT_EQ(back.comparison.cloudLargestRelative, 3e-16);
    EXPECT_TRUE(std::isnan(back.comparison.columnLargestDb));
    EXPECT_EQ(back.comparison.columnsDiffer, 1u);
    EXPECT_EQ(back.comparison.gainRelative, 1e-15);

    Record plain;
    plain.status = "solved";
    const Record plainBack = recordFromJson(toJson(plain));
    EXPECT_EQ(plainBack.processor, "");
    EXPECT_FALSE(plainBack.comparison.made);
    EXPECT_TRUE(plainBack.environment.gpuState.empty());
}
