/**
 * @file
 * @brief Validates algorithm MR against the design example of its paper.
 *
 * Kalla and Nataraj, "Synthesis of fractional-order QFT controllers using
 * interval constraint satisfaction technique", FDA-10, 2010, example 5.1,
 * with its stability margin of 1.2, its two tracking models and its nine
 * plants. Their controller is fractional-order and QFTbx designs
 * integer-order structures, so their number cannot be reproduced. Validated
 * instead are the constraint set, by asserting their own controller against
 * their own specifications, and the search under the margin alone, whose
 * design is checked in complex arithmetic sharing nothing with the algorithm
 * and must be the floor of the gain box; tracking is not asserted on it,
 * nine template representatives being too coarse for it on this problem.
 */

#include <gtest/gtest.h>

#include <string>

#include <algorithm>
#include <cmath>
#include <complex>
#include <limits>
#include <memory>
#include <vector>

#include "src/core/frequencies/omega.h"
#include "src/core/loopshaping/mr/algorithm_mr.h"
#include "src/app/project_controller.h"
#include "src/core/math/range.h"
#include "src/core/specifications/specification_record.h"
#include "src/core/system/free_form.h"
#include "src/core/system/parameter.h"
#include "src/core/system/polynomial_form.h"
#include "src/core/system/zero_pole_gain.h"

using namespace qftbx;

