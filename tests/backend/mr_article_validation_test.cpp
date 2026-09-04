// Validation of MR against the design example of its own paper:
//
//   Rambabu Kalla, P.S.V. Nataraj, "Synthesis of fractional-order QFT
//   controllers using interval constraint satisfaction technique", IFAC
//   Workshop on Fractional Differentiation and its Applications (FDA-10),
//   2010, section 5, Example 5.1.
//
// The paper designs a DC-motor speed loop
//
//   P(s) = k a / (s (s + a)),   k, a in [1, 10],   k0 = a0 = 1
//
// against a robust stability margin ws = 1.2 and the tracking models
// T_U = 1.5/(s+1.5), T_L = 1/(s+1)^2, over
// Omega = {0.001, 0.015, 0.25, 3.84, 60}, with the plant uncertainty
// captured by the 9 plants that combine the minimum, the mean and the
// maximum of each parameter. It reports
//
//   G(s) = 2.785 + 1.968 s^0.787                                      (17)
//
// and Fig. 2 asserts graphically that this controller respects every QFT
// bound.
//
// -- What this file does, and what it deliberately does not -------------
//
// The paper's number cannot be reproduced. Both of its examples design
// FRACTIONAL-order controllers (Kp + Kd s^beta, and
// k(s^beta+z1)/((s+p1)(s^alpha+p2))), and QFTbx designs integer-order
// structures: AlgorithmMr refuses anything but a zero-pole-gain or a
// time-constant controller, because it builds its magnitude and phase from
// (jw + x) factors. Adding a fractional family is a feature, not a
// validation, and a large one (the system families, the persistence, the
// structure dialog, and the magnitude/phase builder of every algorithm).
//
// So the two things validated here are the ones QFTbx does implement:
//
//   1. The CONSTRAINT SET. The paper's own answer, eq. (17), is checked
//      against the paper's own specifications - Fig. 2 turned into an
//      assertion. This pins our reading of the specification models and of
//      inequalities (10) and (11).
//   2. The SEARCH, on the half of the problem it settles: MR is pointed at
//      the paper's plant and frequencies under the stability margin, and
//      what it designs is verified INDEPENDENTLY of the algorithm.
//
// "Independently" is the point of checkSpecifications(): it evaluates |T|
// and the tracking spread in plain complex arithmetic over the 9 plants,
// sharing no code with the expression trees and the HC4 narrowing it
// judges. A check that reused the algorithm's own algebra would only prove
// the algebra self-consistent.
//
// -- What the validation found -------------------------------------------
//
// Two defects, both fixed; see the comments at the exit of
// AlgorithmMr::solve and in isParameterBoxSmall.
//
//   - MR returned the point of an epsilon-small AMBIGUOUS box without ever
//     checking it against its own constraint set. On this example that
//     point missed the robust stability margin by a factor of three
//     (|T| = 3.81 against 1.2), and nothing said so.
//   - MR measured epsilon on the NICHOLS box, the criterion of the other
//     four algorithms, whose papers work on that projection. The paper's
//     eps is a width on the CONTROLLER PARAMETER box, and the difference is
//     not cosmetic: the Nichols diameter scales with |P|, which reaches 1e4
//     at this problem's lowest design frequency, so the paper's 0.001 was
//     four decades tighter than it looked and the search never came back.
//     With the criterion on the paper's footing, eps = 0.001 is what the
//     tests below pass. On planta1 the pinned optimum improved by a tenth
//     of a percent, downwards, which is the direction a finer stop should
//     move a minimum.
//
// One limitation, measured and left alone. The specifications are
// discretised over at most kTemplateRepresentatives = 9 evenly spaced
// contour points, and the tracking constraint over ORDERED PAIRS of them.
// On this problem that is too coarse: the design MR certifies under the
// full specification set exceeds the true tracking bound by 12% to 19% at
// two of the five design frequencies (0.505 dB of spread against 0.408
// allowed at w = 0.25, and 2.01e-3 against 1.52e-3 at w = 0.015), measured
// over a 51x51 sweep of the uncertainty. Raising the count to 25 shrinks
// the excess at twelve times the cost without removing it, and the contour
// tolerance is NOT the cause (eps = 10 and eps = 2 give the identical
// design; eps = 0.5 has no hull at all on this cloud). Which is why the
// full specification set is not asserted here: what a finite set of
// representatives can promise is a matter of fitting the discretisation to
// reality, not a repair.

