/**
 * @file
 * @brief The certification funnel accepts what the verifier accepts and
 * refuses where the verifier would.
 *
 * On the DC motor of the thesis, the published design passes every step;
 * the point on the Routh limit of the family, which the Routh table calls
 * stable, is refused by the roots with the verifier's tolerance, so the
 * search cannot return what the verifier would then reject; and a point
 * far above the family's stability limit is refused by the Routh table
 * before anything else is asked. On the toolbox example, the design the search
 * returns under the nearest-node reading of the columns, which those
 * columns admit, is refused by the specifications themselves.
 */

#include <gtest/gtest.h>

#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "src/app/project_controller.h"
#include "src/core/loopshaping/common/boundary_violation_detector.h"
#include "src/core/loopshaping/common/certifier.h"
#include "src/core/loopshaping/common/common_functions.h"
#include "src/core/loopshaping/common/exact_point_check.h"
#include "src/core/loopshaping/common/family_stability_checker.h"
#include "src/core/loopshaping/common/natural_interval_extension.h"
#include "src/core/loopshaping/common/nominal_stability_checker.h"
#include "src/core/loopshaping/common/specification_checker.h"
#include "src/core/math/range.h"
#include "src/core/specifications/specification_record.h"

using namespace qftbx;

namespace {

std::string example(const char * name)
{
    return (std::filesystem::path(QFTBX_EXAMPLES_DIR) / name).string();
}

struct Funnel
{
    explicit Funnel(ProjectController & project)
        : structure(project.controllerStructure()),
          omega(project.omega()->values()),
          specifications(toSpecificationSet(*project.specifications())),
          exact(*project.plant(), structure, *omega, project.templates(), specifications),
          stability(project.plant(), omega),
          family(project.plant(), structure, project.sweepGrids()),
          certifier(exact, stability, family)
    {
    }

    LtiSystem * structure;
    std::vector<double> * omega;
    SpecificationSet specifications;
    ExactPointCheck exact;
    NominalStabilityChecker stability;
    FamilyStabilityChecker family;
    Certifier certifier;
};

PointController designOf(LtiSystem & design)
{
    PointController point;
    for (const Parameter & z : design.numerator()) point.zeros.push_back(z.nominal());
    for (const Parameter & p : design.denominator()) point.poles.push_back(p.nominal());
    point.gain = design.gain().nominal();
    return point;
}

}

TEST(Certifier, ThePublishedDcMotorDesignPassesEveryStep)
{
    const std::string file = example("dcm-T33.qft");
    if (!std::filesystem::exists(file)) {
        GTEST_SKIP() << "no published problems under " << QFTBX_EXAMPLES_DIR;
    }
    ProjectController project;
    project.load(file);
    Funnel funnel(project);
    ASSERT_TRUE(funnel.exact.usable());
    ASSERT_TRUE(funnel.family.usable());

    const PointController design = designOf(*project.loopShapingResult()->controller());
    EXPECT_TRUE(funnel.certifier.certify(design));
    EXPECT_EQ(funnel.certifier.statistics().certifications, 1u);
    EXPECT_EQ(funnel.certifier.statistics().refusedBySpecifications, 0u);
    EXPECT_EQ(funnel.family.statistics().rootVerdicts, 1u) << "an accepted point was confirmed by the roots";
}

TEST(Certifier, ThePointOnTheRouthLimitIsRefusedByTheRoots)
{
    const std::string file = example("dcm-T33.qft");
    if (!std::filesystem::exists(file)) {
        GTEST_SKIP() << "no published problems under " << QFTBX_EXAMPLES_DIR;
    }
    ProjectController project;
    project.load(file);
    Funnel funnel(project);

    PointController onTheLimit{40.8912, {1000.0}, {466.25}};
    for (int step = 0; step < 200 && !funnel.exact.admits(onTheLimit); ++step) {
        onTheLimit.gain *= 1.0 + 1e-7;
    }
    ASSERT_TRUE(funnel.exact.admits(onTheLimit)) << "the smallest gain the specifications admit at z = 1000, p = 466.25";

    const SpecificationCheck verifier = checkAgainstSpecifications(
                *systemFromPoint(funnel.structure, onTheLimit), *project.plant(), *funnel.omega,
                project.templates(), funnel.specifications, &project.sweepGrids());
    std::printf("CERTIFIER dcm-T33 (%.7f, 1000, 466.25): worst excess %.3g dB, %zu of %zu plants unstable, worst real part %.4g\n",
                onTheLimit.gain, verifier.worstExcessDb, verifier.family.unstableMembers, verifier.family.members,
                verifier.family.worstRealPart);

    EXPECT_TRUE(funnel.stability.isNominallyStable(onTheLimit));
    EXPECT_TRUE(funnel.family.isStable(onTheLimit)) << "the Routh table calls the family stable";
    EXPECT_FALSE(funnel.family.isStableByRoots(onTheLimit)) << "the roots, with the verifier's tolerance, do not";
    EXPECT_FALSE(verifier.satisfied());

    EXPECT_FALSE(funnel.certifier.certify(onTheLimit));
    EXPECT_EQ(funnel.certifier.statistics().refusedByRoots, 1u);
    EXPECT_EQ(funnel.certifier.statistics().refusedByRouth, 0u);
}

