/**
 * @file
 * @brief The exact proof that a whole box meets the specifications, and
 * what MC2 does with it.
 *
 * On the published problems, annular sectors around loop values the
 * specifications admit, and random ones, are asked of
 * ExactPointCheck::sectorFeasible; a sector it proves is sampled densely and
 * every sample meets every specification, by the verifier's own kernel,
 * with the tolerance to spare, and the sector verdict never proves the same
 * sector infeasible nor forbids a magnitude of it. Proofs and refusals both
 * occur. A region proven for a box changes no verdict of admits and no
 * lowest admissible gain for points inside the box, and skips some
 * frequencies. MC2 with research.mc2.exact-history and
 * research.mc2.exact-qsfact returns a design the verifier accepts, whose
 * gain no certificate contradicts: its own lower bound and that of the
 * default run are both at or below the other run's gain.
 */

#include <gtest/gtest.h>

#include <cmath>
#include <complex>
#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include "src/app/project_controller.h"
#include "src/core/loopshaping/common/exact_point_check.h"
#include "src/core/loopshaping/common/natural_interval_extension.h"
#include "src/core/loopshaping/common/specification_checker.h"
#include "src/core/math/constants.h"
#include "src/core/math/range.h"
#include "src/core/project/settings.h"
#include "src/core/specifications/specification.h"
#include "src/core/specifications/specification_record.h"
#include "tests/backend/published_problems.h"

using namespace qftbx;
using namespace qftbx_tests;

namespace {

const char * const kProblems[] = {"dcm-T33-plant-k.qft", "toolbox-2.qft", "dcm-k-plant-k.qft", "toolbox-1.qft"};

std::complex<double> loopOf(LtiSystem & structure, const FrequencyReference & at, const PointController & point)
{
    return structure.valueAt(at.omega, point.zeros, point.poles, point.gain, structure.delay().nominal()) * at.nominalPlant;
}

std::optional<PointController> admittedPoint(ExactPointCheck & check, LtiSystem & structure, std::mt19937 & generator)
{
    PointController point = randomPoint(structure, generator);
    const ExactPointCheck::GainSearch search = check.lowestAdmissibleGain(point.zeros, point.poles,
                                                                        structure.gain().range());
    if (!search.gain.has_value()) {
        return std::nullopt;
    }
    point.gain = std::min(*search.gain * 1.3, structure.gain().range().max);
    if (!check.admits(point)) {
        return std::nullopt;
    }
    return point;
}

std::unique_ptr<LtiSystem> boxAround(LtiSystem & structure, const PointController & point, double relativeWidth)
{
    const auto around = [&](const Parameter & parameter, double centre) {
        const Range within = parameter.isUncertain() ? parameter.range() : Range(parameter.nominal(), parameter.nominal());
        return Range(std::max(within.min, centre / (1.0 + relativeWidth)), std::min(within.max, centre * (1.0 + relativeWidth)));
    };
    std::vector<Parameter> zeros, poles;
    for (std::size_t j = 0; j < structure.numerator().size(); ++j) {
        const Range range = around(structure.numerator()[j], point.zeros[j]);
        zeros.emplace_back(structure.numerator()[j].name(), range, range.min);
    }
    for (std::size_t j = 0; j < structure.denominator().size(); ++j) {
        const Range range = around(structure.denominator()[j], point.poles[j]);
        poles.emplace_back(structure.denominator()[j].name(), range, range.min);
    }
    const Range gain = around(structure.gain(), point.gain);
    return structure.create("box", std::move(zeros), std::move(poles), Parameter(structure.gain().name(), gain, gain.min),
                            structure.delay());
}

}

