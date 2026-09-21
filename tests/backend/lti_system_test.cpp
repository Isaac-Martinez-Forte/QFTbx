/**
 * @file
 * @brief Tests of the system families: symbolic expression and evaluation.
 *
 * The four forms are zero-pole-gain, k prod(s+z)/prod(s+p); time-constant,
 * k prod(s/z+1)/prod(s/p+1); polynomial coefficients; and free text for
 * numerator and denominator. Each must evaluate at j omega to within 1e-9
 * of the form written by hand, with a fixed delay as e^(-s tau) in both the
 * value and the expression, a zero fixed delay omitted from the expression
 * and an uncertain delay of zero nominal kept in it by name. A variable gain
 * is written under its own name, clones are deep and independent of the
 * original, an unspecified delay is a zero value, a time-constant corner at
 * or straddling zero is refused when the system is built, and a value
 * vector of the wrong length is refused like a mismatched name.
 */

#include <gtest/gtest.h>

#include <string>

#include <complex>
#include <vector>

#include "src/core/math/point.h"
#include "src/core/math/range.h"

#include "src/core/system/polynomial_form.h"
#include "src/core/system/free_form.h"
#include "src/core/system/zero_pole_gain.h"
#include "src/core/system/time_constant_gain.h"
#include "src/core/system/lti_system.h"
#include "src/core/system/parameter.h"
#include "src/core/common/exception.h"

using namespace qftbx;

