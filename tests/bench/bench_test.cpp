/**
 * @file
 * @brief Tests of the benchmark library, from the plan file to a measured run.
 *
 * A plan survives its XML file and a malformed one is refused with its reason;
 * it expands into structures, algorithms, epsilons and repetitions with a
 * warm-up, and a step sequence grows the base structure by a zero or a pole.
 * The spread of a sample is its median, mean, deviation and extremes; the
 * summary groups records, leaves warm-ups and failures out of the timings and
 * notices when repetitions disagree on the controller. A certificate with
 * every count and flag set survives its JSON, and so do the infinite lower
 * bounds of a search that left nothing unproven. A run of one case on the ACC'90
 * fixture must leave a complete record that survives its file, with the gain
 * of 0.001 that fixture's goldens pin.
 */

#include <gtest/gtest.h>

#include <cmath>
#include <string>
#include <vector>

#include <QDir>
#include <QTemporaryDir>

#include "src/bench/measurement.h"
#include "src/bench/plan.h"
#include "src/bench/record.h"
#include "src/bench/summary.h"
#include "src/core/common/exception.h"
#include "src/core/system/parameter.h"
#include "src/core/system/zero_pole_gain.h"

using namespace qftbx;
using namespace qftbx::bench;

namespace {

Plan smallPlan(const std::string & outputDirectory)
{
    Plan plan = examplePlan();
    plan.name = "small";
    plan.projectFile = std::string(QFTBX_TEST_DATA_DIR "/acc90.qft");
    plan.outputDirectory = outputDirectory;
    plan.algorithms = {mc2, mc_thesis};
    plan.epsilons = {0.5};
    plan.repetitions = 2;
    plan.warmUp = true;
    plan.runBase = true;
    plan.steps = {{StructureStep::Kind::Zero, 0.01, 1000.0, false}, {StructureStep::Kind::Pole, 0.01, 1000.0, true}};
    plan.settings = {{"stability.base-grid-points", "2000"}};
    return plan;
}

}

TEST(BenchmarkPlan, SurvivesItsFile)
{
    QTemporaryDir directory;
    const Plan plan = smallPlan(directory.path().toStdString());
    const std::string path = directory.filePath("plan.xml").toStdString();
    writePlan(plan, path);
    const Plan back = readPlan(path);

    EXPECT_EQ(back.name, plan.name);
    EXPECT_EQ(back.projectFile, plan.projectFile);
    EXPECT_EQ(back.algorithms, plan.algorithms);
    EXPECT_EQ(back.epsilons, plan.epsilons);
    EXPECT_EQ(back.repetitions, 2);
    EXPECT_TRUE(back.warmUp);
    ASSERT_EQ(back.steps.size(), 2u);
    EXPECT_EQ(back.steps[0].kind, StructureStep::Kind::Zero);
    EXPECT_FALSE(back.steps[0].run);
    EXPECT_EQ(back.steps[1].kind, StructureStep::Kind::Pole);
    EXPECT_DOUBLE_EQ(back.steps[1].max, 1000.0);
    ASSERT_EQ(back.settings.size(), 1u);
    EXPECT_EQ(back.settings[0].first, "stability.base-grid-points");
    EXPECT_EQ(back.measures.memoryTrace, plan.measures.memoryTrace);
}

TEST(BenchmarkPlan, AMalformedPlanIsRefusedWithItsReason)
{
    QTemporaryDir directory;
    const std::string path = directory.filePath("bad.xml").toStdString();
    {
        QFile file(QString::fromStdString(path));
        ASSERT_TRUE(file.open(QIODevice::WriteOnly));
        file.write("<QFTbench version=\"1\"><project file=\"x.qft\"/><algorithms><algorithm name=\"nx\"/></algorithms></QFTbench>");
    }
    EXPECT_THROW(readPlan(path), InvalidInput);
    EXPECT_THROW(readPlan(directory.filePath("missing.xml").toStdString()), FileError);
}

TEST(BenchmarkPlan, ExpandsIntoStructuresAlgorithmsEpsilonsAndRepetitions)
{
    const Plan plan = smallPlan(".");
    EXPECT_EQ(measuredStructures(plan), (std::vector<std::size_t>{0, 2}));
    const std::vector<Case> cases = expandCases(plan);
    ASSERT_EQ(cases.size(), 12u);
    EXPECT_TRUE(cases[0].warmUp);
    EXPECT_EQ(cases[0].repetition, 0);
    EXPECT_EQ(cases[1].repetition, 1);
    EXPECT_EQ(cases[2].repetition, 2);
    EXPECT_EQ(cases[3].algorithm, mc_thesis);
    EXPECT_EQ(cases[6].stepsApplied, 2u);
    EXPECT_EQ(caseId(cases[1]), "s0-mc2-e0_5-r1");
    EXPECT_EQ(structureLabel(plan, 2), "base+z+p");
    for (std::size_t i = 0; i < cases.size(); ++i) {
        EXPECT_EQ(cases[i].index, i);
    }
}

