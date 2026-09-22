/**
 * @file
 * @brief The verifier closes the loop with every plant of the sweep.
 *
 * The four plant forms give the same polynomials their evaluation does; the
 * magnetic levitation benchmark tells apart the design that stabilises the
 * whole family from the one the specifications alone let through (42 of the
 * 121 plants unstable, the case that made the check necessary); a project
 * without a sweep record, a loop with a delay and a plant that is not
 * rational are reported as not checked and never approved as stable.
 */

#include <gtest/gtest.h>

#include <complex>
#include <filesystem>
#include <string>
#include <vector>

#include "src/app/project_controller.h"
#include "src/core/loopshaping/common/specification_checker.h"
#include "src/core/math/polynomial.h"
#include "src/core/math/sequences.h"
#include "src/core/specifications/specification_record.h"
#include "src/core/system/free_form.h"
#include "src/core/system/polynomial_form.h"
#include "src/core/system/time_constant_gain.h"
#include "src/core/system/zero_pole_gain.h"

using namespace qftbx;

namespace {

std::complex<double> at(const std::vector<double> & polynomial, std::complex<double> s)
{
    std::complex<double> value(0.0, 0.0);
    for (const double coefficient : polynomial) {
        value = value * s + coefficient;
    }
    return value;
}

void expectPolynomialsMatchEvaluation(LtiSystem & system)
{
    std::vector<double> numerator, denominator;
    for (const Parameter & p : system.numerator()) numerator.push_back(p.nominal());
    for (const Parameter & p : system.denominator()) denominator.push_back(p.nominal());
    const double gain = system.gain().nominal();

    const std::optional<LtiSystem::Polynomials> polynomials = system.polynomialsAt(numerator, denominator, gain);
    ASSERT_TRUE(polynomials.has_value()) << system.expression();

    for (const double w : {0.1, 1.0, 3.7, 40.0}) {
        const std::complex<double> s(0.0, w);
        const std::complex<double> fromPolynomials = at(polynomials->numerator, s) / at(polynomials->denominator, s);
        const std::complex<double> direct = system.valueAt(w, numerator, denominator, gain, 0.0);
        EXPECT_NEAR(std::abs(fromPolynomials - direct), 0.0, 1e-9 * std::abs(direct)) << system.expression() << " at w = " << w;
    }
}

std::string example(const char * name)
{
    return (std::filesystem::path(QFTBX_EXAMPLES_DIR) / name).string();
}

ParameterGrids gridsOf(LtiSystem & plant, std::size_t points)
{
    ParameterGrids grids;
    const auto take = [&](const Parameter & parameter) {
        if (parameter.isUncertain() && grids.count(parameter.name()) == 0) {
            grids[parameter.name()] = math::linspace(parameter.range().min, parameter.range().max, points);
        }
    };
    for (const Parameter & p : plant.numerator()) take(p);
    for (const Parameter & p : plant.denominator()) take(p);
    take(plant.gain());
    return grids;
}

}

TEST(PolynomialArithmetic, ProductAndSumHighestDegreeFirst)
{
    EXPECT_EQ(math::polynomialProduct({1.0, 2.0}, {1.0, 3.0}), (std::vector<double>{1.0, 5.0, 6.0}));
    EXPECT_EQ(math::polynomialProduct({}, {1.0, 3.0}), (std::vector<double>{1.0, 3.0}));
    EXPECT_EQ(math::polynomialSum({1.0, 2.0, 3.0}, {4.0, 5.0}), (std::vector<double>{1.0, 6.0, 8.0}));
    EXPECT_EQ(math::polynomialSum({4.0, 5.0}, {1.0, 2.0, 3.0}), (std::vector<double>{1.0, 6.0, 8.0}));
}

TEST(PlantPolynomials, EveryFormAgreesWithItsEvaluation)
{
    std::vector<Parameter> zeros{Parameter(std::string("z"), 2.0)};
    std::vector<Parameter> poles{Parameter(std::string("p1"), 0.5), Parameter(std::string("p2"), 30.0)};
    ZeroPoleGain zpk("zpk", zeros, poles, Parameter(7.0), Parameter(0.0));
    expectPolynomialsMatchEvaluation(zpk);

    TimeConstantGain tcg("tcg", zeros, poles, Parameter(7.0), Parameter(0.0));
    expectPolynomialsMatchEvaluation(tcg);

    std::vector<Parameter> numerator{Parameter(std::string("b1"), 1.0), Parameter(std::string("b0"), 2.0)};
    std::vector<Parameter> denominator{Parameter(std::string("a2"), 1.0), Parameter(std::string("a1"), 30.5), Parameter(std::string("a0"), 15.0)};
    PolynomialForm polynomial("poly", numerator, denominator, Parameter(7.0), Parameter(0.0));
    expectPolynomialsMatchEvaluation(polynomial);

    std::vector<Parameter> none;
    std::vector<Parameter> a{Parameter(std::string("a"), 430.25)};
    FreeForm free("free", none, a, Parameter(877.5), Parameter(0.0), std::string("1"), std::string("s^2+a"));
    expectPolynomialsMatchEvaluation(free);
    const std::optional<LtiSystem::Polynomials> maglev = free.polynomialsAt({}, {430.25}, 877.5);
    ASSERT_TRUE(maglev.has_value());
    EXPECT_EQ(maglev->denominator.size(), 3u);
    EXPECT_NEAR(maglev->denominator[2], 430.25, 1e-9);
    EXPECT_NEAR(maglev->numerator.back(), 877.5, 1e-9);

    FreeForm delayed("delayed", none, none, Parameter(1.0), Parameter(0.0), std::string("1"), std::string("s+exp(-s)"));
    EXPECT_FALSE(delayed.polynomialsAt({}, {}, 1.0).has_value());
}

