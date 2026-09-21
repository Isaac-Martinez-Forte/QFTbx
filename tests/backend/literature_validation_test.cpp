/**
 * @file
 * @brief Validates the feasibility test against published designs of example 2.
 *
 * The projection, the boundary union and the detector must agree with the
 * literature about which controllers satisfy the MATLAB QFT Toolbox design
 * example 2, or every algorithm built on them optimises the wrong problem.
 * The references are Tharewal 2005 (doctoral thesis, IIT Bombay), example
 * 3.1, with the interval optimum 3462219 (s+3.85) / ((s+931.27)(s+946.83)),
 * Chen and Ballance's genetic design 6753000 (s+2) / ((s+2930)(s+553)), and
 * qftex2.m for the specifications, whose upper tracking model is the 0.6584
 * second-order one. Both designs must be feasible, 0.8 times Tharewal's gain
 * infeasible since the optimum hugs the tracking bound, and bisection on his
 * zero and poles must reproduce his gain within the fixture's discretisation.
 */

#include <gtest/gtest.h>

#include <string>

#include <cmath>
#include <complex>
#include <vector>
#include <initializer_list>

#include "src/core/math/point.h"

#include "src/app/project_controller.h"
#include "src/core/loopshaping/common/natural_interval_extension.h"
#include "src/core/loopshaping/common/boundary_violation_detector.h"
#include "src/core/system/zero_pole_gain.h"
#include "src/core/system/parameter.h"

using namespace qftbx;

namespace {

LtiSystem* zpk(double k, std::initializer_list<double> zeros,
               std::initializer_list<double> poles)
{
    std::vector<Parameter> nume;
    for (double z : zeros) {
        nume.push_back(Parameter(z));
    }
    std::vector<Parameter> deno;
    for (double p : poles) {
        deno.push_back(Parameter(p));
    }
    return new ZeroPoleGain(std::string("ref"), nume, deno,
                            Parameter(k), Parameter(double(0)));
}

class LiteratureValidation : public ::testing::Test
{
protected:
    void SetUp() override
    {
        controller.load(
            std::string(QFTBX_TEST_DATA_DIR "/qft_toolbox_ex2.qft"));
    }

    qftbx::BoxFlag classify(LtiSystem* point)
    {
        std::vector<double>* omega = controller.omega()->values();
        qftbx::BoxFlag overall = qftbx::feasible;

        for (int i = 0; i < omega->size(); ++i) {
            const std::complex<double> pv = controller.plant()->evaluate(omega->at(i));
            const NicholsBox box = conversion.nicholsBox(
                point, omega->at(i), std::complex<double>(pv.real(), pv.imag()));
            const qftbx::BoxFlag flag = detector.classifyBox(
                box, controller.boundaries(), i).flag();

            if (flag == qftbx::infeasible) {
                return qftbx::infeasible;
            }
            if (flag == qftbx::ambiguous) {
                overall = qftbx::ambiguous;
            }
        }

        return overall;
    }

    ProjectController controller;
    NaturalIntervalExtension conversion;
    BoundaryViolationDetector detector;
};

TEST_F(LiteratureValidation, PublishedControllersAreFeasible)
{
    LtiSystem* tharewal = zpk(3462219.0, {3.85}, {931.27, 946.83});
    LtiSystem* chenBallance = zpk(6753000.0, {2.0}, {2930.0, 553.0});

    EXPECT_EQ(classify(tharewal), qftbx::feasible);
    EXPECT_EQ(classify(chenBallance), qftbx::feasible);

    delete tharewal;
    delete chenBallance;
}

TEST_F(LiteratureValidation, LowerGainsAreInfeasible)
{
    LtiSystem* scaled = zpk(0.8 * 3462219.0, {3.85}, {931.27, 946.83});
    LtiSystem* historical = zpk(1.0, {0.01}, {687.5});

    EXPECT_EQ(classify(scaled), qftbx::infeasible);
    EXPECT_EQ(classify(historical), qftbx::infeasible);

    delete scaled;
    delete historical;
}

TEST_F(LiteratureValidation, MinimalFeasibleGainMatchesTharewal)
{
    double low = 1e4;
    double high = 1e8;

    for (int i = 0; i < 40; ++i) {
        const double mid = std::sqrt(low * high);
        LtiSystem* point = zpk(mid, {3.85}, {931.27, 946.83});
        (classify(point) == qftbx::infeasible ? low : high) = mid;
        delete point;
    }

    EXPECT_NEAR(high, 3462219.0, 0.05 * 3462219.0);
}

}