namespace {

bool endsWith(const std::string & text, const std::string & suffix)
{
    return text.size() >= suffix.size()
            && text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
}

using Complex = std::complex<double>;

constexpr double kTolerance = 1e-9;

std::vector<Parameter> vars(std::initializer_list<Parameter> list)
{
    return std::vector<Parameter>(list);
}

ZeroPoleGain* makePlanta1()
{
    return new ZeroPoleGain(
        std::string("aa"), vars({}),
        vars({Parameter(std::string("a"), Range(1.0, 5.0), 5.0, std::string("a")),
              Parameter(std::string("b"), Range(20.0, 30.0), 30.0, std::string("b"))}),
        Parameter(std::string("kv"), Range(1.0, 10.0), 1.0, std::string("kv")),
        Parameter(0.0));
}

TEST(kGainExpr, Class)
{
    ZeroPoleGain* plant = makePlanta1();
    EXPECT_EQ(plant->type(), LtiSystem::SystemType::ZeroPoleGain);
    delete plant;
}

TEST(kGainExpr, SymbolicExpressionOmitsZeroFixedDelay)
{
    ZeroPoleGain* plant = makePlanta1();
    EXPECT_EQ(plant->expression(), std::string("kv*(1) / ((s + a) *(s + b))"));
    delete plant;
}

TEST(kGainExpr, FixedDelayEvaluatesAsNegativeExponential)
{
    ZeroPoleGain plant(std::string("delayed"), vars({}), vars({Parameter(5.0)}),
                     Parameter(1.0), Parameter(0.5));

    const double w = 2.0;
    const Complex s(0.0, w);
    const Complex expected = std::exp(-s * 0.5) / (s + 5.0);

    const Complex value = plant.evaluate(w);
    EXPECT_NEAR(value.real(), expected.real(), kTolerance);
    EXPECT_NEAR(value.imag(), expected.imag(), kTolerance);

    EXPECT_TRUE(endsWith(plant.expression(), " * e^(-s*0.5)"));
}

TEST(kGainExpr, VariableDelayWithZeroNominalStaysInExpression)
{
    ZeroPoleGain plant(
        std::string("delayed"), vars({}), vars({Parameter(5.0)}), Parameter(1.0),
        Parameter(std::string("tau"), Range(0.0, 0.5), 0.0, std::string("tau")));

    EXPECT_TRUE(endsWith(plant.expression(), " * e^(-s*tau)"));
}

TEST(kGainExpr, NominalEvaluation)
{
    ZeroPoleGain* plant = makePlanta1();

    const Complex s(0.0, 0.1);
    const Complex expected = 1.0 / ((s + 5.0) * (s + 30.0));

    const Complex value = plant->evaluate(0.1);
    EXPECT_NEAR(value.real(), expected.real(), kTolerance);
    EXPECT_NEAR(value.imag(), expected.imag(), kTolerance);
    delete plant;
}

TEST(kGainExpr, CloneIsDeep)
{
    ZeroPoleGain* plant = makePlanta1();
    const std::unique_ptr<LtiSystem> copy = plant->clone();
    ASSERT_NE(copy, nullptr);

    EXPECT_EQ(copy->type(), LtiSystem::SystemType::ZeroPoleGain);
    EXPECT_EQ(copy->expression(), plant->expression());
    EXPECT_NE(&copy->gain(), &plant->gain());
    EXPECT_NE(&copy->numerator(), &plant->numerator());
    ASSERT_EQ(copy->denominator().size(), 2);
    EXPECT_NE(&copy->denominator()[0], &plant->denominator()[0]);
    EXPECT_EQ(copy->denominator()[0].name(), std::string("a"));

    delete plant;
}

TimeConstantGain* makeTimeConstantPlant()
{
    return new TimeConstantGain(std::string("tc"), vars({}),
                          vars({Parameter(10.0), Parameter(20.0)}), Parameter(5.0),
                          Parameter(0.0));
}

TEST(kNumeratorGainExpr, Class)
{
    TimeConstantGain* plant = makeTimeConstantPlant();
    EXPECT_EQ(plant->type(), LtiSystem::SystemType::TimeConstantGain);
    delete plant;
}

TEST(kNumeratorGainExpr, NominalEvaluationMatchesTimeConstantForm)
{
    TimeConstantGain* plant = makeTimeConstantPlant();

    const Complex s(0.0, 1.0);
    const Complex expected = 5.0 / ((s / 10.0 + 1.0) * (s / 20.0 + 1.0));

    const Complex value = plant->evaluate(1.0);
    EXPECT_NEAR(value.real(), expected.real(), kTolerance);
    EXPECT_NEAR(value.imag(), expected.imag(), kTolerance);
    delete plant;
}

TEST(kNumeratorGainExpr, VariableGainUsesItsRealName)
{
    TimeConstantGain plant(std::string("named"), vars({}), vars({Parameter(10.0)}),
                      Parameter(std::string("K1"), Range(1.0, 10.0), 5.0,
                              std::string("K1")),
                      Parameter(0.0));

    EXPECT_TRUE(plant.expression().rfind("K1*(", 0) == 0);

    const Complex s(0.0, 1.0);
    const Complex expected = 5.0 / (s / 10.0 + 1.0);
    const Complex value = plant.evaluate(1.0);
    EXPECT_NEAR(value.real(), expected.real(), kTolerance);
    EXPECT_NEAR(value.imag(), expected.imag(), kTolerance);
}

PolynomialForm* makePolynomialPlant()
{
    return new PolynomialForm(std::string("poly"), vars({Parameter(1.0)}),
                           vars({Parameter(1.0), Parameter(2.0), Parameter(3.0)}),
                           Parameter(1.0), Parameter(0.0));
}

TEST(CPolinomiosExpr, Class)
{
    PolynomialForm* plant = makePolynomialPlant();
    EXPECT_EQ(plant->type(), LtiSystem::SystemType::PolynomialForm);
    delete plant;
}

TEST(CPolinomiosExpr, NominalEvaluationMatchesPolynomialForm)
{
    PolynomialForm* plant = makePolynomialPlant();

    const Complex s(0.0, 2.0);
    const Complex expected = 1.0 / (s * s + 2.0 * s + 3.0);

    const Complex value = plant->evaluate(2.0);
    EXPECT_NEAR(value.real(), expected.real(), kTolerance);
    EXPECT_NEAR(value.imag(), expected.imag(), kTolerance);
    delete plant;
}

TEST(CPolinomiosExpr, VariableGainUsesItsRealName)
{
    PolynomialForm plant(std::string("named"), vars({Parameter(1.0)}),
                       vars({Parameter(1.0), Parameter(2.0)}),
                       Parameter(std::string("K1"), Range(1.0, 10.0), 2.0,
                               std::string("K1")),
                       Parameter(0.0));

    const Complex s(0.0, 1.0);
    const Complex expected = 2.0 / (s + 2.0);
    const Complex value = plant.evaluate(1.0);
    EXPECT_NEAR(value.real(), expected.real(), kTolerance);
    EXPECT_NEAR(value.imag(), expected.imag(), kTolerance);
}

TEST(CPolinomiosExpr, FixedDelayEvaluatesAsNegativeExponential)
{
    PolynomialForm plant(std::string("delayed"), vars({Parameter(1.0)}),
                       vars({Parameter(1.0), Parameter(2.0), Parameter(3.0)}),
                       Parameter(1.0), Parameter(0.7));

    const double w = 2.0;
    const Complex s(0.0, w);
    const Complex expected = std::exp(-s * 0.7) / (s * s + 2.0 * s + 3.0);

    const Complex value = plant.evaluate(w);
    EXPECT_NEAR(value.real(), expected.real(), kTolerance);
    EXPECT_NEAR(value.imag(), expected.imag(), kTolerance);

    EXPECT_TRUE(endsWith(plant.expression(), " * e^(-s*0.7)"));
}

TEST(kNumeratorGainExpr, FixedDelayEvaluatesAsNegativeExponential)
{
    TimeConstantGain plant(std::string("delayed"), vars({}), vars({Parameter(10.0)}),
                      Parameter(5.0), Parameter(0.3));

    const double w = 1.0;
    const Complex s(0.0, w);
    const Complex expected = 5.0 * std::exp(-s * 0.3) / (s / 10.0 + 1.0);

    const Complex value = plant.evaluate(w);
    EXPECT_NEAR(value.real(), expected.real(), kTolerance);
    EXPECT_NEAR(value.imag(), expected.imag(), kTolerance);
}

FreeForm* makeCerveraPlant()
{
    return new FreeForm(
        std::string("cervera"),
        vars({Parameter(std::string("a"), Range(0.5, 2.0), 2.0, std::string("a"))}),
        vars({Parameter(std::string("a"), Range(0.5, 2.0), 2.0, std::string("a"))}),
        Parameter(1.0), Parameter(0.0), std::string("a"),
        std::string("(s^2)*((s^2) + a)"));
}

TEST(FormatoLibreExpr, Class)
{
    FreeForm* plant = makeCerveraPlant();
    EXPECT_EQ(plant->type(), LtiSystem::SystemType::FreeForm);
    delete plant;
}

TEST(FormatoLibreExpr, StoredExpressionsAreVisible)
{
    FreeForm* plant = makeCerveraPlant();
    EXPECT_EQ(plant->numeratorString(), std::string("a"));
    EXPECT_EQ(plant->denominatorString(), std::string("(s^2)*((s^2) + a)"));
    delete plant;
}

TEST(FormatoLibreExpr, SymbolicExpression)
{
    FreeForm* plant = makeCerveraPlant();
    EXPECT_EQ(plant->expression(), std::string("1*(a)/((s^2)*((s^2) + a))"));
    delete plant;
}

TEST(FormatoLibreExpr, NominalEvaluation)
{
    FreeForm* plant = makeCerveraPlant();

    const Complex s(0.0, 0.1);
    const Complex expected = 2.0 / ((s * s) * (s * s + 2.0));

    const Complex value = plant->evaluate(0.1);
    EXPECT_NEAR(value.real(), expected.real(), kTolerance);
    EXPECT_NEAR(value.imag(), expected.imag(), kTolerance);
    delete plant;
}

TEST(FormatoLibreExpr, FixedDelayEvaluatesAsNegativeExponential)
{
    FreeForm plant(std::string("delayed"), vars({}), vars({}),
                        Parameter(1.0), Parameter(0.4), std::string("1"),
                        std::string("s+2"));

    const double w = 1.0;
    const Complex s(0.0, w);
    const Complex expected = std::exp(-s * 0.4) / (s + 2.0);

    const Complex value = plant.evaluate(w);
    EXPECT_NEAR(value.real(), expected.real(), kTolerance);
    EXPECT_NEAR(value.imag(), expected.imag(), kTolerance);

    EXPECT_TRUE(endsWith(plant.expression(), " * e^(-s*0.4)"));
}

TEST(FormatoLibreExpr, CloneKeepsTheDenominator)
{
    FreeForm* plant = makeCerveraPlant();
    const std::unique_ptr<LtiSystem> copia = plant->clone();
    ASSERT_NE(copia, nullptr);

    delete plant;

    EXPECT_EQ(copia->numeratorString(), std::string("a"));
    EXPECT_EQ(copia->denominatorString(), std::string("(s^2)*((s^2) + a)"));
    EXPECT_EQ(copia->expression(), std::string("1*(a)/((s^2)*((s^2) + a))"));
}

TEST(SystemOwnership, CloneAndDestroyBothOwners)
{
    ZeroPoleGain* plant = makePlanta1();
    const std::unique_ptr<LtiSystem> copia = plant->clone();

    delete plant;

    const Complex s(0.0, 0.1);
    const Complex expected = 1.0 / ((s + 5.0) * (s + 30.0));
    const Complex value = copia->evaluate(0.1);
    EXPECT_NEAR(value.real(), expected.real(), kTolerance);
    EXPECT_NEAR(value.imag(), expected.imag(), kTolerance);
}

TEST(SystemInvoke, NullDelayBecomesZeroConstant)
{
    ZeroPoleGain proto(std::string("p"), {}, {}, Parameter(1.0), Parameter(0.0));

    const std::unique_ptr<LtiSystem> built = proto.create(std::string("built"), vars({}),
                                  vars({Parameter(5.0)}), Parameter(2.0));
    ASSERT_NE(built, nullptr);
    EXPECT_FALSE(built->delay().isUncertain());
    EXPECT_DOUBLE_EQ(built->delay().nominal(), 0.0);

    const Complex s(0.0, 1.0);
    const Complex expected = 2.0 / (s + 5.0);
    const Complex value = built->evaluate(1.0);
    EXPECT_NEAR(value.real(), expected.real(), kTolerance);
    EXPECT_NEAR(value.imag(), expected.imag(), kTolerance);
}

}

