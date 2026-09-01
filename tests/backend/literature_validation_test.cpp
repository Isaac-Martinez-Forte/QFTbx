// Validation of the loop-shaping feasibility machinery against published
// solutions of the Matlab QFT Toolbox design example 2 (phase 8b.2b, at
// Isaac's request): the projection, the boundary union and the detection
// must agree with the literature about which controllers satisfy the
// specifications, or every algorithm built on them optimises the wrong
// problem.
//
// References:
// - Tharewal 2005 (IIT Bombay doctoral thesis), Example 3.1: the interval
//   optimum G_A(s) = 3462219 (s+3.85) / ((s+931.27)(s+946.83)) found in
//   the box ((0,1e8], (0,4000]^3), and Chen & Ballance's genetic-algorithm
//   design G_B(s) = 6753000 (s+2) / ((s+2930)(s+553)).
// - qftdemos/qftex2.m (documentos/qft_matlab): the classical loop-shaped
//   design and the specification definitions (also confirming T_U is the
//   0.6584 second-order model - the QFTbx thesis text swaps the names).
//
// A ground-truth check (dense plant sampling straight from the
// specifications, independent of this code base) confirms G_A and G_B
// satisfy margins and tracking, that G_A is bound-hugging (0.8 G_A already
// violates tracking), and that the k = 1 controller the historical
// inverted parity accepted violates tracking by +27 dB.

#include <gtest/gtest.h>

//A failed comparison of C-XSC values must report, not crash: see the header.
#include "tests/backend/cxsc_printing.h"

#include <cmath>
#include <complex>
#include <vector>
#include <initializer_list>

#include <QPointF>
#include <QString>
#include <QVector>

#include "src/core/project_controller.h"
#include "src/core/loopshaping/natural_interval_extension.h"
#include "src/core/loopshaping/boundary_violation_detector.h"
#include "src/core/system/zero_pole_gain.h"
#include "src/core/system/parameter.h"

namespace {

LtiSystem* zpk(qreal k, std::initializer_list<qreal> zeros,
               std::initializer_list<qreal> poles)
{
    std::vector<Parameter> nume;
    for (qreal z : zeros) {
        nume.push_back(Parameter(z));
    }
    std::vector<Parameter> deno;
    for (qreal p : poles) {
        deno.push_back(Parameter(p));
    }
    return new ZeroPoleGain(QStringLiteral("ref"), nume, deno,
                            Parameter(k), Parameter(qreal(0)));
}

class LiteratureValidation : public ::testing::Test
{
protected:
    void SetUp() override
    {
        delete controller.load(
            QStringLiteral(QFTBX_TEST_DATA_DIR "/qft_toolbox_ex2.qft"));
    }

    //Overall feasibility of a point controller against the fixture's
    //boundaries, with the same projection + detection the algorithms use.
    tools::BoxFlag classify(LtiSystem* point)
    {
        QVector<qreal>* omega = controller.omega()->values();
        tools::BoxFlag overall = tools::feasible;

        for (int i = 0; i < omega->size(); ++i) {
            const std::complex<qreal> pv = controller.plant()->evaluate(omega->at(i));
            const cxsc::cinterval box = conversion.nicholsBox(
                point, omega->at(i), cxsc::complex(pv.real(), pv.imag()));
            BoxClassification* datos = deteccion.classifyBox(
                box, controller.boundaries(), i);
            const tools::BoxFlag flag = datos->flag();
            delete datos;

            if (flag == tools::infeasible) {
                return tools::infeasible;
            }
            if (flag == tools::ambiguous) {
                overall = tools::ambiguous;
            }
        }

        return overall;
    }

    ProjectController controller;
    NaturalIntervalExtension conversion;
    BoundaryViolationDetector deteccion;
};

TEST_F(LiteratureValidation, PublishedControllersAreFeasible)
{
    LtiSystem* tharewal = zpk(3462219.0, {3.85}, {931.27, 946.83});
    LtiSystem* chenBallance = zpk(6753000.0, {2.0}, {2930.0, 553.0});

    EXPECT_EQ(classify(tharewal), tools::feasible);
    EXPECT_EQ(classify(chenBallance), tools::feasible);

    delete tharewal;
    delete chenBallance;
}

TEST_F(LiteratureValidation, LowerGainsAreInfeasible)
{
    //The published optimum hugs the bounds: reducing its gain violates
    //the tracking specification (ground truth: +1.87 dB at 0.8 k*).
    LtiSystem* scaled = zpk(0.8 * 3462219.0, {3.85}, {931.27, 946.83});
    LtiSystem* historical = zpk(1.0, {0.01}, {687.5});

    EXPECT_EQ(classify(scaled), tools::infeasible);
    //The controller the inverted open-boundary parity used to accept
    //violates tracking by +27 dB.
    EXPECT_EQ(classify(historical), tools::infeasible);

    delete scaled;
    delete historical;
}

TEST_F(LiteratureValidation, MinimalFeasibleGainMatchesTharewal)
{
    //Bisection of the minimal feasible gain for Tharewal's exact
    //zero/poles: the detection reproduces his published optimum within
    //the template/boundary discretisation of the fixture (measured 1.3%).
    qreal low = 1e4;
    qreal high = 1e8;

    for (int i = 0; i < 40; ++i) {
        const qreal mid = std::sqrt(low * high);
        LtiSystem* point = zpk(mid, {3.85}, {931.27, 946.83});
        (classify(point) == tools::infeasible ? low : high) = mid;
        delete point;
    }

    EXPECT_NEAR(high, 3462219.0, 0.05 * 3462219.0);
}

} // namespace
