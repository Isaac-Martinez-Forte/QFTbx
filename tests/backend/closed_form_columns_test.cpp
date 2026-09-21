/**
 * @file
 * @brief Tests of the closed-form allowed gains against the sampled sheet.
 *
 * At a fixed phase each magnitude specification is a quadratic in the loop
 * gain for every plant of the template (Chait and Yaniv 1993), and the
 * allowed set of the template is the exact intersection of the plants'
 * sets, multivalued where it is. Random plants, phases and bounds check
 * that the closed-form set agrees with the inequality evaluated directly at
 * every gain except within a hair of a root. On QFT toolbox example 2 the
 * closed-form columns must match the sheet cut at the bound to within half
 * a magnitude cell, differing only on features narrower than a cell; NT
 * must land within a tenth of a per cent of the sheet's gain, and a cloud,
 * which has no polygon, must keep the sheet's columns exactly.
 */

#include <gtest/gtest.h>

#include <cmath>
#include <complex>
#include <cstdio>
#include <random>
#include <string>
#include <vector>

#include "src/app/project_controller.h"
#include "src/core/project/settings.h"
#include "src/core/boundaries/boundary_columns.h"
#include "src/core/boundaries/closed_form_columns.h"
#include "src/core/boundaries/closed_loop_worst_case.h"
#include "src/core/boundaries/singular_locus.h"
#include "src/core/math/range.h"
#include "src/core/math/sequences.h"

using namespace qftbx;

namespace {

double magnitudeOf(SpecificationType type, std::complex<double> p0, std::complex<double> p,
                   std::complex<double> q, double g, double phaseDegrees)
{
    const std::complex<double> L = std::polar(g, phaseDegrees * M_PI / 180.0);
    const std::complex<double> d = q + L;
    switch (type) {
    case SpecificationType::Stability:
    case SpecificationType::SensorNoise: return std::abs(L / d);
    case SpecificationType::OutputDisturbance: return std::abs(q / d);
    case SpecificationType::InputDisturbance: return std::abs(p0 / d);
    case SpecificationType::ControlEffort: return std::abs((L / p) / d);
    default: return 0.0;
    }
}

bool contains(const RangeUnion & set, double g)
{
    for (const Range & r : set.components()) {
        if (g >= r.min && g <= r.max) return true;
    }
    return false;
}

}

TEST(ClosedFormColumns, TheQuadraticAgreesWithTheInequalityItSolves)
{
    std::mt19937 rng(7);
    std::uniform_real_distribution<double> mag(-2.0, 2.0), ang(-M_PI, M_PI), ph(-360.0, 0.0), db(-20.0, 20.0);
    const SpecificationType types[] = {SpecificationType::Stability, SpecificationType::OutputDisturbance,
                                       SpecificationType::InputDisturbance, SpecificationType::ControlEffort};
    std::size_t checked = 0, disagreements = 0;
    for (int trial = 0; trial < 400; ++trial) {
        const std::complex<double> p0 = std::polar(std::pow(10.0, mag(rng)), ang(rng));
        const std::complex<double> p = std::polar(std::pow(10.0, mag(rng)), ang(rng));
        const std::complex<double> q = p0 / p;
        const double phase = ph(rng);
        const double boundDb = db(rng);
        const double W = std::pow(10.0, boundDb / 20.0);
        for (SpecificationType type : types) {
            const RangeUnion allowed = ClosedFormColumns::allowedGains(type, W, p0, p, q, phase);
            for (double gDb = -80.0; gDb <= 80.0; gDb += 0.25) {
                const double g = std::pow(10.0, gDb / 20.0);
                const bool direct = magnitudeOf(type, p0, p, q, g, phase) <= W;
                const bool closed = contains(allowed, g);
                ++checked;
                if (direct != closed) {
                    const bool flips = (magnitudeOf(type, p0, p, q, g * 1.001, phase) <= W) != direct ||
                                       (magnitudeOf(type, p0, p, q, g / 1.001, phase) <= W) != direct;
                    if (!flips) ++disagreements;
                }
            }
        }
    }
    EXPECT_EQ(disagreements, 0u) << "of " << checked << " gains checked";
}