TEST(Certifier, TheFunnelStopsAtTheFirstRefusal)
{
    const std::string file = example("dcm-T33.qft");
    if (!std::filesystem::exists(file)) {
        GTEST_SKIP() << "no published problems under " << QFTBX_EXAMPLES_DIR;
    }
    ProjectController project;
    project.load(file);
    Funnel funnel(project);

    const PointController beyondTheLimit{50.0, {1000.0}, {466.25}};
    {
        Funnel probe(project);
        ASSERT_FALSE(probe.family.isStable(beyondTheLimit)) << "above the Routh limit of the family, near 40.89";
    }

    EXPECT_FALSE(funnel.certifier.certify(beyondTheLimit));
    EXPECT_EQ(funnel.certifier.statistics().refusedByRouth, 1u);
    EXPECT_EQ(funnel.family.statistics().verdicts, 1u);
    EXPECT_EQ(funnel.stability.statistics().verdicts, 0u) << "the nominal criterion was never asked";
    EXPECT_EQ(funnel.exact.statistics().verdicts, 0u) << "nor the specifications";
    EXPECT_EQ(funnel.family.statistics().rootVerdicts, 0u);
}

TEST(Certifier, WhatTheNearestColumnAdmitsTheSpecificationRefuses)
{
    const std::string file = example("toolbox-2.qft");
    if (!std::filesystem::exists(file)) {
        GTEST_SKIP() << "no published problems under " << QFTBX_EXAMPLES_DIR;
    }
    ProjectController project;
    project.load(file);
    Funnel funnel(project);
    ASSERT_NE(project.boundaries(), nullptr);

    std::vector<std::complex<double>> nominalPlantValues;
    for (const double w : *funnel.omega) {
        nominalPlantValues.push_back(project.plant()->evaluate(w));
    }
    NaturalIntervalExtension conversion;
    BoundaryViolationDetector nearest(false);
    const auto columnsAdmit = [&](const PointController & point) {
        return satisfiesBoundaries(point, funnel.omega, &conversion, &nearest, project.boundaries(), nominalPlantValues);
    };

    const PointController conservative = designOf(*project.loopShapingResult()->controller());
    EXPECT_TRUE(columnsAdmit(conservative));
    EXPECT_TRUE(funnel.certifier.certify(conservative)) << "the published design of the file passes";

    Settings published;
    published.algorithms.conservativeBoundaryColumns = false;
    project.applySettings(published);
    ASSERT_TRUE(project.computeLoopShaping(0.5, qftbx::mc2, Range(1e-9, 10.0), 100));
    const PointController nearestDesign = designOf(*project.loopShapingResult()->controller());

    const SpecificationCheck verifier = checkAgainstSpecifications(
                *systemFromPoint(funnel.structure, nearestDesign), *project.plant(), *funnel.omega,
                project.templates(), funnel.specifications);
    std::printf("CERTIFIER toolbox-2: the nearest-node search returns gain %.6f, where the worst excess is %+.4f dB\n",
                nearestDesign.gain, verifier.worstExcessDb);

    EXPECT_TRUE(columnsAdmit(nearestDesign));
    EXPECT_GT(verifier.worstExcessDb, 0.0) << "the nearest node lets through what the bound forbids";
    EXPECT_FALSE(funnel.certifier.certify(nearestDesign));
    EXPECT_EQ(funnel.certifier.statistics().refusedBySpecifications, 1u);
}
