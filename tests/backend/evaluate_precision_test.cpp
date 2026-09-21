/**
 * @file
 * @brief Tests that plant evaluation keeps every digit of its inputs.
 *
 * A coefficient and a frequency carrying fifteen significant digits must
 * come through the nominal evaluation and the template sweep to within
 * 1e-15 of exact complex arithmetic, which is what rules out any path that
 * goes through a textual rendering of the transfer function. A free-form
 * plant that names the same variable in numerator and denominator has one
 * variable, so two different values for it are refused. The rigorous
 * interval arithmetic of the loop-shaping algorithms is a separate path and
 * is not covered here.
 */

#include <gtest/gtest.h>

#include <string>

#include "src/core/common/text_tokens.h"
#include "src/core/math/sequences.h"

#include <complex>
#include <vector>

#include "src/core/system/parameter.h"
#include "src/core/system/polynomial_form.h"
#include "src/core/system/free_form.h"
#include "src/core/common/exception.h"
#include "src/core/templates/template_engine.h"

#include "src/core/math/range.h"

using namespace qftbx;

namespace {

TEST(EvaluatePrecision, ACoefficientKeepsAllItsDigits)
{
    const double a = 1.23456789012345;

    std::vector<Parameter> numerator{Parameter(1.0)};
    std::vector<Parameter> denominator{Parameter(a), Parameter(1.0)};
    PolynomialForm plant(std::string("precision"), numerator, denominator,
                         Parameter(1.0), Parameter(0.0));

    const double w = 1.0;
    const std::complex<double> got = plant.evaluate(w);

    const std::complex<double> exact =
        1.0 / (std::complex<double>(0.0, 1.0) * w * a + 1.0);

    const double truncated = 1.23457;
    const std::complex<double> withSixDigits =
        1.0 / (std::complex<double>(0.0, 1.0) * w * truncated + 1.0);

    EXPECT_NEAR(got.real(), exact.real(), 1e-15)
        << "real part lost precision; six-digit answer would be "
        << withSixDigits.real();
    EXPECT_NEAR(got.imag(), exact.imag(), 1e-15)
        << "imaginary part lost precision; six-digit answer would be "
        << withSixDigits.imag();
}

TEST(EvaluatePrecision, TheFrequencyKeepsAllItsDigits)
{
    std::vector<Parameter> numerator{Parameter(1.0)};
    std::vector<Parameter> denominator{Parameter(1.0), Parameter(1.0)};
    PolynomialForm plant(std::string("precision"), numerator, denominator,
                         Parameter(1.0), Parameter(0.0));

    const double w = 1.23456789012345;
    const std::complex<double> got = plant.evaluate(w);

    const std::complex<double> exact =
        1.0 / (std::complex<double>(0.0, 1.0) * w + 1.0);

    EXPECT_NEAR(got.real(), exact.real(), 1e-15) << "real part lost precision";
    EXPECT_NEAR(got.imag(), exact.imag(), 1e-15) << "imaginary part lost precision";
}

TEST(EvaluatePrecision, TheSameNameIsTheSameVariable)
{
    std::vector<Parameter> numerator{
        Parameter(std::string("a"), qftbx::Range(0.5, 2.0), 2.0)};
    std::vector<Parameter> denominator{
        Parameter(std::string("a"), qftbx::Range(0.5, 2.0), 2.0)};

    FreeForm plant(std::string("shared"), numerator, denominator,
                   Parameter(1.0), Parameter(0.0),
                   std::string("a"), std::string("(s) + a"));

    EXPECT_NO_THROW(plant.valueAt(1.0, {2.0}, {2.0}, 1.0, 0.0));

    EXPECT_THROW(plant.valueAt(1.0, {2.0}, {0.5}, 1.0, 0.0), qftbx::InvalidInput);
}

TEST(EvaluatePrecision, TheTemplateSweepKeepsEveryDigitToo)
{
    const double a = 1.23456789012345;
    const double w = 9.87654321098765;

    std::vector<Parameter> numerator{Parameter(1.0)};
    std::vector<Parameter> denominator{Parameter(a), Parameter(1.0)};
    PolynomialForm plant(std::string("sweep"),
                         numerator, denominator,
                         Parameter(std::string("kv"), qftbx::Range(2.0, 2.0), 2.0),
                         Parameter(0.0));

    qftbx::ParameterGrids grids;
    grids[std::string("kv")] = {2.0};

    std::vector<double> frequencies{w};
    TemplateEngine engine;
    engine.setEpsilon(std::vector<double>(1, 10.0));
    engine.setGrids(grids);

    const qftbx::CloudSet clouds = engine.computeClouds(&plant, &frequencies);

    ASSERT_EQ(clouds.size(), 1u);
    ASSERT_EQ(clouds.at(0).size(), 1u);

    const std::complex<double> got = clouds.at(0).at(0);
    const std::complex<double> exact =
        2.0 / (std::complex<double>(0.0, 1.0) * w * a + 1.0);

    EXPECT_NEAR(got.real(), exact.real(), 1e-15) << "the cloud lost precision";
    EXPECT_NEAR(got.imag(), exact.imag(), 1e-15) << "the cloud lost precision";

}

}