TEST(FamilyStability, TheMaglevDesignsAreToldApart)
{
    const std::string file = example("maglev-lower.qft");
    if (!std::filesystem::exists(file)) {
        GTEST_SKIP() << "no published problems under " << QFTBX_EXAMPLES_DIR;
    }
    ProjectController project;
    project.load(file);
    LtiSystem * structure = project.controllerStructure();
    ASSERT_NE(structure, nullptr);
    const std::vector<double> & omega = *project.omega()->values();
    const SpecificationSet specifications = toSpecificationSet(*project.specifications());
    const ParameterGrids sweep = gridsOf(*project.plant(), 11);

    const auto controller = [&](double k, std::vector<double> zeros, std::vector<double> poles) {
        std::vector<Parameter> z, p;
        for (double v : zeros) z.emplace_back(v);
        for (double v : poles) p.emplace_back(v);
        return structure->create("designed", z, p, Parameter(k), Parameter(0.0));
    };

    std::unique_ptr<LtiSystem> stabilising = controller(0.01, {7.822421875, 366.31175373399486}, {0.01});
    const SpecificationCheck good = checkAgainstSpecifications(*stabilising, *project.plant(), omega,
                                                               project.templates(), specifications, &sweep);
    EXPECT_TRUE(good.family.checked);
    EXPECT_EQ(good.family.members, 121u);
    EXPECT_EQ(good.family.unstableMembers, 0u);
    EXPECT_LT(good.family.worstRealPart, 0.0);
    EXPECT_TRUE(good.satisfied());

    std::unique_ptr<LtiSystem> approvedByTheBounds = controller(0.01, {9.30572040929699, 697.8305848598663}, {0.014330125702369627});
    const SpecificationCheck bad = checkAgainstSpecifications(*approvedByTheBounds, *project.plant(), omega,
                                                              project.templates(), specifications, &sweep);
    EXPECT_LE(bad.worstExcessDb, 0.0) << "the specifications alone let this design through";
    EXPECT_TRUE(bad.family.checked);
    EXPECT_EQ(bad.family.members, 121u);
    EXPECT_EQ(bad.family.unstableMembers, 42u);
    EXPECT_NEAR(bad.family.worstRealPart, 0.2403, 1e-3);
    ASSERT_EQ(bad.family.worstMember.size(), 2u);
    EXPECT_FALSE(bad.satisfied());

    const SpecificationCheck unrecorded = checkAgainstSpecifications(*approvedByTheBounds, *project.plant(), omega,
                                                                     project.templates(), specifications, nullptr);
    EXPECT_FALSE(unrecorded.family.checked);
    EXPECT_EQ(unrecorded.family.notChecked, FamilyStability::NotChecked::NoSweepRecord);
    EXPECT_EQ(unrecorded.family.unstableMembers, 0u);
}

TEST(FamilyStability, ADelayLeavesTheFamilyUnchecked)
{
    const std::string file = example("maglev-lower.qft");
    if (!std::filesystem::exists(file)) {
        GTEST_SKIP() << "no published problems under " << QFTBX_EXAMPLES_DIR;
    }
    ProjectController project;
    project.load(file);
    const ParameterGrids sweep = gridsOf(*project.plant(), 3);
    std::unique_ptr<LtiSystem> delayed = project.controllerStructure()->create(
                "delayed", {Parameter(7.8), Parameter(366.3)}, {Parameter(0.01)}, Parameter(0.01), Parameter(0.2));
    const SpecificationCheck check = checkAgainstSpecifications(*delayed, *project.plant(), *project.omega()->values(),
                                                                project.templates(), toSpecificationSet(*project.specifications()), &sweep);
    EXPECT_FALSE(check.family.checked);
    EXPECT_EQ(check.family.notChecked, FamilyStability::NotChecked::Delay);
}
