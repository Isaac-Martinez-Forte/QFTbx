// muParserX is built to parse once and evaluate many times: SetExpr() lexes
// the text and builds an RPN, the first Eval() switches the engine to
// ParseFromRPN, and every Eval() afterwards walks that RPN. The toolbox was
// not using it that way - it built a fresh parser and called SetExpr on every
// single evaluation - so this pins the pattern rather than trusting it.

#include <gtest/gtest.h>

#include <string>

#include <complex>
#include <vector>


#include "src/core/exception.h"
#include "src/core/math/expression_cache.h"
#include "src/core/system/parameter.h"
#include "src/core/range.h"
#include "mpParser.h"

namespace {

TEST(ExpressionCache, OneExpressionIsParsedOnceHoweverOftenItIsEvaluated)
{
    const int before = qftbx::math::cachedExpressionCount();

    const std::string expression = std::string("2*x + 1");
    const std::vector<std::string> names{std::string("x")};

    for (int i = 0; i < 50; i++) {
        const std::complex<double> got = qftbx::math::evaluateCached(
                expression, names, {std::complex<double>(i, 0.0)});
        EXPECT_DOUBLE_EQ(got.real(), 2.0 * i + 1.0);
    }

    //Fifty evaluations, one parser.
    EXPECT_EQ(qftbx::math::cachedExpressionCount(), before + 1)
        << "the expression was parsed more than once";
}

TEST(ExpressionCache, DifferentExpressionsGetDifferentParsers)
{
    const int before = qftbx::math::cachedExpressionCount();

    const std::vector<std::string> names{std::string("y")};
    qftbx::math::evaluateCached(std::string("y^2"), names, {std::complex<double>(3.0, 0.0)});
    qftbx::math::evaluateCached(std::string("y^3"), names, {std::complex<double>(3.0, 0.0)});

    EXPECT_EQ(qftbx::math::cachedExpressionCount(), before + 2);
}

TEST(ExpressionCache, TheValueIsTheONEThatWasJustGiven)
{
    //The bound value is overwritten per call; a cached parser holding a stale
    //value would answer the previous question.
    const std::string expression = std::string("z");
    const std::vector<std::string> names{std::string("z")};

    EXPECT_DOUBLE_EQ(qftbx::math::evaluateCached(expression, names,
                                                {std::complex<double>(7.0, 0.0)}).real(), 7.0);
    EXPECT_DOUBLE_EQ(qftbx::math::evaluateCached(expression, names,
                                                {std::complex<double>(-2.5, 0.0)}).real(), -2.5);
}

TEST(ExpressionCache, AComplexArgumentSurvives)
{
    //The free-form plants bind the Laplace variable as a complex value.
    const std::complex<double> got = qftbx::math::evaluateCached(
            std::string("__jw*__jw"), {std::string("__jw")},
            {std::complex<double>(0.0, 2.0)});

    EXPECT_NEAR(got.real(), -4.0, 1e-15);
    EXPECT_NEAR(got.imag(), 0.0, 1e-15);
}

TEST(ExpressionCache, AMismatchedValueListIsRejected)
{
    EXPECT_THROW(qftbx::math::evaluateCached(std::string("a+b"),
                                             {std::string("a"), std::string("b")},
                                             {std::complex<double>(1.0, 0.0)}),
                 qftbx::InvalidInput);
}

TEST(ExpressionCache, AReparametrisedParameterGoesThroughTheCache)
{
    //Parameter's reparametrisation is the other user-written expression in the
    //toolbox. range() evaluates it at both ends, which used to re-parse for
    //the second one because changing the variable binding throws the RPN away.
    Parameter parameter(std::string("t"), qftbx::Range(1.0, 4.0), 2.0,
                        std::string("10^t"));

    const int before = qftbx::math::cachedExpressionCount();

    const qftbx::Range range = parameter.range();
    EXPECT_DOUBLE_EQ(range.min, 10.0);
    EXPECT_DOUBLE_EQ(range.max, 10000.0);
    EXPECT_DOUBLE_EQ(parameter.nominal(), 100.0);

    //Three evaluations of one expression: one parser.
    EXPECT_LE(qftbx::math::cachedExpressionCount(), before + 1)
        << "the reparametrisation was parsed more than once";
}

TEST(ExpressionCache, AComplexReparametrisationIsReported)
{
    //A reparametrisation describes a real quantity. The historical code threw
    //an untyped muParserX error here; silently taking the real part would be
    //worse than either.
    //"alpha" and not "u": u is one of muParserX's unit operators, which is
    //exactly what isReservedName() is for.
    Parameter parameter(std::string("alpha"), qftbx::Range(1.0, 2.0), 1.0,
                        std::string("alpha*i"));

    try {
        parameter.nominal();
        FAIL() << "a complex reparametrisation was accepted";
    } catch (const qftbx::InvalidInput &) {
        //expected
    } catch (mup::ParserError & e) {
        FAIL() << "muParserX: " << e.GetMsg() << " expr=[" << e.GetExpr() << "]";
    } catch (const std::exception & e) {
        FAIL() << "std: " << e.what();
    }
}

TEST(ExpressionCache, TheNamesMuParserXOwnsAreReserved)
{
    //Constants.
    EXPECT_TRUE(qftbx::math::isReservedName(std::string("e")));
    EXPECT_TRUE(qftbx::math::isReservedName(std::string("pi")));
    EXPECT_TRUE(qftbx::math::isReservedName(std::string("i")));

    //Unit postfix operators - "k" is the obvious name for a gain.
    EXPECT_TRUE(qftbx::math::isReservedName(std::string("k")));
    EXPECT_TRUE(qftbx::math::isReservedName(std::string("m")));
    EXPECT_TRUE(qftbx::math::isReservedName(std::string("u")));

    //Functions, the only category the dialogs used to check.
    EXPECT_TRUE(qftbx::math::isReservedName(std::string("sin")));
    EXPECT_TRUE(qftbx::math::isReservedName(std::string("sqrt")));

    //And the names a user may actually use.
    EXPECT_FALSE(qftbx::math::isReservedName(std::string("a")));
    EXPECT_FALSE(qftbx::math::isReservedName(std::string("kv")));
    EXPECT_FALSE(qftbx::math::isReservedName(std::string("tau")));
    EXPECT_FALSE(qftbx::math::isReservedName(std::string("alpha")));
    EXPECT_FALSE(qftbx::math::isReservedName(std::string()));
}

TEST(ExpressionCache, AReservedNameReallyDoesBreakTheBinding)
{
    //Not a theory: binding a variable named "k" is what fails, which is why
    //the check exists.
    EXPECT_ANY_THROW(qftbx::math::evaluateCached(std::string("k+1"),
                                                 {std::string("k")},
                                                 {std::complex<double>(2.0, 0.0)}));
}

} // namespace
