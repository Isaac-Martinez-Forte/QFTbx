// End-to-end characterisation: the plants recovered from the real .qft
// fixtures must match the documented examples (cervera: ACC Cervera & Baños
// 2013; planta1: PFC example plant 1).

#include <gtest/gtest.h>

#include <complex>

#include <QPointF>
#include "src/core/range.h"
#include <QString>

#include "src/core/system/lti_system.h"
#include "src/core/system/parameter.h"
#include "src/core/frequencies/omega.h"
#include "src/persistence/project_reader.h"

namespace {

using Complex = std::complex<qreal>;

constexpr qreal kTolerance = 1e-9;

TEST(PlantFixture, CerveraRoundTrip)
{
    ProjectReader parser;
    delete parser.load(QString(QFTBX_TEST_DATA_DIR "/cervera.qft"));

    LtiSystem* planta = parser.plant();
    ASSERT_NE(planta, nullptr);

    EXPECT_EQ(planta->type(), LtiSystem::SystemType::FreeForm);
    EXPECT_EQ(planta->numeratorString(), QStringLiteral("a"));
    EXPECT_EQ(planta->denominatorString(), QStringLiteral("(s^2)*((s^2) + a)"));

    ASSERT_EQ(planta->numerator().size(), 1);
    Parameter & a = planta->numerator()[0];
    EXPECT_TRUE(a.isUncertain());
    EXPECT_EQ(a.name(), QStringLiteral("a"));
    EXPECT_DOUBLE_EQ(a.rawNominal(), 2.0);
    EXPECT_EQ(a.rawRange(), Range(0.5, 2.0));

    // Numerator and denominator carry two distinct Parameter objects that share
    // the same name (the template map is keyed by NAME, which is why the
    // numerator and denominator can hold two separate parameters with the
    // same name and the sweep still drives both).
    ASSERT_EQ(planta->denominator().size(), 1);
    EXPECT_NE(&planta->denominator()[0], &a);
    EXPECT_EQ(planta->denominator()[0].name(), QStringLiteral("a"));

    EXPECT_FALSE(planta->gain().isUncertain());
    EXPECT_DOUBLE_EQ(planta->gain().nominal(), 1.0);
    EXPECT_FALSE(planta->delay().isUncertain());
    EXPECT_DOUBLE_EQ(planta->delay().nominal(), 0.0);

    // P(j0.1) with the nominal a = 2.
    const Complex s(0.0, 0.1);
    const Complex expected = 2.0 / ((s * s) * (s * s + 2.0));
    const Complex value = planta->evaluate(0.1);
    EXPECT_NEAR(value.real(), expected.real(), kTolerance);
    EXPECT_NEAR(value.imag(), expected.imag(), kTolerance);

    Omega* omega = parser.omega();
    ASSERT_NE(omega, nullptr);
    ASSERT_EQ(omega->values()->size(), 4);
}

TEST(PlantFixture, Planta1RoundTrip)
{
    ProjectReader parser;
    delete parser.load(QString(QFTBX_TEST_DATA_DIR "/planta1.qft"));

    LtiSystem* planta = parser.plant();
    ASSERT_NE(planta, nullptr);

    EXPECT_EQ(planta->type(), LtiSystem::SystemType::ZeroPoleGain);
    EXPECT_TRUE(planta->numerator().empty());
    ASSERT_EQ(planta->denominator().size(), 2);
    EXPECT_EQ(planta->denominator()[0].name(), QStringLiteral("a"));
    EXPECT_EQ(planta->denominator()[1].name(), QStringLiteral("b"));

    EXPECT_TRUE(planta->gain().isUncertain());
    EXPECT_EQ(planta->gain().name(), QStringLiteral("kv"));
    EXPECT_EQ(planta->gain().rawRange(), Range(1.0, 10.0));

    EXPECT_EQ(planta->expression(0.1),
              QStringLiteral("kv*(1) / (((0.1*i) + a) *((0.1*i) + b))"));

    // Nominals kv=1, a=5, b=30 at s = 0.1j.
    const Complex s(0.0, 0.1);
    const Complex expected = 1.0 / ((s + 5.0) * (s + 30.0));
    const Complex value = planta->evaluate(0.1);
    EXPECT_NEAR(value.real(), expected.real(), kTolerance);
    EXPECT_NEAR(value.imag(), expected.imag(), kTolerance);
}

} // namespace