TEST(ClosedFormColumns, OnExampleTwoTheColumnsMatchTheSheetToItsInterpolationError)
{
    ProjectController controller;
    {
        qftbx::Settings published;
        published.algorithms.conservativeBoundaryColumns = false;
        controller.applySettings(published);
    }
    controller.load(std::string(QFTBX_TEST_DATA_DIR "/qft_toolbox_ex2.qft"));
    const std::vector<double> & omega = *controller.omega()->values();
    const std::vector<double> phases = qftbx::math::linspace(-360.0, 0.0, 361);

    double worst = 0.0;
    std::size_t crossings = 0, mismatchedCounts = 0;
    for (std::size_t f = 0; f < omega.size(); ++f) {
        const std::complex<double> p0 = controller.plant()->evaluate(omega[f]);
        const ComplexCloud & cloud = controller.contour()[f];
        const std::vector<std::complex<double>> q = nominalOverValueSet(p0, cloud);
        const double boundDb = 20.0 * std::log10(1.2);

        const SingularLocus locus(q, true);
        const BoundaryColumns closed = ClosedFormColumns::columns(SpecificationType::Stability, boundDb, p0, cloud, q,
                                                                  phases, Range(-360.0, 0.0), &locus);
        const std::vector<double> magnitudes = qftbx::math::linspace(-60.0, 160.0, 441);
        std::vector<std::vector<double>> sheet(magnitudes.size(), std::vector<double>(phases.size()));
        for (std::size_t k = 0; k < magnitudes.size(); ++k) {
            for (std::size_t j = 0; j < phases.size(); ++j) {
                const std::complex<double> L = std::polar(std::pow(10.0, magnitudes[k] / 20.0), phases[j] * M_PI / 180.0);
                sheet[k][j] = 20.0 * std::log10(locus.guard(worstCaseAt(p0, L, cloud, q), L, p0).stabilityNoise);
            }
        }
        const BoundaryColumns fromSheet = BoundaryColumns::fromSheet(
            [&sheet](std::int32_t phase, std::int32_t magnitude) { return sheet[static_cast<std::size_t>(magnitude)][static_cast<std::size_t>(phase)]; },
            boundDb, 361, Range(-360.0, 0.0), 441, Range(-60.0, 160.0));

        for (std::int32_t c = 0; c < 361; ++c) {
            const BoundaryColumns::Intervals a = closed.intervals(c);
            const BoundaryColumns::Intervals b = fromSheet.intervals(c);
            if (a.count != b.count) {
                ++mismatchedCounts;
                std::printf("  w=%g column %d (phase %g): closed form", omega[f], c, phases[static_cast<std::size_t>(c)]);
                for (std::int32_t i = 0; i < a.count; ++i) std::printf(" [%.2f, %.2f]", a.lo[i], a.hi[i]);
                std::printf("  sheet");
                for (std::int32_t i = 0; i < b.count; ++i) std::printf(" [%.2f, %.2f]", b.lo[i], b.hi[i]);
                std::printf("\n");
                const BoundaryColumns::Intervals & more = a.count > b.count ? a : b;
                const BoundaryColumns::Intervals & fewer = a.count > b.count ? b : a;
                double narrowest = 1e300;
                for (std::int32_t i = 0; i < more.count; ++i) {
                    bool matched = false;
                    for (std::int32_t k = 0; k < fewer.count; ++k) {
                        if (std::abs(more.lo[i] - fewer.lo[k]) < 0.5 && std::abs(more.hi[i] - fewer.hi[k]) < 0.5) matched = true;
                    }
                    if (!matched) narrowest = std::min(narrowest, more.hi[i] - more.lo[i]);
                }
                double narrowestGap = 1e300;
                for (std::int32_t i = 0; i + 1 < more.count; ++i) narrowestGap = std::min(narrowestGap, more.lo[i + 1] - more.hi[i]);
                EXPECT_LT(std::min(narrowest, narrowestGap), 0.5) << "a feature wider than a cell that only one side sees";
                continue;
            }
            for (std::int32_t i = 0; i < a.count; ++i) {
                for (const double * pair : {&a.lo[i], &a.hi[i]}) {
                    const double x = *pair;
                    const double y = pair == &a.lo[i] ? b.lo[i] : b.hi[i];
                    if (std::isfinite(x) && std::isfinite(y)) {
                        worst = std::max(worst, std::abs(x - y));
                        ++crossings;
                    } else {
                        EXPECT_EQ(std::isfinite(x), std::isfinite(y)) << "w " << omega[f] << " column " << c;
                    }
                }
            }
        }
    }
    std::printf("CLOSEDFORM ex2 stability: %zu crossings compared, largest deviation from the sheet %.4f dB, columns with a different interval count %zu\n",
                crossings, worst, mismatchedCounts);
    std::fflush(stdout);
    EXPECT_GT(crossings, 1000u);
    EXPECT_LT(worst, 0.25) << "half a magnitude cell";
    EXPECT_LT(mismatchedCounts, 20u) << "a handful of sub-cell features";
}

TEST(ClosedFormColumns, TheSearchOnExampleTwoLandsWithinTheInterpolationError)
{
    const auto solve = [](bool closedForm) {
        ProjectController controller;
        {
            qftbx::Settings published;
            published.algorithms.conservativeBoundaryColumns = false;
            controller.applySettings(published);
        }
    {
        qftbx::Settings published;
        published.algorithms.conservativeBoundaryColumns = false;
        controller.applySettings(published);
    }
        controller.load(std::string(QFTBX_TEST_DATA_DIR "/qft_toolbox_ex2.qft"));
        controller.setClosedFormColumns(closedForm);
        EXPECT_TRUE(controller.computeBoundaries(Range(-360.0, 0.0), 361, Range(-60.0, 160.0), 441, 1.0e6, true, false));
        EXPECT_TRUE(controller.computeLoopShaping(0.5, qftbx::nt, Range(1e-9, 10.0), 100));
        return controller.loopShapingResult()->controller()->gain().nominal();
    };
    const double sheet = solve(false);
    const double closed = solve(true);
    std::printf("CLOSEDFORM ex2 NT: sheet k=%.7f  closed form k=%.7f\n", sheet, closed);
    std::fflush(stdout);
    EXPECT_NEAR(closed, sheet, 1e-3 * sheet);
}

TEST(ClosedFormColumns, ACloudKeepsTheSheetsColumns)
{
    const auto solve = [](bool closedForm) {
        ProjectController controller;
        {
            qftbx::Settings published;
            published.algorithms.conservativeBoundaryColumns = false;
            controller.applySettings(published);
        }
    {
        qftbx::Settings published;
        published.algorithms.conservativeBoundaryColumns = false;
        controller.applySettings(published);
    }
        controller.load(std::string(QFTBX_TEST_DATA_DIR "/qft_toolbox_ex2.qft"));
        controller.setClosedFormColumns(closedForm);
        EXPECT_TRUE(controller.computeBoundaries(Range(-360.0, 0.0), 361, Range(-60.0, 160.0), 441, 1.0e6, false, false));
        EXPECT_TRUE(controller.computeLoopShaping(0.5, qftbx::nt, Range(1e-9, 10.0), 100));
        return controller.loopShapingResult()->controller()->gain().nominal();
    };
    EXPECT_DOUBLE_EQ(solve(true), solve(false));
}
