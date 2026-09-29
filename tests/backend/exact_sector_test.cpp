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
 * verdicts of one that never gave any before.
 */

#include <gtest/gtest.h>

#include <cmath>
#include <complex>
#include <cstdio>
#include <filesystem>
#include <random>
#include <string>
#include <vector>

#include "src/app/project_controller.h"
#include "src/core/loopshaping/common/exact_point_check.h"
#include "src/core/loopshaping/common/specification_checker.h"
#include "src/core/math/range.h"
#include "src/core/specifications/specification_record.h"

using namespace qftbx;

TEST(ExactSector, WhatItProvesInfeasibleViolatesEverywhere)
{
    constexpr double kPi = 3.14159265358979323846;
    for (const char * name : {"dcm-T33.qft", "toolbox-2.qft", "dcm-k.qft", "toolbox-1.qft"}) {
        const std::string file = (std::filesystem::path(QFTBX_EXAMPLES_DIR) / name).string();
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
                const double centrePhase = -360.0 * unit(generator);
                const double centreDb = -40.0 + 100.0 * unit(generator);
                const double spanPhase = std::pow(10.0, -2.0 + 3.6 * unit(generator));
                const double spanDb = std::pow(10.0, -2.0 + 3.0 * unit(generator));
                const Range phase(centrePhase - spanPhase / 2.0, centrePhase + spanPhase / 2.0);
                const Range magnitude(centreDb - spanDb / 2.0, centreDb + spanDb / 2.0);

                const ExactPointCheck::SectorVerdict verdict = check.sectorVerdict(at.index, phase, magnitude);

                const auto loopAt = [&](double phaseDeg, double db) {
                    return std::polar(std::pow(10.0, db / 20.0), phaseDeg * kPi / 180.0);
                };

                if (verdict.provablyInfeasible) {
                    ++proven;
                    for (int a = 0; a <= 12; ++a) {
                        for (int b = 0; b <= 12; ++b) {
                            const double phaseDeg = phase.min + (phase.max - phase.min) * a / 12.0;
                            const double db = magnitude.min + (magnitude.max - magnitude.min) * b / 12.0;
                            EXPECT_GT(reference.worstExcessAt(at, loopAt(phaseDeg, db)), 0.0)
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
                        EXPECT_GT(reference.worstExcessAt(at, loopAt(phaseDeg, db)), 0.0)
                            << name << " w=" << at.omega << " trial " << trial << " below " << verdict.forbiddenBelowDb << " dB at " << db;
                        ++sampled;
                    }
                }
                if (std::isfinite(verdict.forbiddenAboveDb)) {
                    ++strips;
                    for (int a = 0; a < 40; ++a) {
                        const double phaseDeg = phase.min + (phase.max - phase.min) * unit(generator);
                        const double db = verdict.forbiddenAboveDb + 1e-6 + 30.0 * unit(generator);
                        EXPECT_GT(reference.worstExcessAt(at, loopAt(phaseDeg, db)), 0.0)
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
        const std::string file = (std::filesystem::path(QFTBX_EXAMPLES_DIR) / name).string();
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

        struct Sector {
            std::size_t frequency;
            Range phase;
            Range magnitude;
        };
        std::mt19937 generator(31);
        std::uniform_real_distribution<double> unit(0.0, 1.0);
        std::vector<Sector> sectors;
        for (std::size_t f = 0; f < omega.size(); ++f) {
            for (int trial = 0; trial < 20; ++trial) {
                const double centrePhase = -360.0 * unit(generator);
                const double centreDb = -40.0 + 100.0 * unit(generator);
                const double spanPhase = std::pow(10.0, -2.0 + 3.6 * unit(generator));
                const double spanDb = std::pow(10.0, -2.0 + 3.0 * unit(generator));
                sectors.push_back({f, Range(centrePhase - spanPhase / 2.0, centrePhase + spanPhase / 2.0),
                                   Range(centreDb - spanDb / 2.0, centreDb + spanDb / 2.0)});
            }
        }

        for (const Sector & s : sectors) {
            warmed.sectorVerdict(s.frequency, s.phase, s.magnitude);
        }

        const auto logDraw = [&](const Parameter & parameter) {
            const Range r = parameter.range();
            return std::exp(std::log(r.min) + unit(generator) * (std::log(r.max) - std::log(r.min)));
        };
        for (int vertex = 0; vertex < 30; ++vertex) {
            std::vector<double> zeros, poles;
            for (const Parameter & z : structure->numerator()) zeros.push_back(logDraw(z));
            for (const Parameter & q : structure->denominator()) poles.push_back(logDraw(q));
            warmed.lowestAdmissibleGain(zeros, poles, structure->gain().range());
            fresh.lowestAdmissibleGain(zeros, poles, structure->gain().range());
        }
        grown += warmed.statistics().exchangeRounds;

        for (const Sector & s : sectors) {
            const ExactPointCheck::SectorVerdict a = warmed.sectorVerdict(s.frequency, s.phase, s.magnitude);
            const ExactPointCheck::SectorVerdict b = fresh.sectorVerdict(s.frequency, s.phase, s.magnitude);
            EXPECT_EQ(a.provablyInfeasible, b.provablyInfeasible) << name << " frequency " << s.frequency;
            EXPECT_EQ(a.forbiddenBelowDb, b.forbiddenBelowDb) << name << " frequency " << s.frequency;
            EXPECT_EQ(a.forbiddenAboveDb, b.forbiddenAboveDb) << name << " frequency " << s.frequency;
        }
    }
    EXPECT_GT(grown, 0u) << "the gain searches grew some working set";
}
