#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "src/core/common/text_tokens.h"
#include "src/core/math/expression_tree.h"
#include "src/core/math/formula.h"
#include "src/core/system/polynomial_form.h"
#include "src/core/system/system_formula.h"
#include "src/core/system/time_constant_gain.h"
#include "src/core/system/zero_pole_gain.h"

using namespace qftbx;

namespace {

const int kDigits = 4;

std::string latexOfText(const std::string & expression)
{
    return latexOf(formulaOfText(expression, kDigits));
}

} // namespace

TEST(Formula, RoundsWhatItShowsAndDropsThePadding)
{
    EXPECT_EQ(text::number(567.3175312139062, 4), "567.3");
    EXPECT_EQ(text::number(1.5, 4), "1.5");
    EXPECT_EQ(text::number(-6.988436109557454e-05, 4), "-6.988e-05");
    EXPECT_EQ(text::number(0.0, 4), "0");

    //Out of range is the full number, which is what a file keeps.
    EXPECT_EQ(text::number(567.3175312139062, 0), text::number(567.3175312139062));
    EXPECT_EQ(text::number(567.3175312139062, 99), text::number(567.3175312139062));
}

TEST(Formula, DivisionBecomesAFractionAndPowersRise)
{
    EXPECT_EQ(latexOfText("(s+1)/(s+2)"), "\\frac{s + 1}{s + 2}");
    EXPECT_EQ(latexOfText("s^2"), "s^{2}");
    EXPECT_EQ(latexOfText("sqrt(1+s)"), "\\sqrt{1 + s}");
    EXPECT_EQ(latexOfText("abs(s)"), "\\left|s\\right|");
    EXPECT_EQ(latexOfText("exp(-s)"), "e^{-s}");
    EXPECT_EQ(latexOfText("ln(s)"), "\\ln\\left(s\\right)");
    EXPECT_EQ(latexOfText("log10(s)"), "\\log_{10}\\left(s\\right)");
}

TEST(Formula, KeepsTheParenthesesThatChangeTheMeaningAndNoOthers)
{
    //A fraction and an exponent fence their own parts.
    EXPECT_EQ(latexOfText("(a*b)/(c*d)"), "\\frac{ab}{cd}");
    EXPECT_EQ(latexOfText("(a+b)*c"), "\\left(a + b\\right)c");
    EXPECT_EQ(latexOfText("a-(b-c)"), "a - \\left(b - c\\right)");
    EXPECT_EQ(latexOfText("a-b-c"), "a - b - c");
    EXPECT_EQ(latexOfText("(a+b)^2"), "\\left(a + b\\right)^{2}");
    EXPECT_EQ(latexOfText("2*s"), "2s");
    EXPECT_EQ(latexOfText("s*2"), "s \\cdot 2");
}

TEST(Formula, NamesTheTrailingDigitsAsASubscript)
{
    EXPECT_EQ(latexOfText("a1+a2"), "a_{1} + a_{2}");
    EXPECT_EQ(latexOfText("kv"), "\\mathit{kv}");
}

TEST(Formula, WritesTheConstantsAsConstants)
{
    EXPECT_EQ(latexOfText("pi*s"), "\\pi s");
    EXPECT_EQ(latexOf(formula::number(1.5e-06, 4)), "1.5 \\cdot 10^{-6}");
    EXPECT_EQ(latexOf(formula::number(1e-06, 4)), "10^{-6}");
}

TEST(Formula, AnUnreadableExpressionGivesNothing)
{
    EXPECT_TRUE(formula::isEmpty(formulaOfText("(s+1", kDigits)));
    EXPECT_TRUE(formula::isEmpty(formulaOfText("", kDigits)));
}

TEST(Formula, EachFamilyIsWrittenTheWayItsOwnLiteratureWritesIt)
{
    const Range range(1.0, 10.0);

    ZeroPoleGain zpk("plant", {}, {Parameter(double(0)), Parameter("a", range, 1.0)},
                     Parameter("k", range, 1.0), Parameter(double(0)));
    EXPECT_EQ(latexOf(formulaOf(static_cast<LtiSystem &>(zpk), kDigits)),
              "k\\frac{1}{s\\left(s + a\\right)}");

    TimeConstantGain tcg("plant", {}, {Parameter(double(2))}, Parameter(double(5)),
                         Parameter(double(0)));
    EXPECT_EQ(latexOf(formulaOf(static_cast<LtiSystem &>(tcg), kDigits)),
              "5 \\cdot \\frac{1}{\\left(1 + \\frac{s}{2}\\right)}");

    //A zero coefficient drops its term, a unit one drops its factor, and an
    //uncertain one keeps both.
    PolynomialForm poly("plant", {Parameter(double(1)), Parameter(double(0)), Parameter(double(3))},
                        {Parameter(double(1)), Parameter("b", range, 2.0)},
                        Parameter(double(1)), Parameter(double(0)));
    EXPECT_EQ(latexOf(formulaOf(static_cast<LtiSystem &>(poly), kDigits)),
              "\\frac{s^{2} + 3}{s + b}");
}

TEST(Formula, AnEmptyPolynomialIsTheOneTheEvaluationUses)
{
    //Every family evaluates an empty coefficient list as 1 (the product or
    //the Horner loop starts there), and the formula has to say the same
    //thing: written as 0, a numerator nobody filled in made the bound
    //vanish on screen.
    PolynomialForm empty("plant", {}, {Parameter(double(1))},
                         Parameter(double(1)), Parameter(double(0)));
    EXPECT_EQ(latexOf(formulaOf(static_cast<LtiSystem &>(empty), kDigits)),
              "\\frac{1}{1}");
}

TEST(Formula, TheDelayIsAnExponentAndTheUnitGainIsNotWritten)
{
    PolynomialForm delayed("plant", {Parameter(double(1))}, {Parameter(double(1))},
                           Parameter(double(1)), Parameter(double(0.05)));
    EXPECT_EQ(latexOf(formulaOf(static_cast<LtiSystem &>(delayed), kDigits)),
              "\\frac{1}{1}e^{-0.05s}");
}
