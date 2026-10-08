/**
 * @file
 * @brief The exact best gain at fixed zeros and poles is the smallest gain
 * the verifier admits there.
 *
 * On the published problems: the admissible gain set computed over the
 * whole cloud agrees with the verifier's criterion on a fine scan of gains
 * at random zeros and poles, the working-set search finds the same minimum
 * as the whole cloud, and the minimum is a minimum, a hair below it refused.
 * The search keeps state of its own from one vertex to the next, which plant
 * it asks first, and the points asked in between must not move it: two
 * checks asked the same vertices, one of them asked other points between,
 * give the same gains to the bit after the same exchange rounds and the same
 * confirmations. The vertices the searches returned in the study are
 * pinned: the DC motor at z = 1000, p = 472.86
 * where the two bracketing columns demanded 89 and the specifications admit
 * 41.47, the toolbox example at its nearest-node vertex, and the gear-train
 * motor at its conservative vertex.
 */

#include <gtest/gtest.h>

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include "src/app/project_controller.h"
#include "src/core/loopshaping/common/exact_point_check.h"
#include "src/core/math/range.h"
#include "src/core/math/range_union.h"
#include "src/core/specifications/specification_record.h"
#include "tests/backend/published_problems.h"

using namespace qftbx;
using namespace qftbx_tests;

namespace {

struct Problem
{
    explicit Problem(const char * name)
    {
        project.load(example(name));
        structure = project.controllerStructure();
        omega = project.omega()->values();
        specifications = toSpecificationSet(*project.specifications());
        check = std::make_unique<ExactPointCheck>(*project.plant(), structure, *omega, project.templates(), specifications);
    }

    ProjectController project;
    LtiSystem * structure = nullptr;
    std::vector<double> * omega = nullptr;
    SpecificationSet specifications;
    std::unique_ptr<ExactPointCheck> check;
};

}

TEST(ExactGain, TheAdmissibleSetIsTheVerifiersOnAScan)
{
    for (const char * name : {"dcm-T33.qft", "toolbox-2.qft", "dcm-k.qft"}) {
        if (!std::filesystem::exists(example(name))) {
            GTEST_SKIP() << "no published problems under " << QFTBX_EXAMPLES_DIR;
        }
        Problem problem(name);
        ASSERT_TRUE(problem.check->usable());
        const Range gains = problem.structure->gain().range();

        std::mt19937 generator(3);
        std::size_t compared = 0, admitted = 0;
        for (int vertex = 0; vertex < 12; ++vertex) {
            PointController point;
            for (const Parameter & z : problem.structure->numerator()) point.zeros.push_back(drawIn(z, generator));
            for (const Parameter & p : problem.structure->denominator()) point.poles.push_back(drawIn(p, generator));

            const RangeUnion set = problem.check->admissibleGainsDb(point.zeros, point.poles, gains);

            const double lowDb = 20.0 * std::log10(gains.min), highDb = 20.0 * std::log10(gains.max);
            for (int k = 0; k <= 400; ++k) {
                const double gainDb = lowDb + (highDb - lowDb) * k / 400.0;
                bool nearAnEnd = false;
                for (const Range & part : set.components()) {
                    nearAnEnd = nearAnEnd || std::abs(gainDb - part.min) < 1e-6 || std::abs(gainDb - part.max) < 1e-6;
                }
                if (nearAnEnd) {
                    continue;
                }
                point.gain = std::pow(10.0, gainDb / 20.0);
                const bool verifier = problem.check->admits(point);
                EXPECT_EQ(set.contains(gainDb), verifier) << name << " vertex " << vertex << " gain " << point.gain << " dB " << gainDb;
                ++compared;
                admitted += verifier ? 1 : 0;
            }
        }
        std::printf("EXACT-GAIN %-14s scan: %zu gains compared, %zu admitted\n", name, compared, admitted);
        EXPECT_GT(admitted, 0u) << name;
        EXPECT_LT(admitted, compared) << name;
    }
}

