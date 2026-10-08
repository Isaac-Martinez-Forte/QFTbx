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
 * before anything else is asked. A point below the smallest gain the
 * specifications admit is refused by them, and passes when the caller says
 * the specifications already admitted it, which is how the searches hand over
 * a point whose gain came from the specifications. On the magnetic
 * levitation problem, a loop the Routh table of the family calls stable is
 * refused by the nominal criterion, whose grid ends with the loop still above
 * 0 dB, and the funnel stops there. On the ACC'90 benchmark, with no sweep,
 * a loop the nominal criterion approves, its grid missing the crossing of
 * the -180 degree ray near 0.71 rad/s, has its nominal closed-loop poles in
 * the right half-plane, and the Routh table of the nominal plant refuses it
 * before anything else is asked; the verifier refuses it too, on the nominal
 * closed loop alone. On the toolbox example, the design the
 * search returns under the nearest-node reading of the columns, which those
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
#include "src/core/system/parameter.h"
#include "src/core/templates/parameter_grids.h"
#include "tests/backend/published_problems.h"

using namespace qftbx;
using namespace qftbx_tests;

namespace {

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
        FamilyStabilityChecker probe(project.plant(), project.controllerStructure(), project.sweepGrids());
        ASSERT_FALSE(probe.isStable(beyondTheLimit)) << "above the Routh limit of the family, near 40.89";
    }

    EXPECT_FALSE(funnel.certifier.certify(beyondTheLimit));
    EXPECT_EQ(funnel.certifier.statistics().refusedByRouth, 1u);
    EXPECT_EQ(funnel.family.statistics().verdicts, 1u);
    EXPECT_EQ(funnel.stability.statistics().verdicts, 0u) << "the nominal criterion was never asked";
    EXPECT_EQ(funnel.exact.statistics().verdicts, 0u) << "nor the specifications";
    EXPECT_EQ(funnel.family.statistics().rootVerdicts, 0u);
}

TEST(Certifier, AnAdmittedPointSkipsTheSpecifications)
{
    const std::string file = example("dcm-T33.qft");
    if (!std::filesystem::exists(file)) {
        GTEST_SKIP() << "no published problems under " << QFTBX_EXAMPLES_DIR;
    }
    ProjectController project;
    project.load(file);

    const PointController belowTheBound{40.0, {1000.0}, {466.25}};
    {
        Funnel asked(project);
        EXPECT_FALSE(asked.certifier.certify(belowTheBound)) << "below the smallest gain the specifications admit, near 40.89";
        EXPECT_EQ(asked.certifier.statistics().refusedBySpecifications, 1u);
        EXPECT_EQ(asked.certifier.statistics().refusedByRouth, 0u) << "a lower gain is further from the Routh limit";
    }

    Funnel admitted(project);
    EXPECT_TRUE(admitted.certifier.certifyAdmitted(belowTheBound));
    EXPECT_EQ(admitted.exact.statistics().verdicts, 0u) << "the specifications were never asked";
    EXPECT_EQ(admitted.family.statistics().rootVerdicts, 1u) << "the roots still confirm the point";
}

TEST(Certifier, ALoopTheNominalCriterionRefusesStopsThere)
{
    const std::string file = example("maglev-lower.qft");
    if (!std::filesystem::exists(file)) {
        GTEST_SKIP() << "no published problems under " << QFTBX_EXAMPLES_DIR;
    }
    ProjectController project;
    project.load(file);
    Funnel funnel(project);
    ASSERT_TRUE(funnel.family.usable());

    const PointController stillAboveZeroDb{728.517, {536.28, 3.41424}, {367.298}};
    {
        Funnel probe(project);
        ASSERT_TRUE(probe.family.isStable(stillAboveZeroDb)) << "the Routh table of the family calls the loop stable";
        ASSERT_FALSE(probe.stability.isNominallyStable(stillAboveZeroDb)) << "the nominal criterion does not";
    }

    EXPECT_FALSE(funnel.certifier.certify(stillAboveZeroDb));
    EXPECT_EQ(funnel.certifier.statistics().refusedByNominalStability, 1u);
    EXPECT_EQ(funnel.certifier.statistics().refusedByRouth, 0u);
    EXPECT_EQ(funnel.exact.statistics().verdicts, 0u) << "the specifications were never asked";
    EXPECT_EQ(funnel.family.statistics().rootVerdicts, 0u);
}

TEST(Certifier, TheNominalRouthRefusesALoopTheNominalCriterionApproves)
{
    const std::string file = example("acc90.qft");
    if (!std::filesystem::exists(file)) {
        GTEST_SKIP() << "no published problems under " << QFTBX_EXAMPLES_DIR;
    }
    ProjectController project;
    project.load(file);
    LtiSystem * base = project.controllerStructure();
    ASSERT_NE(base, nullptr);
    ASSERT_GE(base->denominator().size(), 1u);
    const std::vector<Parameter> poles = {base->denominator().front(), Parameter(std::string("p2"), Range(0.01, 1000.0), 0.01)};
    const std::unique_ptr<LtiSystem> structure = base->create("base+p", base->numerator(), poles, base->gain());

    std::vector<double> * omega = project.omega()->values();
    const SpecificationSet specifications = toSpecificationSet(*project.specifications());
    ExactPointCheck exact(*project.plant(), structure.get(), *omega, project.templates(), specifications);
    NominalStabilityChecker stability(project.plant(), omega);
    FamilyStabilityChecker family(project.plant(), structure.get(), ParameterGrids());
    Certifier certifier(exact, stability, family);
    ASSERT_FALSE(family.usable());

    const PointController unstable{428296.26, {12.141131223597256}, {24.303731346711288, 999.99999999999079}};
    ASSERT_TRUE(stability.isNominallyStable(unstable)) << "the nominal criterion approves the loop";
    EXPECT_TRUE(family.isStable(unstable)) << "there is no family to refuse it";
    EXPECT_FALSE(family.isStableAtNominal(unstable));
    EXPECT_FALSE(family.isStableByRoots(unstable)) << "nor do the roots accept it";

    EXPECT_FALSE(certifier.certify(unstable));
    EXPECT_EQ(certifier.statistics().refusedByNominalRouth, 1u);
    EXPECT_EQ(certifier.statistics().refusedByNominalStability, 0u);
    EXPECT_EQ(exact.statistics().verdicts, 0u) << "the specifications were never asked";

    const SpecificationCheck verifier = checkAgainstSpecifications(*systemFromPoint(structure.get(), unstable),
                                                                   *project.plant(), *omega, project.templates(),
                                                                   specifications);
    EXPECT_FALSE(verifier.family.checked);
    ASSERT_TRUE(verifier.nominal.checked);
    EXPECT_FALSE(verifier.nominal.stable);
    EXPECT_NEAR(verifier.nominal.worstRealPart, 2.22, 0.01);
    EXPECT_FALSE(verifier.satisfied()) << "the verifier refuses it too, with no sweep to close the loop over";
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
    published.research.conservativeColumns = false;
    published.research.mc2Reading = Settings::Research::PointReading::Columns;
    published.research.mc2.feasibleMagnitude = true;
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