TEST(BenchmarkPlan, TheStructureSequenceAddsToTheBase)
{
    Plan plan = smallPlan(".");
    plan.gainRange = Range(1.0, 100.0);
    ZeroPoleGain base("c", {Parameter("z1", Range(1.0, 10.0), 1.0)}, {Parameter("p1", Range(5.0, 50.0), 5.0)},
                      Parameter("k", Range(0.1, 10.0), 0.1), Parameter(0.0));

    std::unique_ptr<LtiSystem> grown = structureAfter(plan, base, 2);
    ASSERT_EQ(grown->numerator().size(), 2u);
    ASSERT_EQ(grown->denominator().size(), 2u);
    EXPECT_EQ(grown->numerator()[1].name(), "z2");
    EXPECT_EQ(grown->denominator()[1].name(), "p2");
    EXPECT_TRUE(grown->numerator()[1].isUncertain());
    EXPECT_DOUBLE_EQ(grown->denominator()[1].range().max, 1000.0);
    EXPECT_DOUBLE_EQ(grown->gain().range().min, 1.0);
    EXPECT_DOUBLE_EQ(grown->gain().range().max, 100.0);

    std::unique_ptr<LtiSystem> same = structureAfter(plan, base, 0);
    EXPECT_EQ(same->numerator().size(), 1u);
    EXPECT_THROW(structureAfter(plan, base, 3), InvalidInput);
}

TEST(BenchmarkSummary, SpreadIsMedianMeanDeviationAndExtremes)
{
    const Spread s = spreadOf({4.0, 1.0, 3.0, 2.0});
    EXPECT_EQ(s.n, 4u);
    EXPECT_DOUBLE_EQ(s.median, 2.5);
    EXPECT_DOUBLE_EQ(s.mean, 2.5);
    EXPECT_NEAR(s.standardDeviation, std::sqrt(5.0 / 3.0), 1e-12);
    EXPECT_DOUBLE_EQ(s.min, 1.0);
    EXPECT_DOUBLE_EQ(s.max, 4.0);
    EXPECT_NEAR(s.coefficientOfVariation, std::sqrt(5.0 / 3.0) / 2.5, 1e-12);
    EXPECT_EQ(spreadOf({}).n, 0u);
}

TEST(BenchmarkSummary, GroupsRecordsAndLeavesWarmUpsAndFailuresOut)
{
    std::vector<Record> records;
    const auto make = [&](int repetition, bool warmUp, const std::string & status, double ms, const std::string & digest) {
        Record r;
        r.stepsApplied = 1;
        r.structure = "base+z";
        r.algorithm = "nt";
        r.epsilon = 2.0;
        r.repetition = repetition;
        r.warmUp = warmUp;
        r.status = status;
        r.wallMilliseconds = ms;
        r.digest = digest;
        r.statistics.peakLiveNodes = 7;
        records.push_back(r);
    };
    make(0, true, "solved", 900.0, "a");
    make(1, false, "solved", 100.0, "a");
    make(2, false, "solved", 120.0, "a");
    make(3, false, "timeout", 0.0, "");
    make(4, false, "solved", 110.0, "b");

    const std::vector<Aggregate> aggregates = summarize(records);
    ASSERT_EQ(aggregates.size(), 1u);
    const Aggregate & a = aggregates.front();
    EXPECT_EQ(a.runs, 4u);
    EXPECT_EQ(a.solved, 3u);
    EXPECT_EQ(a.failed, 1u);
    EXPECT_DOUBLE_EQ(a.wallMilliseconds.median, 110.0);
    EXPECT_EQ(a.wallMilliseconds.n, 3u);
    EXPECT_FALSE(a.resultsAgree) << "one repetition returned another controller";
    EXPECT_TRUE(a.countersAgree);
    EXPECT_EQ(a.statistics.peakLiveNodes, 7u);
    EXPECT_NE(markdownTable(aggregates).find("NO"), std::string::npos);
}

