// The closed form of the five magnitude specifications, against the sheet.
//
// At a fixed phase each specification is a quadratic in the gain for each
// plant (Chait and Yaniv 1993); the allowed set of the template is the exact
// intersection of the plants' sets (RangeUnion), multivalued where it is.
// The sheet samples the same worst case on a magnitude grid and cuts it at
// the bound with linear interpolation, so the two must agree to that
// interpolation's error, and where they disagree the closed form is the
// exact one.

#include <gtest/gtest.h>

#include <cmath>
#include <complex>
#include <cstdio>
#include <random>
#include <string>
#include <vector>

#include "src/app/project_controller.h"
#include "src/core/boundaries/boundary_columns.h"
#include "src/core/boundaries/closed_form_columns.h"
#include "src/core/boundaries/closed_loop_worst_case.h"
#include "src/core/boundaries/singular_locus.h"
#include "src/core/math/range.h"
#include "src/core/math/sequences.h"

using namespace qftbx;

namespace {

//The magnitude the specification bounds, at L = g e^{j phi}, for one plant.
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

} // namespace

TEST(ClosedFormColumns, TheQuadraticAgreesWithTheInequalityItSolves)
{
    //Random plants, phases and bounds; the closed-form set and the
    //inequality evaluated directly must agree at every gain except within
    //a hair of a root.
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
                    //Only acceptable right at a root: nudging g by 0.1 per
                    //cent must flip the direct verdict.
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
    //Example 2: stability at every frequency. The sheet columns (guard on,
    //as the engine computes them, 441 magnitude nodes of 0.5 dB) against
    //the closed form with the same guard (the segments between samples and
    //the inside of the polygon): every crossing within a fraction of a
    //cell, and where the interval counts differ the feature is narrower than
    //a cell, which is what the sheet cannot see. The largest deviation is
    //printed.
    ProjectController controller;
    controller.load(std::string(QFTBX_TEST_DATA_DIR "/qft_toolbox_ex2.qft"));
    const std::vector<double> & omega = *controller.omega()->values();
    const std::vector<double> phases = qftbx::math::linspace(-360.0, 0.0, 361);

    double worst = 0.0;
    std::size_t crossings = 0, mismatchedCounts = 0;
    for (std::size_t f = 0; f < omega.size(); ++f) {
        const std::complex<double> p0 = controller.plant()->evaluate(omega[f]);
        const ComplexCloud & cloud = controller.contour()[f];
        const std::vector<std::complex<double>> q = nominalOverValueSet(p0, cloud);
        //Example 2's stability bound is 1.2 at every frequency.
        const double boundDb = 20.0 * std::log10(1.2);

        const SingularLocus locus(q, true);
        const BoundaryColumns closed = ClosedFormColumns::columns(SpecificationType::Stability, boundDb, p0, cloud, q,
                                                                  phases, Range(-360.0, 0.0), &locus);
        //The sheet, sampled as the engine samples it, guard off.
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
                //A count the sheet cannot see: an allowed or forbidden band
                //narrower than a magnitude cell. Every interval on either
                //side that has no counterpart must be that narrow.
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
                //Or a gap: two intervals of 'more' that 'fewer' holds as one.
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
    //Boundaries from the sheet and from the closed form (guard on in both,
    //the sheet with its segment widening, the closed form with the inside
    //test), NT on each: gains within a tenth of a per cent, printed.
    const auto solve = [](bool closedForm) {
        ProjectController controller;
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
    //A cloud has no polygon, and its guard on the sheet has no closed form:
    //with the option on, boundaries from the cloud are the sheet's, and the
    //search lands where it always did (example 2, NT from the cloud).
    const auto solve = [](bool closedForm) {
        ProjectController controller;
        controller.load(std::string(QFTBX_TEST_DATA_DIR "/qft_toolbox_ex2.qft"));
        controller.setClosedFormColumns(closedForm);
        EXPECT_TRUE(controller.computeBoundaries(Range(-360.0, 0.0), 361, Range(-60.0, 160.0), 441, 1.0e6, false, false));
        EXPECT_TRUE(controller.computeLoopShaping(0.5, qftbx::nt, Range(1e-9, 10.0), 100));
        return controller.loopShapingResult()->controller()->gain().nominal();
    };
    EXPECT_DOUBLE_EQ(solve(true), solve(false));
}