TEST(ExactFeasible, WhatItProvesFeasibleMeetsTheSpecificationsEverywhere)
{
    for (const char * name : kProblems) {
        const std::string file = problem(name);
        if (!std::filesystem::exists(file)) {
            GTEST_SKIP() << "no published problems under " << QFTBX_EXAMPLES_DIR;
        }
        ProjectController project;
        project.load(file);
        LtiSystem * structure = project.controllerStructure();
        const std::vector<double> & omega = *project.omega()->values();
        const SpecificationSet specifications = toSpecificationSet(*project.specifications());
        ExactPointCheck check(*project.plant(), structure, omega, project.templates(), specifications);
        ASSERT_TRUE(check.usable());
        const SpecificationReference reference(*project.plant(), omega, project.templates(), specifications);

        std::mt19937 generator(37);
        std::uniform_real_distribution<double> unit(0.0, 1.0);
        std::size_t proven = 0, refused = 0, sampled = 0;

        for (int trial = 0; trial < 40; ++trial) {
            const std::optional<PointController> point = admittedPoint(check, *structure, generator);
            if (!point.has_value()) {
                continue;
            }
            for (const FrequencyReference & at : reference.frequencies()) {
                const std::complex<double> loop = loopOf(*structure, at, *point);
                const double phaseDeg = std::arg(loop) * 180.0 / math::kPi;
                const double db = 20.0 * std::log10(std::abs(loop));
                const double halfPhase = std::pow(10.0, -3.0 + 4.5 * unit(generator));
                const double halfDb = std::pow(10.0, -3.0 + 4.5 * unit(generator));
                const Range phase(phaseDeg - halfPhase, phaseDeg + halfPhase);
                const Range magnitude(db - halfDb, db + halfDb);

                if (!check.sectorFeasible(at.index, phase, magnitude)) {
                    ++refused;
                    continue;
                }
                ++proven;
                const ExactPointCheck::SectorVerdict verdict = check.sectorVerdict(at.index, phase, magnitude);
                EXPECT_FALSE(verdict.provablyInfeasible) << name << " w=" << at.omega;
                EXPECT_LT(verdict.forbiddenBelowDb, magnitude.min) << name << " w=" << at.omega;
                EXPECT_GT(verdict.forbiddenAboveDb, magnitude.max) << name << " w=" << at.omega;
                for (int a = 0; a <= 12; ++a) {
                    for (int b = 0; b <= 12; ++b) {
                        const double p = phase.min + (phase.max - phase.min) * a / 12.0;
                        const double m = magnitude.min + (magnitude.max - magnitude.min) * b / 12.0;
                        const std::complex<double> value = std::polar(std::pow(10.0, m / 20.0), p * math::kPi / 180.0);
                        EXPECT_LE(at.worstExcessAt(value), -ExactPointCheck::kToleranceDb)
                            << name << " w=" << at.omega << " at " << p << " deg, " << m << " dB";
                        ++sampled;
                    }
                }
            }
        }
        std::printf("EXACT-FEASIBLE %-20s %zu sectors proven, %zu refused, %zu loop values sampled\n", name, proven,
                    refused, sampled);
        EXPECT_GT(proven, 0u) << name;
        EXPECT_GT(refused, 0u) << name;
    }
}

TEST(ExactFeasible, ARandomSectorIsNeverProvenBothWays)
{
    std::size_t asked = 0, proven = 0;
    for (const char * name : kProblems) {
        const std::string file = problem(name);
        if (!std::filesystem::exists(file)) {
            GTEST_SKIP() << "no published problems under " << QFTBX_EXAMPLES_DIR;
        }
        ProjectController project;
        project.load(file);
        const std::vector<double> & omega = *project.omega()->values();
        const SpecificationSet specifications = toSpecificationSet(*project.specifications());
        ExactPointCheck check(*project.plant(), project.controllerStructure(), omega, project.templates(), specifications);
        ASSERT_TRUE(check.usable());

        std::mt19937 generator(41);
        std::uniform_real_distribution<double> unit(0.0, 1.0);
        for (std::size_t f = 0; f < omega.size(); ++f) {
            for (int trial = 0; trial < 200; ++trial) {
                const double centrePhase = -360.0 * unit(generator);
                const double centreDb = -40.0 + 100.0 * unit(generator);
                const double halfPhase = std::pow(10.0, -2.0 + 3.0 * unit(generator));
                const double halfDb = std::pow(10.0, -2.0 + 2.5 * unit(generator));
                const Range phase(centrePhase - halfPhase, centrePhase + halfPhase);
                const Range magnitude(centreDb - halfDb, centreDb + halfDb);
                ++asked;
                if (!check.sectorFeasible(f, phase, magnitude)) {
                    continue;
                }
                ++proven;
                const ExactPointCheck::SectorVerdict verdict = check.sectorVerdict(f, phase, magnitude);
                EXPECT_FALSE(verdict.provablyInfeasible) << name << " frequency " << f;
                EXPECT_LT(verdict.forbiddenBelowDb, magnitude.min) << name << " frequency " << f;
                EXPECT_GT(verdict.forbiddenAboveDb, magnitude.max) << name << " frequency " << f;
            }
        }
    }
    std::printf("EXACT-FEASIBLE random sectors: %zu asked, %zu proven\n", asked, proven);
    EXPECT_GT(proven, 0u);
}

