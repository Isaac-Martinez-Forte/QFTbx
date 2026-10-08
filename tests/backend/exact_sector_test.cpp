/**
 * @file
 * @brief The exact sector verdict never discards a loop value that meets
 * the specifications.
 *
 * On the published problems, random annular sectors of the loop's
 * enclosure at every design frequency are asked of the verdict; a sector
 * proven infeasible is sampled densely and every sample violates some
 * specification by the verifier's own kernel, and every magnitude in a strip
 * the verdict certifies forbidden, at any phase of the span, violates too.
 * Sectors that are proven, and strips that are found, both occur. The
 * tracking part of a verdict is built from the working set the gain search
 * grows, and kept between verdicts while that set stands: a check that gave
 * verdicts before its working sets grew gives afterwards, to the bit, the
 * verdicts of one that never gave any before. The last verdict of each
 * frequency is remembered and given again for the same sector while the
 * working set stands, counted as asked; a sector whose verdict the growth
 * of the working set changes, asked just before and just after it grows,
 * gets the new verdict. A verdict is that of every plant of the value set,
 * as a scan over all of them gives it, for sectors of any width and on any
 * turn, on the published problems and on value sets whose plants lie in
 * every direction from the nominal one.
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <limits>
#include <optional>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include "src/app/project_controller.h"
#include "src/core/loopshaping/common/exact_point_check.h"
#include "src/core/loopshaping/common/specification_checker.h"
#include "src/core/math/constants.h"
#include "src/core/math/convex_hull.h"
#include "src/core/math/quadratic_set.h"
#include "src/core/math/range.h"
#include "src/core/specifications/specification.h"
#include "src/core/specifications/specification_record.h"
#include "tests/backend/published_problems.h"

using namespace qftbx;
using namespace qftbx_tests;

namespace {

struct Sector {
    Range phase;
    Range magnitude;
};

Sector randomSector(std::mt19937 & generator)
{
    std::uniform_real_distribution<double> unit(0.0, 1.0);
    const double centrePhase = -360.0 * unit(generator);
    const double centreDb = -40.0 + 100.0 * unit(generator);
    const double spanPhase = std::pow(10.0, -2.0 + 3.6 * unit(generator));
    const double spanDb = std::pow(10.0, -2.0 + 3.0 * unit(generator));
    return {Range(centrePhase - spanPhase / 2.0, centrePhase + spanPhase / 2.0),
            Range(centreDb - spanDb / 2.0, centreDb + spanDb / 2.0)};
}

double largestCosineOver(double from, double to, double cosine, double sine)
{
    if (to - from >= 2.0 * math::kPi) {
        return 1.0;
    }
    const double middle = 0.5 * (from + to);
    if (cosine * std::cos(middle) + sine * std::sin(middle) >= std::cos(0.5 * (to - from))) {
        return 1.0;
    }
    return std::max(cosine * std::cos(from) + sine * std::sin(from), cosine * std::cos(to) + sine * std::sin(to));
}

ExactPointCheck::SectorVerdict scanOfEveryPlant(const FrequencyReference & at, Range phase, Range magnitude)
{
    const double inf = std::numeric_limits<double>::infinity();
    const double from = phase.min * math::kPi / 180.0, to = phase.max * math::kPi / 180.0;
    const double g1 = dbToLinear(magnitude.min), g2 = dbToLinear(magnitude.max);
    const std::vector<std::complex<double>> & q = at.nominalOverValueSet;
    const std::vector<std::size_t> hull = math::convexHullVertices(q);

    ExactPointCheck::SectorVerdict verdict;
    std::vector<double> lower, upper;
    const auto take = [&](double a, double b, double c) {
        double largest = std::max(a * g1 * g1 + b * g1 + c, a * g2 * g2 + b * g2 + c);
        const double vertex = a < 0.0 ? -b / (2.0 * a) : 0.0;
        if (a < 0.0 && vertex > g1 && vertex < g2) {
            largest = std::max(largest, a * vertex * vertex + b * vertex + c);
        }
        verdict.provablyInfeasible = verdict.provablyInfeasible || largest < 0.0;
        math::appendWhereNegative(a, b, c, lower, upper);
    };

    for (const FrequencyReference::Bound & bound : at.bounds) {
        if (bound.type == SpecificationType::TrackingLower) {
            const double dm1 = std::expm1(bound.boundDb * std::log(10.0) / 10.0);
            for (const std::size_t n : hull) {
                for (const std::size_t j : hull) {
                    const std::complex<double> w = (dm1 + 1.0) * q[j] - q[n];
                    const double angle = std::arg(w);
                    take(dm1, 2.0 * std::abs(w) * largestCosineOver(from, to, std::cos(angle), std::sin(angle)),
                         (dm1 + 1.0) * std::norm(q[j]) - std::norm(q[n]));
                }
            }
            continue;
        }
        const double W = dbToLinear(bound.boundDb);
        for (std::size_t n = 0; n < q.size(); ++n) {
            const double modulus = std::abs(q[n]), plantModulus = std::abs((*at.valueSet)[n]);
            std::optional<std::pair<double, double>> disc;
            switch (bound.type) {
            case SpecificationType::Stability:
            case SpecificationType::SensorNoise: disc = std::make_pair(1.0 / W, 0.0); break;
            case SpecificationType::OutputDisturbance: disc = std::make_pair(0.0, modulus / W); break;
            case SpecificationType::InputDisturbance: disc = std::make_pair(0.0, std::abs(at.nominalPlant) / W); break;
            case SpecificationType::ControlEffort:
                if (plantModulus > 0.0) disc = std::make_pair(1.0 / (W * plantModulus), 0.0);
                break;
            default: break;
            }
            if (!disc.has_value()) {
                continue;
            }
            const auto [s, t] = *disc;
            const double angle = std::arg(q[n]);
            const double cmax = modulus * largestCosineOver(from, to, std::cos(angle), std::sin(angle));
            take(1.0 - s * s, 2.0 * (cmax - s * t), std::norm(q[n]) - t * t);
        }
    }

    double covered = 0.0;
    for (bool grew = true; grew;) {
        grew = false;
        for (std::size_t k = 0; k < lower.size(); ++k) {
            if (lower[k] <= covered && upper[k] > covered) {
                covered = upper[k];
                grew = true;
            }
        }
    }
    double reached = inf;
    for (bool grew = true; grew;) {
        grew = false;
        for (std::size_t k = 0; k < lower.size(); ++k) {
            if (upper[k] >= reached && lower[k] < reached) {
                reached = lower[k];
                grew = true;
            }
        }
    }
    if (covered > 0.0) {
        verdict.forbiddenBelowDb = linearToDb(covered);
    }
    if (reached < inf) {
        verdict.forbiddenAboveDb = linearToDb(reached);
    }
    if (verdict.forbiddenBelowDb >= magnitude.max || verdict.forbiddenAboveDb <= magnitude.min) {
        verdict.provablyInfeasible = true;
    }
    return verdict;
}

bool closeEnough(double a, double b)
{
    return a == b || std::abs(a - b) <= 1e-9;
}

}

TEST(ExactSector, ItsVerdictIsThatOfEveryPlantOfTheValueSet)
{
    std::size_t asked = 0, proven = 0, strips = 0;
    for (const std::string name : {"dcm-T33.qft", "toolbox-2.qft", "dcm-k.qft", "toolbox-1.qft", "maglev-upper.qft", "msf.qft",
                                   "maglev-lower.qft", "all round"}) {
        const bool allRound = name == "all round";
        const std::string file = example(allRound ? "toolbox-2.qft" : name.c_str());
        if (!std::filesystem::exists(file)) {
            GTEST_SKIP() << "no published problems under " << QFTBX_EXAMPLES_DIR;
        }
        ProjectController project;
        project.load(file);
        const std::vector<double> & omega = *project.omega()->values();
        const SpecificationSet specifications = toSpecificationSet(*project.specifications());
        CloudSet templates = project.templates();
        if (allRound) {
            for (std::size_t i = 0; i < omega.size(); ++i) {
                const std::complex<double> nominal = project.plant()->evaluate(omega[i]);
                templates[i].clear();
                for (int k = 0; k < 90; ++k) {
                    templates[i].push_back(nominal * std::polar(0.4 + 0.02 * k, 2.0 * math::kPi * k / 90.0 + 0.013));
                }
            }
        }
        ExactPointCheck check(*project.plant(), project.controllerStructure(), omega, templates, specifications);
        ASSERT_TRUE(check.usable());
        const SpecificationReference reference(*project.plant(), omega, templates, specifications);

        std::mt19937 generator(37);
        std::uniform_real_distribution<double> unit(0.0, 1.0);
        for (const FrequencyReference & at : reference.frequencies()) {
            for (int trial = 0; trial < 150; ++trial) {
                const double centre = -1080.0 + 1440.0 * unit(generator);
                const double span = std::pow(10.0, -3.0 + 5.7 * unit(generator));
                const double centreDb = -40.0 + 100.0 * unit(generator);
                const double spanDb = std::pow(10.0, -2.0 + 3.0 * unit(generator));
                const Range phase(centre - span / 2.0, centre + span / 2.0);
                const Range magnitude(centreDb - spanDb / 2.0, centreDb + spanDb / 2.0);

                const ExactPointCheck::SectorVerdict verdict = check.sectorVerdict(at.index, phase, magnitude);
                const ExactPointCheck::SectorVerdict scan = scanOfEveryPlant(at, phase, magnitude);
                EXPECT_EQ(verdict.provablyInfeasible, scan.provablyInfeasible)
                    << name << " w=" << at.omega << " phase [" << phase.min << ", " << phase.max << "]";
                EXPECT_TRUE(closeEnough(verdict.forbiddenBelowDb, scan.forbiddenBelowDb))
                    << name << " w=" << at.omega << " phase [" << phase.min << ", " << phase.max << "] below "
                    << verdict.forbiddenBelowDb << " against " << scan.forbiddenBelowDb;
                EXPECT_TRUE(closeEnough(verdict.forbiddenAboveDb, scan.forbiddenAboveDb))
                    << name << " w=" << at.omega << " phase [" << phase.min << ", " << phase.max << "] above "
                    << verdict.forbiddenAboveDb << " against " << scan.forbiddenAboveDb;
                ++asked;
                proven += scan.provablyInfeasible ? 1 : 0;
                strips += std::isfinite(scan.forbiddenBelowDb) || std::isfinite(scan.forbiddenAboveDb) ? 1 : 0;
            }
        }
    }
    std::printf("EXACT-SECTOR every plant: %zu sectors, %zu proven infeasible, %zu with a strip\n", asked, proven, strips);
    EXPECT_GT(proven, 0u);
    EXPECT_GT(strips, 0u);
}

TEST(ExactSector, WhatItProvesInfeasibleViolatesEverywhere)
{
    for (const char * name : {"dcm-T33.qft", "toolbox-2.qft", "dcm-k.qft", "toolbox-1.qft"}) {
        const std::string file = example(name);
        if (!std::filesystem::exists(file)) {
            GTEST_SKIP() << "no published problems under " << QFTBX_EXAMPLES_DIR;
        }
        ProjectController project;
        project.load(file);
        const std::vector<double> & omega = *project.omega()->values();
        const SpecificationSet specifications = toSpecificationSet(*project.specifications());
        ExactPointCheck check(*project.plant(), project.controllerStructure(), omega, project.templates(), specifications);
        ASSERT_TRUE(check.usable());
        const SpecificationReference reference(*project.plant(), omega, project.templates(), specifications);

        std::mt19937 generator(29);
        std::uniform_real_distribution<double> unit(0.0, 1.0);
        std::size_t proven = 0, strips = 0, sampled = 0;

        for (const FrequencyReference & at : reference.frequencies()) {
            for (int trial = 0; trial < 60; ++trial) {
                const auto [phase, magnitude] = randomSector(generator);

                const ExactPointCheck::SectorVerdict verdict = check.sectorVerdict(at.index, phase, magnitude);

                const auto loopAt = [&](double phaseDeg, double db) {
                    return std::polar(std::pow(10.0, db / 20.0), phaseDeg * math::kPi / 180.0);
                };

                if (verdict.provablyInfeasible) {
                    ++proven;
                    for (int a = 0; a <= 12; ++a) {
                        for (int b = 0; b <= 12; ++b) {
                            const double phaseDeg = phase.min + (phase.max - phase.min) * a / 12.0;
                            const double db = magnitude.min + (magnitude.max - magnitude.min) * b / 12.0;
                            EXPECT_GT(at.worstExcessAt(loopAt(phaseDeg, db)), 0.0)
                                << name << " w=" << at.omega << " trial " << trial << " at " << phaseDeg << " deg, " << db << " dB";
                            ++sampled;
                        }
                    }
                }
                if (std::isfinite(verdict.forbiddenBelowDb)) {
                    ++strips;
                    for (int a = 0; a < 40; ++a) {
                        const double phaseDeg = phase.min + (phase.max - phase.min) * unit(generator);
                        const double db = verdict.forbiddenBelowDb - 1e-6 - 30.0 * unit(generator);
                        EXPECT_GT(at.worstExcessAt(loopAt(phaseDeg, db)), 0.0)
                            << name << " w=" << at.omega << " trial " << trial << " below " << verdict.forbiddenBelowDb << " dB at " << db;
                        ++sampled;
                    }
                }
                if (std::isfinite(verdict.forbiddenAboveDb)) {
                    ++strips;
                    for (int a = 0; a < 40; ++a) {
                        const double phaseDeg = phase.min + (phase.max - phase.min) * unit(generator);
                        const double db = verdict.forbiddenAboveDb + 1e-6 + 30.0 * unit(generator);
                        EXPECT_GT(at.worstExcessAt(loopAt(phaseDeg, db)), 0.0)
                            << name << " w=" << at.omega << " trial " << trial << " above " << verdict.forbiddenAboveDb << " dB at " << db;
                        ++sampled;
                    }
                }
            }
        }
        std::printf("EXACT-SECTOR %-13s %zu sectors proven infeasible, %zu certified strips, %zu loop values sampled\n",
                    name, proven, strips, sampled);
        EXPECT_GT(proven, 0u) << name;
        EXPECT_GT(strips, 0u) << name;
    }
}

TEST(ExactSector, AVerdictAfterTheWorkingSetGrowsIsThatOfAFreshCache)
{
    std::size_t grown = 0;
    for (const char * name : {"dcm-T33.qft", "toolbox-2.qft", "dcm-k.qft", "toolbox-1.qft"}) {
        const std::string file = example(name);
        if (!std::filesystem::exists(file)) {
            GTEST_SKIP() << "no published problems under " << QFTBX_EXAMPLES_DIR;
        }
        ProjectController project;
        project.load(file);
        LtiSystem * structure = project.controllerStructure();
        const std::vector<double> & omega = *project.omega()->values();
        const SpecificationSet specifications = toSpecificationSet(*project.specifications());
        ExactPointCheck warmed(*project.plant(), structure, omega, project.templates(), specifications);
        ExactPointCheck fresh(*project.plant(), structure, omega, project.templates(), specifications);
        ASSERT_TRUE(warmed.usable());

        std::mt19937 generator(31);
        std::vector<std::pair<std::size_t, Sector>> sectors;
        for (std::size_t f = 0; f < omega.size(); ++f) {
            for (int trial = 0; trial < 20; ++trial) {
                sectors.emplace_back(f, randomSector(generator));
            }
        }

        for (const auto & [frequency, sector] : sectors) {
            warmed.sectorVerdict(frequency, sector.phase, sector.magnitude);
        }

        for (int vertex = 0; vertex < 30; ++vertex) {
            std::vector<double> zeros, poles;
            for (const Parameter & z : structure->numerator()) zeros.push_back(drawIn(z, generator));
            for (const Parameter & q : structure->denominator()) poles.push_back(drawIn(q, generator));
            warmed.lowestAdmissibleGain(zeros, poles, structure->gain().range());
            fresh.lowestAdmissibleGain(zeros, poles, structure->gain().range());
        }
        grown += warmed.statistics().exchangeRounds;

        for (const auto & [frequency, sector] : sectors) {
            const ExactPointCheck::SectorVerdict a = warmed.sectorVerdict(frequency, sector.phase, sector.magnitude);
            const ExactPointCheck::SectorVerdict b = fresh.sectorVerdict(frequency, sector.phase, sector.magnitude);
            EXPECT_EQ(a.provablyInfeasible, b.provablyInfeasible) << name << " frequency " << frequency;
            EXPECT_EQ(a.forbiddenBelowDb, b.forbiddenBelowDb) << name << " frequency " << frequency;
            EXPECT_EQ(a.forbiddenAboveDb, b.forbiddenAboveDb) << name << " frequency " << frequency;
        }
    }
    EXPECT_GT(grown, 0u) << "the gain searches grew some working set";
}

TEST(ExactSector, ARememberedVerdictLastsOnlyWhileTheWorkingSetStands)
{
    const auto same = [](const ExactPointCheck::SectorVerdict & a, const ExactPointCheck::SectorVerdict & b) {
        return a.provablyInfeasible == b.provablyInfeasible && a.forbiddenBelowDb == b.forbiddenBelowDb
               && a.forbiddenAboveDb == b.forbiddenAboveDb;
    };

    std::size_t changed = 0;
    for (const char * name : {"dcm-T33.qft", "toolbox-2.qft", "dcm-k.qft", "toolbox-1.qft"}) {
        const std::string file = example(name);
        if (!std::filesystem::exists(file)) {
            GTEST_SKIP() << "no published problems under " << QFTBX_EXAMPLES_DIR;
        }
        ProjectController project;
        project.load(file);
        LtiSystem * structure = project.controllerStructure();
        const std::vector<double> & omega = *project.omega()->values();
        const SpecificationSet specifications = toSpecificationSet(*project.specifications());
        ExactPointCheck standing(*project.plant(), structure, omega, project.templates(), specifications);
        ExactPointCheck grown(*project.plant(), structure, omega, project.templates(), specifications);
        ExactPointCheck asked(*project.plant(), structure, omega, project.templates(), specifications);
        ASSERT_TRUE(standing.usable());

        std::mt19937 generator(31);
        std::vector<std::pair<std::size_t, Sector>> sectors;
        for (std::size_t f = 0; f < omega.size(); ++f) {
            for (int trial = 0; trial < 20; ++trial) {
                sectors.emplace_back(f, randomSector(generator));
            }
        }
        std::vector<std::pair<std::vector<double>, std::vector<double>>> vertices;
        for (int vertex = 0; vertex < 30; ++vertex) {
            std::vector<double> zeros, poles;
            for (const Parameter & z : structure->numerator()) zeros.push_back(drawIn(z, generator));
            for (const Parameter & q : structure->denominator()) poles.push_back(drawIn(q, generator));
            vertices.emplace_back(zeros, poles);
        }
        const auto grow = [&](ExactPointCheck & check) {
            for (const auto & [zeros, poles] : vertices) {
                check.lowestAdmissibleGain(zeros, poles, structure->gain().range());
            }
        };

        const std::size_t before = standing.statistics().sectorVerdicts;
        const auto & [firstFrequency, firstSector] = sectors.front();
        EXPECT_TRUE(same(standing.sectorVerdict(firstFrequency, firstSector.phase, firstSector.magnitude),
                         standing.sectorVerdict(firstFrequency, firstSector.phase, firstSector.magnitude)));
        EXPECT_EQ(standing.statistics().sectorVerdicts, before + 2) << "a remembered verdict still counts as asked";

        grow(grown);
        std::vector<std::pair<std::size_t, Sector>> picked;
        std::vector<ExactPointCheck::SectorVerdict> after;
        for (const auto & [frequency, sector] : sectors) {
            if (!picked.empty() && picked.back().first == frequency) {
                continue;
            }
            const ExactPointCheck::SectorVerdict old = standing.sectorVerdict(frequency, sector.phase, sector.magnitude);
            const ExactPointCheck::SectorVerdict fresh = grown.sectorVerdict(frequency, sector.phase, sector.magnitude);
            if (!same(old, fresh)) {
                picked.emplace_back(frequency, sector);
                after.push_back(fresh);
            }
        }

        for (const auto & [frequency, sector] : picked) {
            asked.sectorVerdict(frequency, sector.phase, sector.magnitude);
        }
        grow(asked);
        for (std::size_t k = 0; k < picked.size(); ++k) {
            const auto & [frequency, sector] = picked[k];
            EXPECT_TRUE(same(asked.sectorVerdict(frequency, sector.phase, sector.magnitude), after[k]))
                << name << " frequency " << frequency << ": the verdict from before the working set grew was given again";
        }
        changed += picked.size();
    }
    EXPECT_GT(changed, 0u) << "some verdict changes as the working set grows";
}
