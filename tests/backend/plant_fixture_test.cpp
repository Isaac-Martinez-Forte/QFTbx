// End-to-end characterisation: the plants recovered from the real .qft
// fixtures must match the documented examples (cervera: ACC Cervera & Baños
// 2013; planta1: PFC example plant 1).

#include <gtest/gtest.h>

#include <string>

#include <complex>

#include "src/core/point.h"
#include "src/core/range.h"

#include "src/core/system/lti_system.h"
#include "src/core/system/parameter.h"
#include "src/core/frequencies/omega.h"
#include "src/persistence/project_reader.h"

using namespace qftbx;

namespace {

using Complex = std::complex<double>;

constexpr double kTolerance = 1e-9;

TEST(PlantFixture, CerveraRoundTrip)
{
    ProjectReader parser;
    parser.load(std::string(QFTBX_TEST_DATA_DIR "/cervera.qft"));

    LtiSystem* plant = parser.plant();
    ASSERT_NE(plant, nullptr);

    EXPECT_EQ(plant->type(), LtiSystem::SystemType::FreeForm);
    EXPECT_EQ(plant->numeratorString(), std::string("a"));
    EXPECT_EQ(plant->denominatorString(), std::string("(s^2)*((s^2) + a)"));

    ASSERT_EQ(plant->numerator().size(), 1);
    Parameter & a = plant->numerator()[0];
    EXPECT_TRUE(a.isUncertain());
    EXPECT_EQ(a.name(), std::string("a"));
    EXPECT_DOUBLE_EQ(a.rawNominal(), 2.0);
    EXPECT_EQ(a.rawRange(), Range(0.5, 2.0));

    // Numerator and denominator carry two distinct Parameter objects that share
    // the same name (the template map is keyed by NAME, which is why the
    // numerator and denominator can hold two separate parameters with the
    // same name and the sweep still drives both).
    ASSERT_EQ(plant->denominator().size(), 1);
    EXPECT_NE(&plant->denominator()[0], &a);
    EXPECT_EQ(plant->denominator()[0].name(), std::string("a"));

    EXPECT_FALSE(plant->gain().isUncertain());
    EXPECT_DOUBLE_EQ(plant->gain().nominal(), 1.0);
    EXPECT_FALSE(plant->delay().isUncertain());
    EXPECT_DOUBLE_EQ(plant->delay().nominal(), 0.0);

    // P(j0.1) with the nominal a = 2.
    const Complex s(0.0, 0.1);
    const Complex expected = 2.0 / ((s * s) * (s * s + 2.0));
    const Complex value = plant->evaluate(0.1);
    EXPECT_NEAR(value.real(), expected.real(), kTolerance);
    EXPECT_NEAR(value.imag(), expected.imag(), kTolerance);

    Omega* omega = parser.omega();
    ASSERT_NE(omega, nullptr);
    ASSERT_EQ(omega->values()->size(), 4);
}

TEST(PlantFixture, Planta1RoundTrip)
{
    ProjectReader parser;
    parser.load(std::string(QFTBX_TEST_DATA_DIR "/planta1.qft"));

    LtiSystem* plant = parser.plant();
    ASSERT_NE(plant, nullptr);

    EXPECT_EQ(plant->type(), LtiSystem::SystemType::ZeroPoleGain);
    EXPECT_TRUE(plant->numerator().empty());
    ASSERT_EQ(plant->denominator().size(), 2);
    EXPECT_EQ(plant->denominator()[0].name(), std::string("a"));
    EXPECT_EQ(plant->denominator()[1].name(), std::string("b"));

    EXPECT_TRUE(plant->gain().isUncertain());
    EXPECT_EQ(plant->gain().name(), std::string("kv"));
    EXPECT_EQ(plant->gain().rawRange(), Range(1.0, 10.0));

    // Nominals kv=1, a=5, b=30 at s = 0.1j.
    const Complex s(0.0, 0.1);
    const Complex expected = 1.0 / ((s + 5.0) * (s + 30.0));
    const Complex value = plant->evaluate(0.1);
    EXPECT_NEAR(value.real(), expected.real(), kTolerance);
    EXPECT_NEAR(value.imag(), expected.imag(), kTolerance);
}

} // namespace