TEST(ExactFeasible, AProvenRegionChangesNoVerdictInsideItsBox)
{
    std::size_t skips = 0, compared = 0;
    for (const char * name : kProblems) {
        const std::string file = problem(name);
        if (!std::filesystem::exists(file)) {
            GTEST_SKIP() << "no published problems under " << QFTBX_EXAMPLES_DIR;
        }
        ProjectController project;
        project.load(file);
        LtiSystem * structure = project.controllerStructure();
        const std::vector<double> & omega = *project.omega()->values();
        const SpecificationSet specifications = toSpecificationSet(*project.specifications());
        ExactPointCheck regioned(*project.plant(), structure, omega, project.templates(), specifications);
        ExactPointCheck plain(*project.plant(), structure, omega, project.templates(), specifications);
        NaturalIntervalExtension extension;

        std::mt19937 generator(43);
        for (int trial = 0; trial < 30; ++trial) {
            const std::optional<PointController> point = admittedPoint(plain, *structure, generator);
            if (!point.has_value()) {
                continue;
            }
            std::unique_ptr<LtiSystem> box = boxAround(*structure, *point, 1e-3);

            ExactPointCheck::ProvenRegion region;
            region.frequencies.assign(omega.size(), 0);
            bool any = false;
            for (std::size_t i = 0; i < omega.size(); ++i) {
                const NicholsBox projection = extension.nicholsBox(box.get(), omega[i], project.plant()->evaluate(omega[i]));
                if (regioned.sectorFeasible(i, Range(projection.phaseDegrees.lower(), projection.phaseDegrees.upper()),
                                            Range(projection.magnitudeDb.lower(), projection.magnitudeDb.upper()))) {
                    region.frequencies[i] = 1;
                    any = true;
                }
            }
            if (!any) {
                continue;
            }
            region.gain = box->gain().range();
            for (const Parameter & z : box->numerator()) region.zeros.push_back(z.range());
            for (const Parameter & p : box->denominator()) region.poles.push_back(p.range());
            regioned.setProvenRegion(region);

            for (int sample = 0; sample < 20; ++sample) {
                const PointController inside = pointInside(*box, generator);
                EXPECT_EQ(regioned.admits(inside), plain.admits(inside)) << name << " trial " << trial;
                ++compared;
            }
            std::vector<double> zeros, poles;
            for (const Parameter & z : box->numerator()) zeros.push_back(z.range().middle());
            for (const Parameter & p : box->denominator()) poles.push_back(p.range().middle());
            const std::optional<double> a = regioned.lowestAdmissibleGain(zeros, poles, box->gain().range()).gain;
            const std::optional<double> b = plain.lowestAdmissibleGain(zeros, poles, box->gain().range()).gain;
            ASSERT_EQ(a.has_value(), b.has_value()) << name << " trial " << trial;
            if (a.has_value()) {
                EXPECT_NEAR(*a, *b, std::abs(*b) * 1e-12) << name << " trial " << trial;
            }
            regioned.clearProvenRegion();
        }
        skips += regioned.statistics().provenSkips;
    }
    std::printf("EXACT-FEASIBLE regions: %zu points compared, %zu frequencies skipped\n", compared, skips);
    EXPECT_GT(compared, 0u);
    EXPECT_GT(skips, 0u);
}

namespace {

struct Mc2Run {
    double gain = 0.0;
    double lowerBound = 0.0;
    std::size_t provenFrequencies = 0;
};

Mc2Run runMc2(const char * name, bool history, bool qsfact)
{
    ProjectController controller;
    controller.load(problem(name));
    Settings settings;
    settings.research.mc2Reading = Settings::Research::PointReading::Exact;
    settings.research.mc2ExactHistory = history;
    settings.research.mc2ExactQsfact = qsfact;
    controller.applySettings(settings);

    Mc2Run run;
    EXPECT_TRUE(controller.computeLoopShaping(0.5, qftbx::mc2, Range(1e-9, 10.0), 100)) << name;
    const LoopShapingResult * result = controller.loopShapingResult();
    if (result == nullptr || !result->check().has_value()) {
        ADD_FAILURE() << name << " returned no checked design";
        return run;
    }
    const SpecificationCheck & check = *result->check();
    EXPECT_TRUE(check.satisfied()) << name << " worst excess " << check.worstExcessDb << " dB";
    EXPECT_EQ(check.family.unstableMembers, 0u) << name;
    run.gain = result->controller()->gain().nominal();
    run.lowerBound = result->statistics().certificate.lowerBound;
    run.provenFrequencies = result->statistics().certificate.provenFrequencies;
    EXPECT_LE(run.lowerBound, run.gain) << name;
    return run;
}

}

TEST(ExactFeasible, Mc2WithTheProofsReturnsADesignNoCertificateContradicts)
{
    std::size_t proven = 0;
    for (const char * name : {"dcm-T33-plant-k.qft", "toolbox-2.qft", "dcm-k-plant-k.qft"}) {
        if (!std::filesystem::exists(problem(name))) {
            GTEST_SKIP() << "no published problems under " << QFTBX_EXAMPLES_DIR;
        }
        const Mc2Run plain = runMc2(name, false, false);
        EXPECT_EQ(plain.provenFrequencies, 0u) << name;
        const std::vector<std::pair<bool, bool>> switches = {{true, false}, {false, true}, {true, true}};
        for (const auto & [history, qsfact] : switches) {
            const Mc2Run proved = runMc2(name, history, qsfact);
            EXPECT_LE(proved.lowerBound, plain.gain) << name << " history " << history << " qsfact " << qsfact;
            EXPECT_LE(plain.lowerBound, proved.gain) << name << " history " << history << " qsfact " << qsfact;
            std::printf("EXACT-FEASIBLE %-20s history %d qsfact %d: gain %.10g (default %.10g), %zu frequencies proven\n",
                        name, history, qsfact, proved.gain, plain.gain, proved.provenFrequencies);
            proven += proved.provenFrequencies;
        }
    }
    EXPECT_GT(proven, 0u);
}
