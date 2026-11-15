#include <gtest/gtest.h>

#include <chrono>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <random>
#include <vector>

#include "src/app/project_controller.h"
#include "src/core/loopshaping/common/exact_point_check.h"
#include "src/core/loopshaping/common/family_stability_checker.h"
#include "src/core/math/range.h"
#include "src/core/specifications/specification_record.h"

using namespace qftbx;

TEST(T3Cost, PerCallAfterConvergence)
{
    for (const char * name : {"dcm-T33.qft", "toolbox-1.qft"}) {
        ProjectController project;
        project.load((std::filesystem::path(QFTBX_EXAMPLES_DIR) / name).string());
        LtiSystem * structure = project.controllerStructure();
        const std::vector<double> & omega = *project.omega()->values();
        const SpecificationSet specifications = toSpecificationSet(*project.specifications());
        ExactPointCheck check(*project.plant(), structure, omega, project.templates(), specifications);
        FamilyStabilityChecker family(project.plant(), structure, project.sweepGrids());
        const Range gains = structure->gain().range();

        std::mt19937 generator(21);
        std::uniform_real_distribution<double> unit(0.0, 1.0);
        std::vector<std::vector<double>> zeros, poles;
        for (int v = 0; v < 4000; ++v) {
            std::vector<double> z, p;
            for (const Parameter & q : structure->numerator()) {
                const Range r = q.range();
                z.push_back(q.isUncertain() ? std::exp(std::log(r.min) + unit(generator) * (std::log(r.max) - std::log(r.min))) : q.nominal());
            }
            for (const Parameter & q : structure->denominator()) {
                const Range r = q.range();
                p.push_back(q.isUncertain() ? std::exp(std::log(r.min) + unit(generator) * (std::log(r.max) - std::log(r.min))) : q.nominal());
            }
            zeros.push_back(z);
            poles.push_back(p);
        }

        for (int v = 0; v < 1000; ++v) check.lowestAdmissibleGain(zeros[v], poles[v], gains);
        const ExactPointCheck::Statistics before = check.statistics();

        auto start = std::chrono::steady_clock::now();
        std::size_t found = 0;
        std::vector<PointController> points;
        for (int v = 1000; v < 4000; ++v) {
            const ExactPointCheck::GainSearch s = check.lowestAdmissibleGain(zeros[v], poles[v], gains);
            if (s.gain) { ++found; points.push_back({*s.gain, zeros[v], poles[v]}); }
        }
        const double t3 = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - start).count() / 3000.0;
        const ExactPointCheck::Statistics after = check.statistics();

        start = std::chrono::steady_clock::now();
        std::size_t admitted = 0;
        for (const PointController & p : points) admitted += check.admits(p) ? 1 : 0;
        const double admits = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - start).count() / points.size();

        start = std::chrono::steady_clock::now();
        std::size_t stable = 0;
        for (const PointController & p : points) stable += family.isStable(p) ? 1 : 0;
        const double routh = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - start).count() / points.size();

        std::printf("T3COST %-13s T3 %.1f us/call (%zu of 3000 admit; %zu kernel passes, %zu rounds, largest set %zu) | admits %.1f us (%zu of %zu) | Routh %.1f us (%zu stable)\n",
                    name, t3, found, after.kernelPasses - before.kernelPasses, after.exchangeRounds - before.exchangeRounds,
                    after.largestWorkingSet, admits, admitted, points.size(), routh, stable);
    }
}
