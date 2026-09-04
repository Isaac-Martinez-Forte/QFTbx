// Characterisation tests for the LtiSystem hierarchy: expression generation
// (expression) and nominal evaluation through muParserX (evaluate). They pin the
// CURRENT behaviour, including known defects marked "// BUG:"; fixing those
// must flip the expectation in a dedicated commit.
//
// Reminder of the plant forms:
//   SystemType::ZeroPoleGain     P(s) = k * prod(s + z) / prod(s + p)
//   SystemType::TimeConstantGain  P(s) = k * prod(s/z + 1) / prod(s/p + 1)
//   SystemType::PolynomialForm P(s) = k * (a0*s^(n-1)+...) / (b0*s^(m-1)+...)
//   SystemType::FreeForm  P(s) = k * N(s)/D(s), N and D free text

#include <gtest/gtest.h>

#include <string>

#include <complex>
#include <vector>

#include "src/core/point.h"
#include "src/core/range.h"

#include "src/core/system/polynomial_form.h"
#include "src/core/system/free_form.h"
#include "src/core/system/zero_pole_gain.h"
#include "src/core/system/time_constant_gain.h"
#include "src/core/system/lti_system.h"
#include "src/core/system/parameter.h"
#include "src/core/exception.h"

namespace {

//std::string has no endsWith until C++20.
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

// ---------------------------------------------------------------------------
// ZeroPoleGain: zero/pole form, mirrors tests/data/planta1.qft
// ---------------------------------------------------------------------------

ZeroPoleGain* makePlanta1()
{
    // planta1.qft: empty numerator, denominator {a in [1,5] nom 5,
    // b in [20,30] nom 30}, k = "kv" variable in [1,10] nom 1, ret = 0.
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

TEST(kGainExpr, NumericExpressionKeepsVariableNames)
{
    ZeroPoleGain* plant = makePlanta1();
    EXPECT_EQ(plant->expression(0.1),
              std::string("kv*(1) / (((0.1*i) + a) *((0.1*i) + b))"));
    delete plant;
}

TEST(kGainExpr, SymbolicExpressionOmitsZeroFixedDelay)
{
    // Fixed in the delay rework: a zero fixed delay is not emitted (it used
    // to append "* e^(s*0)" unconditionally, and with the wrong sign).
    ZeroPoleGain* plant = makePlanta1();
    EXPECT_EQ(plant->expression(), std::string("kv*(1) / ((s + a) *(s + b))"));
    delete plant;
}

TEST(kGainExpr, FixedDelayEvaluatesAsNegativeExponential)
{
    // P(s) = 1/(s+5) with a pure delay of 0.5s: P(jw)*e^(-j*w*0.5).
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
    // An uncertain delay must stay in the expression by name even when its
    // nominal is 0, so the template sweep can drive it (it used to vanish).
    ZeroPoleGain plant(
        std::string("delayed"), vars({}), vars({Parameter(5.0)}), Parameter(1.0),
        Parameter(std::string("tau"), Range(0.0, 0.5), 0.0, std::string("tau")));

    EXPECT_TRUE(endsWith(plant.expression(0.1), "* e^(-i*0.1*tau)"));
    EXPECT_TRUE(endsWith(plant.expression(), " * e^(-s*tau)"));
}

TEST(kGainExpr, NominalEvaluation)
{
    ZeroPoleGain* plant = makePlanta1();

    // Nominals kv=1, a=5, b=30 at s = 0.1j: 1/((s+5)(s+30)).
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

// ---------------------------------------------------------------------------
// TimeConstantGain: time-constant form
// ---------------------------------------------------------------------------

TimeConstantGain* makeTimeConstantPlant()
{
    // P(s) = 5 / ((s/10 + 1)(s/20 + 1)), all values fixed.
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

    const Complex s(0.0, 1.0); // w = 1
    const Complex expected = 5.0 / ((s / 10.0 + 1.0) * (s / 20.0 + 1.0));

    const Complex value = plant->evaluate(1.0);
    EXPECT_NEAR(value.real(), expected.real(), kTolerance);
    EXPECT_NEAR(value.imag(), expected.imag(), kTolerance);
    delete plant;
}

TEST(kNumeratorGainExpr, PointDenominatorUsesAllPoles)
{
    // Fixed: evaluateDenominator used to loop from i = 1 and repeat the last
    // element, skipping the first pole (same off-by-one in the all-numeric
    // expression route). With poles {10, 20} it must be (s/10+1)(s/20+1).
    TimeConstantGain* plant = makeTimeConstantPlant();

    std::vector<double> poles{10.0, 20.0};
    const Complex s(0.0, 1.0);
    const Complex expected = (s / 10.0 + 1.0) * (s / 20.0 + 1.0);

    const Complex value = plant->evaluateDenominator(&poles, 1.0);
    EXPECT_NEAR(value.real(), expected.real(), kTolerance);
    EXPECT_NEAR(value.imag(), expected.imag(), kTolerance);
    delete plant;
}

TEST(kNumeratorGainExpr, ExplicitValuesRouteMatchesNominalRoute)
{
    // The all-numeric expression route (used by loop shaping) must agree with
    // the nominal evaluation for the same values.
    TimeConstantGain* plant = makeTimeConstantPlant();

    std::vector<double> nume;
    std::vector<double> deno{10.0, 20.0};
    const Complex viaValues = plant->evaluate(&nume, &deno, 5.0, 0.0, 1.0);
    const Complex viaNominals = plant->evaluate(1.0);

    EXPECT_NEAR(viaValues.real(), viaNominals.real(), kTolerance);
    EXPECT_NEAR(viaValues.imag(), viaNominals.imag(), kTolerance);
    delete plant;
}

TEST(kNumeratorGainExpr, VariableGainUsesItsRealName)
{
    // Fixed: the expression emitted the hardcoded identifier "kv" for a
    // variable gain; with any other name muParserX auto-created kv = 0 and
    // the whole plant silently evaluated to zero.
    TimeConstantGain plant(std::string("named"), vars({}), vars({Parameter(10.0)}),
                      Parameter(std::string("K1"), Range(1.0, 10.0), 5.0,
                              std::string("K1")),
                      Parameter(0.0));

    EXPECT_TRUE(plant.expression(1.0).rfind("K1*(", 0) == 0);
    EXPECT_TRUE(plant.expression().rfind("K1*(", 0) == 0);

    const Complex s(0.0, 1.0);
    const Complex expected = 5.0 / (s / 10.0 + 1.0);
    const Complex value = plant.evaluate(1.0);
    EXPECT_NEAR(value.real(), expected.real(), kTolerance);
    EXPECT_NEAR(value.imag(), expected.imag(), kTolerance);
}

// ---------------------------------------------------------------------------
// PolynomialForm: polynomial-coefficient form
// ---------------------------------------------------------------------------

PolynomialForm* makePolynomialPlant()
{
    // P(s) = 1 / (s^2 + 2s + 3): numerator {1}, denominator {1, 2, 3}.
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

    const Complex s(0.0, 2.0); // w = 2
    const Complex expected = 1.0 / (s * s + 2.0 * s + 3.0);

    const Complex value = plant->evaluate(2.0);
    EXPECT_NEAR(value.real(), expected.real(), kTolerance);
    EXPECT_NEAR(value.imag(), expected.imag(), kTolerance);
    delete plant;
}

TEST(CPolinomiosExpr, VariableGainUsesItsRealName)
{
    // Fixed: same hardcoded "kv" as TimeConstantGain.
    PolynomialForm plant(std::string("named"), vars({Parameter(1.0)}),
                       vars({Parameter(1.0), Parameter(2.0)}),
                       Parameter(std::string("K1"), Range(1.0, 10.0), 2.0,
                               std::string("K1")),
                       Parameter(0.0));

    EXPECT_TRUE(plant.expression(1.0).rfind("(K1*(", 0) == 0);

    const Complex s(0.0, 1.0);
    const Complex expected = 2.0 / (s + 2.0);
    const Complex value = plant.evaluate(1.0);
    EXPECT_NEAR(value.real(), expected.real(), kTolerance);
    EXPECT_NEAR(value.imag(), expected.imag(), kTolerance);
}

TEST(CPolinomiosExpr, FixedDelayEvaluatesAsNegativeExponential)
{
    // P(s) = 1/(s^2+2s+3) with a pure delay of 0.7s. The symbolic form used
    // to concatenate the delay without a '*' (a parse error) — now fixed.
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
    // P(s) = 5/(s/10+1) with a pure delay of 0.3s.
    TimeConstantGain plant(std::string("delayed"), vars({}), vars({Parameter(10.0)}),
                      Parameter(5.0), Parameter(0.3));

    const double w = 1.0;
    const Complex s(0.0, w);
    const Complex expected = 5.0 * std::exp(-s * 0.3) / (s / 10.0 + 1.0);

    const Complex value = plant.evaluate(w);
    EXPECT_NEAR(value.real(), expected.real(), kTolerance);
    EXPECT_NEAR(value.imag(), expected.imag(), kTolerance);
}

// ---------------------------------------------------------------------------
// FreeForm: free-format expression, mirrors tests/data/cervera.qft
// ---------------------------------------------------------------------------

FreeForm* makeCerveraPlant()
{
    // cervera.qft: P(s) = a / (s^2 (s^2 + a)), a in [0.5,2] nominal 2,
    // k = 1 fixed, ret = 0. Numerator and denominator each hold their own
    // Parameter "a" (two distinct objects with identical content).
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

TEST(FormatoLibreExpr, NumericExpressionSubstitutesS)
{
    FreeForm* plant = makeCerveraPlant();
    EXPECT_EQ(plant->expression(0.1),
              std::string("1*(a)/(((0.1*i)^2)*(((0.1*i)^2) + a))"));
    delete plant;
}

TEST(FormatoLibreExpr, NumericExpressionOnlyReplacesTheLaplaceVariable)
{
    //The historical substring replace mutilated "sin", "sqrt" and any
    //parameter whose name contains an 's'.
    FreeForm* plant = new FreeForm(
        std::string("tokens"),
        vars({Parameter(std::string("desp"), Range(0.5, 2.0), 1.0, std::string("desp"))}),
        vars({}),
        Parameter(1.0), Parameter(0.0),
        std::string("sin(s) + sqrt(desp) + s"), std::string("1"));

    EXPECT_EQ(plant->expression(2.0),
              std::string("1*(sin((2*i)) + sqrt(desp) + (2*i))/(1)"));
    delete plant;
}

TEST(FormatoLibreExpr, ExplicitValueEvaluationThrows)
{
    //The historical stubs returned 0 silently.
    FreeForm* plant = makeCerveraPlant();
    std::vector<double> values{1.0};

    EXPECT_THROW(plant->evaluate(&values, &values, 1.0, 0.0, 1.0),
                 qftbx::ComputationError);
    EXPECT_THROW(plant->evaluateNumerator(&values, 1.0),
                 qftbx::ComputationError);
    EXPECT_THROW(plant->evaluateDenominator(&values, 1.0),
                 qftbx::ComputationError);
    delete plant;
}

TEST(FormatoLibreExpr, NominalEvaluation)
{
    FreeForm* plant = makeCerveraPlant();

    // a = 2 at s = 0.1j: 2 / (s^2 (s^2 + 2)) = 2 / (-0.01 * 1.99).
    const Complex s(0.0, 0.1);
    const Complex expected = 2.0 / ((s * s) * (s * s + 2.0));

    const Complex value = plant->evaluate(0.1);
    EXPECT_NEAR(value.real(), expected.real(), kTolerance);
    EXPECT_NEAR(value.imag(), expected.imag(), kTolerance);
    delete plant;
}

TEST(FormatoLibreExpr, FixedDelayEvaluatesAsNegativeExponential)
{
    // P(s) = 1/(s+2) with a pure delay of 0.4s. The symbolic form used to
    // ignore the delay entirely — now both forms emit e^(-s*tau).
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
    // clone() must produce an independent, complete deep copy (N/D, not
    // N/N); both objects own their data and can be destroyed independently.
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
    // The plant owns its Vars and vectors; clone() deep-copies them, so
    // destroying original and clone in any order must be safe (checked for
    // leaks and double frees under ASan builds).
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
    // Fixed: create() declares ret = NULL as default, but ZeroPoleGain's guard
    // was `ret = NULL ? ... : ret` (assignment, not comparison) and the
    // other types had no guard at all: expression dereferenced a null pointer.
    ZeroPoleGain proto(std::string("p"), {}, {}, Parameter(1.0), Parameter(0.0));

    const std::unique_ptr<LtiSystem> built = proto.create(std::string("built"), vars({}),
                                  vars({Parameter(5.0)}), Parameter(2.0));
    ASSERT_NE(built, nullptr);
    //An unspecified delay is a zero-delay VALUE now: there is no null to
    //dereference (the bug this test pins was exactly that).
    EXPECT_FALSE(built->delay().isUncertain());
    EXPECT_DOUBLE_EQ(built->delay().nominal(), 0.0);

    const Complex s(0.0, 1.0);
    const Complex expected = 2.0 / (s + 5.0);
    const Complex value = built->evaluate(1.0);
    EXPECT_NEAR(value.real(), expected.real(), kTolerance);
    EXPECT_NEAR(value.imag(), expected.imag(), kTolerance);
}

} // namespace

TEST(TimeConstantGainValidation, AZeroCornerIsRefusedAtConstruction)
{
    // Every factor is s/z + 1, so a corner of zero divides by zero at every
    // frequency - and zero is finite, so Parameter's own check lets it through.
    // valueAt() used to acknowledge the division in a comment and nothing
    // refused it; the family refuses it now, where the system is built.
    std::vector<Parameter> numerator{Parameter(1.0)};

    std::vector<Parameter> zeroConstant{Parameter(0.0), Parameter(1.0)};
    EXPECT_THROW(TimeConstantGain(std::string("P"), numerator, zeroConstant,
                                  Parameter(1.0), Parameter(0.0)),
                 qftbx::InvalidInput);

    // An uncertain corner whose range straddles zero would meet the division
    // at some point of the template sweep.
    std::vector<Parameter> straddling{
        Parameter(std::string("a"), qftbx::Range(-1.0, 2.0), 1.0)};
    EXPECT_THROW(TimeConstantGain(std::string("P"), numerator, straddling,
                                  Parameter(1.0), Parameter(0.0)),
                 qftbx::InvalidInput);

    // And one clear of zero is fine, either side.
    std::vector<Parameter> negative{
        Parameter(std::string("a"), qftbx::Range(-3.0, -1.0), -2.0)};
    EXPECT_NO_THROW(TimeConstantGain(std::string("P"), numerator, negative,
                                     Parameter(1.0), Parameter(0.0)));
}

TEST(FormatoLibreExpr, AMiscountedValueVectorIsRefused)
{
    // valueAt() used to walk to the shorter of the parameter list and the value
    // list and say nothing, while a name given two values one line below was
    // refused. The same class of caller mistake now gets the same answer.
    std::unique_ptr<FreeForm> plant(makeCerveraPlant());

    const std::vector<double> tooFew;
    EXPECT_THROW(plant->valueAt(1.0, tooFew, tooFew, 1.0, 0.0),
                 qftbx::InvalidInput);
}