namespace {

const std::vector<double> kOmega{0.001, 0.015, 0.25, 3.84, 60.0};

const double kWs = 1.2;

const std::vector<double> kParameterValues{1.0, 5.5, 10.0};

const double kEpsilon = 0.001;

double toDb(double magnitude)
{
    return 20.0 * std::log10(magnitude);
}

std::complex<double> articlePlant(double k, double a, double w)
{
    const std::complex<double> s(0.0, w);
    return (k * a) / (s * (s + a));
}

std::complex<double> articleController(double w)
{
    const double beta = 0.787;
    return std::complex<double>(2.785, 0.0)
            + 1.968 * std::polar(std::pow(w, beta), beta * M_PI / 2.0);
}

double allowedSpreadDb(double w)
{
    const std::complex<double> s(0.0, w);
    const double upper = std::abs(1.5 / (s + 1.5));
    const double lower = std::abs(1.0 / ((s + 1.0) * (s + 1.0)));
    return toDb(upper) - toDb(lower);
}

struct Verdict {
    double worstMargin = 0.0;
    double worstTrackingSlackDb = std::numeric_limits<double>::max();
    double worstTrackingFrequency = 0.0;
};

template <class Controller>
Verdict checkSpecifications(const Controller & controller)
{
    Verdict verdict;

    for (const double w : kOmega) {

        double maxTdb = -std::numeric_limits<double>::max();
        double minTdb = std::numeric_limits<double>::max();

        for (const double k : kParameterValues) {
            for (const double a : kParameterValues) {
                const std::complex<double> l = articlePlant(k, a, w) * controller(w);
                const double t = std::abs(l / (1.0 + l));
                verdict.worstMargin = std::max(verdict.worstMargin, t);
                maxTdb = std::max(maxTdb, toDb(t));
                minTdb = std::min(minTdb, toDb(t));
            }
        }

        const double slack = allowedSpreadDb(w) - (maxTdb - minTdb);
        if (slack < verdict.worstTrackingSlackDb) {
            verdict.worstTrackingSlackDb = slack;
            verdict.worstTrackingFrequency = w;
        }
    }

    return verdict;
}

std::unique_ptr<ProjectController> articleProblem(bool withTracking)
{
    auto project = std::make_unique<ProjectController>();

    std::vector<Parameter> numerator{Parameter("a", Range(1.0, 10.0), 1.0)};
    std::vector<Parameter> denominator{Parameter("a", Range(1.0, 10.0), 1.0)};
    project->setPlant(std::make_unique<qftbx::FreeForm>(
        std::string("FDA-10 Example 5.1"), std::move(numerator), std::move(denominator),
        Parameter("kv", Range(1.0, 10.0), 1.0), Parameter(double(0)),
        std::string("a"), std::string("s*(s+a)")));

    qftbx::SpecificationRecords specifications;

    specifications[0].name = std::string("tracking lower");
    specifications[0].used = withTracking;
    specifications[0].system = std::make_unique<qftbx::PolynomialForm>(
        std::string("TL"), std::vector<Parameter>{},
        std::vector<Parameter>{Parameter(1.0), Parameter(2.0), Parameter(1.0)},
        Parameter(1.0), Parameter(double(0)));
    specifications[0].omegaStart = kOmega.front();
    specifications[0].omegaEnd = kOmega.back();

    specifications[1].name = std::string("tracking upper");
    specifications[1].used = withTracking;
    specifications[1].system = std::make_unique<qftbx::PolynomialForm>(
        std::string("TU"), std::vector<Parameter>{},
        std::vector<Parameter>{Parameter(1.0), Parameter(1.5)},
        Parameter(1.5), Parameter(double(0)));
    specifications[1].omegaStart = kOmega.front();
    specifications[1].omegaEnd = kOmega.back();

    specifications[2].name = std::string("stability");
    specifications[2].used = true;
    specifications[2].constant = true;
    specifications[2].height = kWs;
    specifications[2].omegaStart = kOmega.front();
    specifications[2].omegaEnd = kOmega.back();

    project->setSpecifications(std::move(specifications));

    project->setOmega(std::make_unique<Omega>(kOmega.front(), kOmega.back(), kOmega.size(),
                                              kOmega, Omega::Manual));

    qftbx::ParameterGrids grids;
    grids["kv"] = kParameterValues;
    grids["a"] = kParameterValues;

    const std::vector<double> contourEpsilon(kOmega.size(), 10.0);

    project->computeTemplates(contourEpsilon, grids, false);

    return project;
}

TEST(MrArticleValidation, TheArticleControllerMeetsTheArticleSpecifications)
{
    const Verdict verdict = checkSpecifications(&articleController);

    EXPECT_LE(verdict.worstMargin, kWs);

    EXPECT_GT(verdict.worstTrackingSlackDb, -1e-4)
        << "worst tracking slack " << verdict.worstTrackingSlackDb
        << " dB at w = " << verdict.worstTrackingFrequency;
}

TEST(MrArticleValidation, MrDesignsAFeasibleControllerUnderTheStabilityMargin)
{
    std::unique_ptr<ProjectController> project = articleProblem(false);
    ASSERT_FALSE(project->contour().empty());

    std::vector<Parameter> zeros{Parameter("z1", Range(0.01, 1e3), 1.0)};
    std::vector<Parameter> poles{Parameter("p1", Range(0.01, 1e3), 1.0)};
    auto structure = std::make_unique<qftbx::ZeroPoleGain>(
        std::string("n3"), std::move(zeros), std::move(poles),
        Parameter("kc", Range(0.1, 1e4), 1.0), Parameter(double(0)));

    AlgorithmMr mr;
    mr.setProblem(project->plant(), structure.get(), project->omega()->values(),
                 nullptr, kEpsilon, project->contour(), project->specifications());

    ASSERT_TRUE(mr.solve());

    const std::unique_ptr<LtiSystem> designed = mr.controllerStructure();
    ASSERT_NE(designed, nullptr);

    const double kc = designed->gain().range().min;
    const double z1 = designed->numerator().at(0).range().min;
    const double p1 = designed->denominator().at(0).range().min;

    const Verdict verdict = checkSpecifications([&](double w) {
        const std::complex<double> s(0.0, w);
        return kc * (s + z1) / (s + p1);
    });

    EXPECT_LE(verdict.worstMargin, kWs)
        << "designed kc = " << kc << ", z1 = " << z1 << ", p1 = " << p1;

    EXPECT_NEAR(kc, 0.1, 1e-9);
}

}
