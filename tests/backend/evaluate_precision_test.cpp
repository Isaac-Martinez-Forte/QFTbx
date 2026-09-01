// Does the plant evaluation keep the precision of its own coefficients?
//
// TransferFunction::evaluate(w) does not compute the transfer function: it
// builds a TEXTUAL expression of it and hands that to muParserX. The text is
// produced with QString::number(), whose default is 'g' with SIX significant
// digits. Measured loss: ~8e-7 relative, on the coefficients AND on the
// frequency.
//
// DISABLED on purpose. These two tests FAIL today and they are the evidence
// for a decision that is Isaac's: taking the expression parser out of the
// numeric core. Fixing it changes the numbers - more accurately - so the
// golden fixtures, produced by the old six-digit build, have to be re-pinned.
// Run them with --gtest_also_run_disabled_tests.
//
// Scope: this is the path of TransferFunction::evaluate, so it reaches the
// template clouds and the boundaries built from them. It does NOT reach the
// rigorous interval arithmetic of the loop-shaping algorithms, which
// NaturalIntervalExtension computes straight from the double values.

#include <gtest/gtest.h>

#include <complex>
#include <vector>

#include "src/core/system/parameter.h"
#include "src/core/system/polynomial_form.h"
#include "src/core/range.h"

namespace {

TEST(EvaluatePrecision, DISABLED_ACoefficientKeepsAllItsDigits)
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

TEST(EvaluatePrecision, DISABLED_TheFrequencyKeepsAllItsDigits)
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

} // namespace