TEST(BenchmarkRecord, TheCertificateSurvivesItsJson)
{
    Record record;
    LoopShapingStatistics::Certificate & c = record.statistics.certificate;
    c.kept = true;
    c.finished = true;
    c.exactPoints = true;
    c.lowerBound = 556.5;
    c.lowerBoundStrict = 550.25;
    c.residueNodes = 1;
    c.epsilonResolved = 2;
    c.unprovenDiscards = 3;
    c.gridBackedPrunes = 4;
    c.familyPrunes = 5;
    c.nominalBoxPrunes = 101;
    c.provenInfeasible = 6;
    c.columnsOverruled = 7;
    c.certifiedCuts = 8;
    c.sectorVerdicts = 9;
    c.certifications = 10;
    c.refusedByRouth = 11;
    c.refusedByNominalRouth = 104;
    c.refusedByNominalStability = 12;
    c.refusedBySpecifications = 13;
    c.refusedByRoots = 14;
    c.incumbentUpdates = 15;
    c.kernelPasses = 16;
    c.gainSearches = 17;
    c.exchangeRounds = 18;
    c.ladderSteps = 19;
    c.laddersExhausted = 102;
    c.roundLimitsReached = 103;
    c.largestWorkingSet = 20;

    const Record back = recordFromJson(toJson(record));
    const LoopShapingStatistics::Certificate & b = back.statistics.certificate;
    EXPECT_TRUE(b.kept);
    EXPECT_TRUE(b.finished);
    EXPECT_TRUE(b.exactPoints);
    EXPECT_EQ(b.lowerBound, c.lowerBound);
    EXPECT_EQ(b.lowerBoundStrict, c.lowerBoundStrict);
    EXPECT_EQ(b.residueNodes, c.residueNodes);
    EXPECT_EQ(b.epsilonResolved, c.epsilonResolved);
    EXPECT_EQ(b.unprovenDiscards, c.unprovenDiscards);
    EXPECT_EQ(b.gridBackedPrunes, c.gridBackedPrunes);
    EXPECT_EQ(b.familyPrunes, c.familyPrunes);
    EXPECT_EQ(b.nominalBoxPrunes, c.nominalBoxPrunes);
    EXPECT_EQ(b.provenInfeasible, c.provenInfeasible);
    EXPECT_EQ(b.columnsOverruled, c.columnsOverruled);
    EXPECT_EQ(b.certifiedCuts, c.certifiedCuts);
    EXPECT_EQ(b.sectorVerdicts, c.sectorVerdicts);
    EXPECT_EQ(b.certifications, c.certifications);
    EXPECT_EQ(b.refusedByRouth, c.refusedByRouth);
    EXPECT_EQ(b.refusedByNominalRouth, c.refusedByNominalRouth);
    EXPECT_EQ(b.refusedByNominalStability, c.refusedByNominalStability);
    EXPECT_EQ(b.refusedBySpecifications, c.refusedBySpecifications);
    EXPECT_EQ(b.refusedByRoots, c.refusedByRoots);
    EXPECT_EQ(b.incumbentUpdates, c.incumbentUpdates);
    EXPECT_EQ(b.kernelPasses, c.kernelPasses);
    EXPECT_EQ(b.gainSearches, c.gainSearches);
    EXPECT_EQ(b.exchangeRounds, c.exchangeRounds);
    EXPECT_EQ(b.ladderSteps, c.ladderSteps);
    EXPECT_EQ(b.laddersExhausted, c.laddersExhausted);
    EXPECT_EQ(b.roundLimitsReached, c.roundLimitsReached);
    EXPECT_EQ(b.largestWorkingSet, c.largestWorkingSet);

    const Record unbounded = recordFromJson(toJson(Record()));
    EXPECT_FALSE(unbounded.statistics.certificate.kept);
    EXPECT_FALSE(unbounded.statistics.certificate.finished);
    EXPECT_FALSE(unbounded.statistics.certificate.exactPoints);
    EXPECT_TRUE(std::isinf(unbounded.statistics.certificate.lowerBound));
    EXPECT_TRUE(std::isinf(unbounded.statistics.certificate.lowerBoundStrict));
    EXPECT_EQ(unbounded.statistics.certificate.largestWorkingSet, 0u);
}

TEST(BenchmarkMeasurement, ARunOfACaseLeavesACompleteRecord)
{
    QTemporaryDir directory;
    Plan plan = smallPlan(directory.path().toStdString());
    plan.measures.memoryTrace = true;
    const std::vector<Case> cases = expandCases(plan);

    const Record record = runCase(plan, cases[1]);
    EXPECT_EQ(record.status, "solved") << record.message;
    EXPECT_EQ(record.algorithm, "mc2");
    EXPECT_EQ(record.structure, "base");
    EXPECT_GT(record.wallMilliseconds, 0.0);
    EXPECT_GE(record.cpuMilliseconds, 0.0);
    EXPECT_GT(record.peakMemoryBytes, 0u);
    EXPECT_FALSE(record.memoryTrace.empty());
    EXPECT_GE(record.statistics.nodesProcessed, 1u);
    EXPECT_NEAR(record.gain, 0.001, 1e-9);
    EXPECT_EQ(record.digest, digestOf(record.gain, record.zeros, record.poles));
    EXPECT_FALSE(record.environment.hostname.empty());
    EXPECT_EQ(record.environment.cores > 0, true);

    QDir().mkpath(QString::fromStdString(recordsDirectory(plan)));
    writeRecord(record, recordPath(plan, cases[1]));
    const Record back = readRecord(recordPath(plan, cases[1]));
    EXPECT_EQ(back.digest, record.digest);
    EXPECT_EQ(back.statistics.peakLiveNodes, record.statistics.peakLiveNodes);
    EXPECT_EQ(back.memoryTrace.size(), record.memoryTrace.size());
    EXPECT_EQ(readRecords(recordsDirectory(plan)).size(), 1u);

    const Record grown = runCase(plan, cases[7]);
    EXPECT_EQ(grown.structure, "base+z+p");
    EXPECT_EQ(grown.poles.size(), 2u) << grown.message;
}
