/**
 * @file
 * @brief The plants in the sample .qft files match their published examples.
 *
 * `cervera.qft` is the ACC example of Cervera and Baños (2013) and
 * `planta1.qft` the first example plant of the final-year project. Each is loaded and checked
 * field by field, then evaluated at s = 0.1j against the value computed by
 * hand from the nominal parameters. The numerator and the denominator of the
 * free-form plant hold two distinct parameters of the same name, which is what
 * lets one sweep drive both.
 */

#include <gtest/gtest.h>

#include <string>

#include <complex>

#include "src/core/math/point.h"
#include "src/core/math/range.h"

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

    ASSERT_EQ(plant->denominator().size(), 1);
    EXPECT_NE(&plant->denominator()[0], &a);
    EXPECT_EQ(plant->denominator()[0].name(), std::string("a"));

    EXPECT_FALSE(plant->gain().isUncertain());
    EXPECT_DOUBLE_EQ(plant->gain().nominal(), 1.0);
    EXPECT_FALSE(plant->delay().isUncertain());
    EXPECT_DOUBLE_EQ(plant->delay().nominal(), 0.0);

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

    const Complex s(0.0, 0.1);
    const Complex expected = 1.0 / ((s + 5.0) * (s + 30.0));
    const Complex value = plant->evaluate(0.1);
    EXPECT_NEAR(value.real(), expected.real(), kTolerance);
    EXPECT_NEAR(value.imag(), expected.imag(), kTolerance);
}

}
