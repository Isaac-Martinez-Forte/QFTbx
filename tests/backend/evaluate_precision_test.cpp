// Does the plant evaluation keep the precision of its own coefficients?
//
// TransferFunction::evaluate(w) does not compute the transfer function: it
// builds a TEXTUAL expression of it and hands that to muParserX. The text is
// produced with QString::number(), whose default is 'g' with SIX significant
// digits. Measured loss: ~8e-7 relative, on the coefficients AND on the
// frequency.
//
// Both used to fail. valueAt() now computes the transfer function directly in
// complex arithmetic, so the coefficients and the frequency keep every digit.
//
// Scope: this is the path of TransferFunction::evaluate, so it reaches the
// template clouds and the boundaries built from them. It does NOT reach the
// rigorous interval arithmetic of the loop-shaping algorithms, which
// NaturalIntervalExtension computes straight from the double values.

#include <gtest/gtest.h>

#include "src/core/math/sequences.h"

#include <complex>
#include <vector>

#include "src/core/system/parameter.h"
#include "src/core/system/polynomial_form.h"
#include "src/core/system/free_form.h"
#include "src/core/exception.h"
#include "src/core/templates/template_engine.h"

#include <QHash>
#include "src/core/range.h"

namespace {

TEST(EvaluatePrecision, ACoefficientKeepsAllItsDigits)
{
    //P(s) = 1 / (a*s + 1) with a carrying 15 significant digits.
    const double a = 1.23456789012345;

    std::vector<Parameter> numerator{Parameter(1.0)};
    std::vector<Parameter> denominator{Parameter(a), Parameter(1.0)};
    PolynomialForm plant(QStringLiteral("precision"), numerator, denominator,
                         Parameter(1.0), Parameter(0.0));

    const double w = 1.0;
    const std::complex<double> got = plant.evaluate(w);

    //Exact: 1 / (a*jw + 1).
    const std::complex<double> exact =
        1.0 / (std::complex<double>(0.0, 1.0) * w * a + 1.0);

    //What the truncated coefficient would give instead.
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
    //The frequency goes into the same text, through the same conversion.
    std::vector<Parameter> numerator{Parameter(1.0)};
    std::vector<Parameter> denominator{Parameter(1.0), Parameter(1.0)};
    PolynomialForm plant(QStringLiteral("precision"), numerator, denominator,
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
    //A free-form plant can carry one variable in both its numerator and its
    //denominator - cervera's "a" does exactly that. They are the same
    //variable, so they must be given the same value; two different values is
    //an inconsistent request, and evaluating one of them would answer for a
    //plant nobody described.
    std::vector<Parameter> numerator{
        Parameter(QStringLiteral("a"), qftbx::Range(0.5, 2.0), 2.0)};
    std::vector<Parameter> denominator{
        Parameter(QStringLiteral("a"), qftbx::Range(0.5, 2.0), 2.0)};

    FreeForm plant(QStringLiteral("shared"), numerator, denominator,
                   Parameter(1.0), Parameter(0.0),
                   QStringLiteral("a"), QStringLiteral("(s) + a"));

    //Agreeing values: one variable, evaluated once.
    EXPECT_NO_THROW(plant.valueAt(1.0, {2.0}, {2.0}, 1.0, 0.0));

    //Disagreeing values for the same name.
    EXPECT_THROW(plant.valueAt(1.0, {2.0}, {0.5}, 1.0, 0.0), qftbx::InvalidInput);
}

TEST(EvaluatePrecision, TheTemplateSweepKeepsEveryDigitToo)
{
    //The sweep does not go through evaluate(): it used to build the plant's
    //expression per frequency with QString::number() and bind the SWEPT
    //parameters as variables. So the swept values were exact while the
    //frequency and the constant coefficients were rounded to six digits.
    //This pins the cloud itself against exact complex arithmetic.
    const double a = 1.23456789012345;
    const double w = 9.87654321098765;

    //P(s) = k / (a*s + 1), with k swept over a single grid point so the cloud
    //has exactly one point and can be compared value by value.
    std::vector<Parameter> numerator{Parameter(1.0)};
    std::vector<Parameter> denominator{Parameter(a), Parameter(1.0)};
    PolynomialForm plant(QStringLiteral("sweep"),
                         numerator, denominator,
                         Parameter(QStringLiteral("kv"), qftbx::Range(2.0, 2.0), 2.0),
                         Parameter(0.0));

    qftbx::ParameterGrids grids;
    grids[QStringLiteral("kv")] = {2.0};

    auto * frequencies = new std::vector<double>{w};
    TemplateEngine engine;
    engine.setEpsilon(std::vector<double>(1, 10.0));
    engine.setGrids(grids);

    const qftbx::CloudSet clouds = engine.computeClouds(&plant, frequencies);

    ASSERT_EQ(clouds.size(), 1u);
    ASSERT_EQ(clouds.at(0).size(), 1u);

    const std::complex<double> got = clouds.at(0).at(0);
    const std::complex<double> exact =
        2.0 / (std::complex<double>(0.0, 1.0) * w * a + 1.0);

    EXPECT_NEAR(got.real(), exact.real(), 1e-15) << "the cloud lost precision";
    EXPECT_NEAR(got.imag(), exact.imag(), 1e-15) << "the cloud lost precision";

    delete frequencies;
}

} // namespace