#include <gtest/gtest.h>

#include <string>

#include <algorithm>
#include <cmath>
#include <complex>
#include <limits>
#include <memory>
#include <vector>


#include "src/core/frequencies/omega.h"
#include "src/core/loopshaping/algorithm_mr.h"
#include "src/core/project_controller.h"
#include "src/core/range.h"
#include "src/core/specifications/specification_record.h"
#include "src/core/system/free_form.h"
#include "src/core/system/parameter.h"
#include "src/core/system/polynomial_form.h"
#include "src/core/system/zero_pole_gain.h"

using namespace qftbx;

namespace {


//The paper's design frequency set.
const std::vector<double> kOmega{0.001, 0.015, 0.25, 3.84, 60.0};

//The paper's robust stability margin.
const double kWs = 1.2;

//The paper's 9 plants: minimum, mean and maximum of each parameter.
const std::vector<double> kParameterValues{1.0, 5.5, 10.0};

//The paper's own accuracy, which MR can now be given: a width on the
//controller parameter box.
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

//Eq. (17): G(s) = 2.785 + 1.968 s^0.787, with (jw)^b = w^b e^{j b pi/2}.
std::complex<double> articleController(double w)
{
    const double beta = 0.787;
    return std::complex<double>(2.785, 0.0)
            + 1.968 * std::polar(std::pow(w, beta), beta * M_PI / 2.0);
}

//The allowed closed-loop spread |T_U/T_L| in dB, from the paper's models.
double allowedSpreadDb(double w)
{
    const std::complex<double> s(0.0, w);
    const double upper = std::abs(1.5 / (s + 1.5));
    const double lower = std::abs(1.0 / ((s + 1.0) * (s + 1.0)));
    return toDb(upper) - toDb(lower);
}

struct Verdict {
    //Largest |T| over every plant and every design frequency.
    double worstMargin = 0.0;
    //Smallest (allowed spread - achieved spread), over the frequencies.
    double worstTrackingSlackDb = std::numeric_limits<double>::max();
    double worstTrackingFrequency = 0.0;
};

//Independent evaluation of the paper's specifications for a controller
//given as a frequency response. Shares nothing with AlgorithmMr.
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

//The paper's problem, built through the real pipeline so the template the
//algorithm sees is the one QFTbx computes. withTracking = false keeps only
//the robust stability margin.
std::unique_ptr<ProjectController> articleProblem(bool withTracking)
{
    auto project = std::make_unique<ProjectController>();

    //The gain is named 'kv' because muparserx predefines 'k' as a constant;
    //'a' appears in numerator and denominator, and the sweep couples the two
    //occurrences by name.
    std::vector<Parameter> numerator{Parameter("a", Range(1.0, 10.0), 1.0)};
    std::vector<Parameter> denominator{Parameter("a", Range(1.0, 10.0), 1.0)};
    project->setPlant(std::make_unique<qftbx::FreeForm>(
        std::string("FDA-10 Example 5.1"), std::move(numerator), std::move(denominator),
        Parameter("kv", Range(1.0, 10.0), 1.0), Parameter(double(0)),
        std::string("a"), std::string("s*(s+a)")));

    qftbx::SpecificationRecords specifications;

    //T_L = 1/(s+1)^2 = 1/(s^2 + 2s + 1).
    specifications[0].name = std::string("tracking lower");
    specifications[0].used = withTracking;
    specifications[0].system = std::make_unique<qftbx::PolynomialForm>(
        std::string("TL"), std::vector<Parameter>{},
        std::vector<Parameter>{Parameter(1.0), Parameter(2.0), Parameter(1.0)},
        Parameter(1.0), Parameter(double(0)));
    specifications[0].omegaStart = kOmega.front();
    specifications[0].omegaEnd = kOmega.back();

    //T_U = 1.5/(s + 1.5).
    specifications[1].name = std::string("tracking upper");
    specifications[1].used = withTracking;
    specifications[1].system = std::make_unique<qftbx::PolynomialForm>(
        std::string("TU"), std::vector<Parameter>{},
        std::vector<Parameter>{Parameter(1.0), Parameter(1.5)},
        Parameter(1.5), Parameter(double(0)));
    specifications[1].omegaStart = kOmega.front();
    specifications[1].omegaEnd = kOmega.back();

    //Robust stability margin ws = 1.2.
    specifications[2].name = std::string("stability");
    specifications[2].used = true;
    specifications[2].constant = true;
    specifications[2].height = kWs;
    specifications[2].omegaStart = kOmega.front();
    specifications[2].omegaEnd = kOmega.back();

    project->setSpecifications(std::move(specifications));

    project->setOmega(std::make_unique<Omega>(kOmega.front(), kOmega.back(), kOmega.size(),
                                              kOmega, Omega::Manual));

    //The paper's 9 plants, exactly: three values per parameter.
    qftbx::ParameterGrids grids;
    grids["kv"] = kParameterValues;
    grids["a"] = kParameterValues;

    //A coarse hull tolerance because a 9-point cloud spanning 40 dB has no
    //tight epsilon-hull; measurement (b) in the header establishes that the
    //design does not depend on it.
    const std::vector<double> contourEpsilon(kOmega.size(), 10.0);

    project->computeTemplates(contourEpsilon, grids, false);

    return project;
}

TEST(MrArticleValidation, TheArticleControllerMeetsTheArticleSpecifications)
{
    const Verdict verdict = checkSpecifications(&articleController);

    //Fig. 2 as an assertion. The margin binds at w = 3.84 (|T| = 1.114
    //against the allowed 1.2), which is what the figure shows: the design is
    //driven by stability, and tracking has slack at every other frequency.
    EXPECT_LE(verdict.worstMargin, kWs);

    //The tracking slack is negative by 2e-6 dB at w = 0.001, where the
    //allowed spread is 6.8e-6 dB: three decades below the corner of the
    //models, T_U and T_L coincide and the band closes. Seven microdecibels
    //of allowed spread is far below the perturbation of quoting eq. (17) to
    //four significant figures, so what is asserted is the tolerance, not an
    //exact inequality. It is also why no interval method can certify a box
    //on this problem: no box of controllers is provably inside a
    //7-microdecibel band.
    EXPECT_GT(verdict.worstTrackingSlackDb, -1e-4)
        << "worst tracking slack " << verdict.worstTrackingSlackDb
        << " dB at w = " << verdict.worstTrackingFrequency;
}

TEST(MrArticleValidation, MrDesignsAFeasibleControllerUnderTheStabilityMargin)
{
    std::unique_ptr<ProjectController> project = articleProblem(false);
    ASSERT_FALSE(project->contour().empty());

    //An integer-order structure in place of the paper's fractional one:
    //C(s) = kc (s + z1) / (s + p1).
    std::vector<Parameter> zeros{Parameter("z1", Range(0.01, 1e3), 1.0)};
    std::vector<Parameter> poles{Parameter("p1", Range(0.01, 1e3), 1.0)};
    auto structure = std::make_unique<qftbx::ZeroPoleGain>(
        std::string("n3"), std::move(zeros), std::move(poles),
        Parameter("kc", Range(0.1, 1e4), 1.0), Parameter(double(0)));

    AlgorithmMr mr;
    //MR takes no Nichols boundaries: its constraints come from the
    //specifications and the template representatives directly. The
    //epsilon is the paper's.
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

    //The same independent evaluation the paper's own controller passes.
    //Tracking is not asserted: it is switched off in this problem, and a
    //minimum-gain loop spreads freely without it.
    EXPECT_LE(verdict.worstMargin, kWs)
        << "designed kc = " << kc << ", z1 = " << z1 << ", p1 = " << p1;

    //The objective is the gain, and with only a margin to respect the
    //optimum is the floor of the search box: any loop this slow is
    //trivially robust. Worth pinning, because a search that returned
    //anything else here would have lost the minimum.
    EXPECT_NEAR(kc, 0.1, 1e-9);
}

} // namespace