TEST(ExactGain, TheWorkingSetFindsTheMinimumOfTheWholeCloud)
{
    for (const char * name : {"dcm-T33.qft", "toolbox-2.qft", "dcm-k.qft"}) {
        if (!std::filesystem::exists(example(name))) {
            GTEST_SKIP() << "no published problems under " << QFTBX_EXAMPLES_DIR;
        }
        Problem problem(name);
        const Range gains = problem.structure->gain().range();

        std::mt19937 generator(9);
        std::size_t found = 0, rounds = 0, confirmations = 0;
        for (int vertex = 0; vertex < 30; ++vertex) {
            std::vector<double> zeros, poles;
            for (const Parameter & z : problem.structure->numerator()) zeros.push_back(drawIn(z, generator));
            for (const Parameter & p : problem.structure->denominator()) poles.push_back(drawIn(p, generator));

            const RangeUnion whole = problem.check->admissibleGainsDb(zeros, poles, gains);
            const ExactPointCheck::GainSearch search = problem.check->lowestAdmissibleGain(zeros, poles, gains);
            rounds += search.rounds;
            confirmations += search.confirmations;

            if (whole.isEmpty()) {
                EXPECT_FALSE(search.gain.has_value()) << name << " vertex " << vertex;
                continue;
            }
            ASSERT_TRUE(search.gain.has_value()) << name << " vertex " << vertex;
            ++found;
            const double minimum = std::pow(10.0, whole.minimum() / 20.0);
            EXPECT_NEAR(*search.gain, minimum, 2e-7 * minimum) << name << " vertex " << vertex;

            PointController point{*search.gain, zeros, poles};
            EXPECT_TRUE(problem.check->admits(point));
            point.gain = *search.gain * (1.0 - 1e-6);
            EXPECT_FALSE(problem.check->admits(point)) << name << " vertex " << vertex << ": a hair below is refused";
        }
        std::printf("EXACT-GAIN %-14s working set: %zu of 30 vertices admit a gain, %zu exchange rounds, %zu confirmations, largest set %zu\n",
                    name, found, rounds, confirmations, problem.check->statistics().largestWorkingSet);
        EXPECT_GT(found, 0u) << name;
    }
}

TEST(ExactGain, WhatIsAskedBetweenTwoSearchesDoesNotMoveThem)
{
    for (const char * name : {"dcm-T33.qft", "toolbox-2.qft", "dcm-k.qft"}) {
        if (!std::filesystem::exists(example(name))) {
            GTEST_SKIP() << "no published problems under " << QFTBX_EXAMPLES_DIR;
        }
        Problem alone(name);
        Problem interrupted(name);
        const Range gains = alone.structure->gain().range();

        std::mt19937 vertices(17);
        std::mt19937 between(41);
        std::size_t found = 0;
        for (int vertex = 0; vertex < 20; ++vertex) {
            std::vector<double> zeros, poles;
            for (const Parameter & z : alone.structure->numerator()) zeros.push_back(drawIn(z, vertices));
            for (const Parameter & p : alone.structure->denominator()) poles.push_back(drawIn(p, vertices));

            for (int asked = 0; asked < 3; ++asked) {
                interrupted.check->admits(randomPoint(*alone.structure, between));
            }

            const ExactPointCheck::GainSearch first = alone.check->lowestAdmissibleGain(zeros, poles, gains);
            const ExactPointCheck::GainSearch second = interrupted.check->lowestAdmissibleGain(zeros, poles, gains);
            ASSERT_EQ(first.gain.has_value(), second.gain.has_value()) << name << " vertex " << vertex;
            EXPECT_EQ(first.rounds, second.rounds) << name << " vertex " << vertex;
            EXPECT_EQ(first.confirmations, second.confirmations) << name << " vertex " << vertex;
            if (first.gain.has_value()) {
                EXPECT_EQ(*first.gain, *second.gain) << name << " vertex " << vertex;
                ++found;
            }
        }
        EXPECT_GT(found, 0u) << name;
        EXPECT_GT(interrupted.check->statistics().verdicts, alone.check->statistics().verdicts) << name;
    }
}

TEST(ExactGain, TheVerticesOfTheStudyArePinned)
{
    struct Pin {
        const char * file;
        std::vector<double> zeros;
        std::vector<double> poles;
        double gain;
    };
    const Pin pins[] = {
        {"dcm-T33.qft", {1000.0}, {472.86}, 41.4696},
        {"toolbox-2.qft", {1.8076}, {139.95}, 566.88},
        {"dcm-k.qft", {3.24587}, {67.9953, 69.6121}, 47807.0},
    };
    for (const Pin & pin : pins) {
        if (!std::filesystem::exists(example(pin.file))) {
            GTEST_SKIP() << "no published problems under " << QFTBX_EXAMPLES_DIR;
        }
        Problem problem(pin.file);
        const ExactPointCheck::GainSearch search =
                problem.check->lowestAdmissibleGain(pin.zeros, pin.poles, problem.structure->gain().range());
        ASSERT_TRUE(search.gain.has_value()) << pin.file;
        std::printf("EXACT-GAIN %-14s vertex: gain %.6f (study %.4f), %zu rounds, %zu confirmations\n",
                    pin.file, *search.gain, pin.gain, search.rounds, search.confirmations);
        EXPECT_NEAR(*search.gain, pin.gain, 2e-4 * pin.gain) << pin.file;
    }
}