TEST(TimeConstantGainValidation, AZeroCornerIsRefusedAtConstruction)
{
    std::vector<Parameter> numerator{Parameter(1.0)};

    std::vector<Parameter> zeroConstant{Parameter(0.0), Parameter(1.0)};
    EXPECT_THROW(TimeConstantGain(std::string("P"), numerator, zeroConstant,
                                  Parameter(1.0), Parameter(0.0)),
                 qftbx::InvalidInput);

    std::vector<Parameter> straddling{
        Parameter(std::string("a"), qftbx::Range(-1.0, 2.0), 1.0)};
    EXPECT_THROW(TimeConstantGain(std::string("P"), numerator, straddling,
                                  Parameter(1.0), Parameter(0.0)),
                 qftbx::InvalidInput);

    std::vector<Parameter> negative{
        Parameter(std::string("a"), qftbx::Range(-3.0, -1.0), -2.0)};
    EXPECT_NO_THROW(TimeConstantGain(std::string("P"), numerator, negative,
                                     Parameter(1.0), Parameter(0.0)));
}

TEST(FormatoLibreExpr, AMiscountedValueVectorIsRefused)
{
    std::unique_ptr<FreeForm> plant(makeCerveraPlant());

    const std::vector<double> tooFew;
    EXPECT_THROW(plant->valueAt(1.0, tooFew, tooFew, 1.0, 0.0),
                 qftbx::InvalidInput);
}
