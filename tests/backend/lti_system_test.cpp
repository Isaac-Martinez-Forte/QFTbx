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

#include <complex>

#include <QPointF>
#include <QString>
#include <QVector>

#include "src/core/system/polynomial_form.h"
#include "src/core/system/free_form.h"
#include "src/core/system/zero_pole_gain.h"
#include "src/core/system/time_constant_gain.h"
#include "src/core/system/lti_system.h"
#include "src/core/system/parameter.h"

namespace {

using Complex = std::complex<qreal>;

constexpr qreal kTolerance = 1e-9;

QVector<Parameter*>* vars(std::initializer_list<Parameter*> list)
{
    auto* result = new QVector<Parameter*>();
    for (Parameter* var : list) {
        result->append(var);
    }
    return result;
}

// ---------------------------------------------------------------------------
// ZeroPoleGain: zero/pole form, mirrors tests/data/planta1.qft
// ---------------------------------------------------------------------------

ZeroPoleGain* makePlanta1()
{
    // planta1.qft: empty numerator, denominator {a in [1,5] nom 5,
    // b in [20,30] nom 30}, k = "kv" variable in [1,10] nom 1, ret = 0.
    return new ZeroPoleGain(
        QStringLiteral("aa"), vars({}),
        vars({new Parameter(QStringLiteral("a"), QPointF(1.0, 5.0), 5.0, QStringLiteral("a")),
              new Parameter(QStringLiteral("b"), QPointF(20.0, 30.0), 30.0, QStringLiteral("b"))}),
        new Parameter(QStringLiteral("kv"), QPointF(1.0, 10.0), 1.0, QStringLiteral("kv")),
        new Parameter(0.0));
}

TEST(KGananciaExpr, Class)
{
    ZeroPoleGain* planta = makePlanta1();
    EXPECT_EQ(planta->type(), LtiSystem::SystemType::ZeroPoleGain);
    delete planta;
}

TEST(KGananciaExpr, NumericExpressionKeepsVariableNames)
{
    ZeroPoleGain* planta = makePlanta1();
    EXPECT_EQ(planta->expression(0.1),
              QStringLiteral("kv*(1) / (((0.1*i) + a) *((0.1*i) + b))"));
    delete planta;
}

TEST(KGananciaExpr, SymbolicExpressionOmitsZeroFixedDelay)
{
    // Fixed in the delay rework: a zero fixed delay is not emitted (it used
    // to append "* e^(s*0)" unconditionally, and with the wrong sign).
    ZeroPoleGain* planta = makePlanta1();
    EXPECT_EQ(planta->expression(), QStringLiteral("kv*(1) / ((s + a) *(s + b))"));
    delete planta;
}

TEST(KGananciaExpr, FixedDelayEvaluatesAsNegativeExponential)
{
    // P(s) = 1/(s+5) with a pure delay of 0.5s: P(jw)*e^(-j*w*0.5).
    ZeroPoleGain planta(QStringLiteral("delayed"), vars({}), vars({new Parameter(5.0)}),
                     new Parameter(1.0), new Parameter(0.5));

    const qreal w = 2.0;
    const Complex s(0.0, w);
    const Complex expected = std::exp(-s * 0.5) / (s + 5.0);

    const Complex value = planta.evaluate(w);
    EXPECT_NEAR(value.real(), expected.real(), kTolerance);
    EXPECT_NEAR(value.imag(), expected.imag(), kTolerance);

    EXPECT_TRUE(planta.expression().endsWith(QStringLiteral(" * e^(-s*0.5)")));
}

TEST(KGananciaExpr, VariableDelayWithZeroNominalStaysInExpression)
{
    // An uncertain delay must stay in the expression by name even when its
    // nominal is 0, so the template sweep can drive it (it used to vanish).
    ZeroPoleGain planta(
        QStringLiteral("delayed"), vars({}), vars({new Parameter(5.0)}), new Parameter(1.0),
        new Parameter(QStringLiteral("tau"), QPointF(0.0, 0.5), 0.0, QStringLiteral("tau")));

    EXPECT_TRUE(planta.expression(0.1).endsWith(QStringLiteral("* e^(-i*0.1*tau)")));
    EXPECT_TRUE(planta.expression().endsWith(QStringLiteral(" * e^(-s*tau)")));
}

TEST(KGananciaExpr, NominalEvaluation)
{
    ZeroPoleGain* planta = makePlanta1();

    // Nominals kv=1, a=5, b=30 at s = 0.1j: 1/((s+5)(s+30)).
    const Complex s(0.0, 0.1);
    const Complex expected = 1.0 / ((s + 5.0) * (s + 30.0));

    const Complex value = planta->evaluate(0.1);
    EXPECT_NEAR(value.real(), expected.real(), kTolerance);
    EXPECT_NEAR(value.imag(), expected.imag(), kTolerance);
    delete planta;
}

TEST(KGananciaExpr, CloneIsDeep)
{
    ZeroPoleGain* planta = makePlanta1();
    LtiSystem* copy = planta->clone();
    ASSERT_NE(copy, nullptr);

    EXPECT_EQ(copy->type(), LtiSystem::SystemType::ZeroPoleGain);
    EXPECT_EQ(copy->expression(), planta->expression());
    EXPECT_NE(copy->gain(), planta->gain());
    EXPECT_NE(copy->numerator(), planta->numerator());
    ASSERT_EQ(copy->denominator()->size(), 2);
    EXPECT_NE(copy->denominator()->at(0), planta->denominator()->at(0));
    EXPECT_EQ(copy->denominator()->at(0)->name(), QStringLiteral("a"));

    delete copy;
    delete planta;
}

// ---------------------------------------------------------------------------
// TimeConstantGain: time-constant form
// ---------------------------------------------------------------------------

TimeConstantGain* makeTimeConstantPlant()
{
    // P(s) = 5 / ((s/10 + 1)(s/20 + 1)), all values fixed.
    return new TimeConstantGain(QStringLiteral("tc"), vars({}),
                          vars({new Parameter(10.0), new Parameter(20.0)}), new Parameter(5.0),
                          new Parameter(0.0));
}

TEST(KNGananciaExpr, Class)
{
    TimeConstantGain* planta = makeTimeConstantPlant();
    EXPECT_EQ(planta->type(), LtiSystem::SystemType::TimeConstantGain);
    delete planta;
}

TEST(KNGananciaExpr, NominalEvaluationMatchesTimeConstantForm)
{
    TimeConstantGain* planta = makeTimeConstantPlant();

    const Complex s(0.0, 1.0); // w = 1
    const Complex expected = 5.0 / ((s / 10.0 + 1.0) * (s / 20.0 + 1.0));

    const Complex value = planta->evaluate(1.0);
    EXPECT_NEAR(value.real(), expected.real(), kTolerance);
    EXPECT_NEAR(value.imag(), expected.imag(), kTolerance);
    delete planta;
}

TEST(KNGananciaExpr, PuntoDenoUsesAllPoles)
{
    // Fixed: evaluateDenominator used to loop from i = 1 and repeat the last
    // element, skipping the first pole (same off-by-one in the all-numeric
    // expression route). With poles {10, 20} it must be (s/10+1)(s/20+1).
    TimeConstantGain* planta = makeTimeConstantPlant();

    QVector<qreal> poles{10.0, 20.0};
    const Complex s(0.0, 1.0);
    const Complex expected = (s / 10.0 + 1.0) * (s / 20.0 + 1.0);

    const Complex value = planta->evaluateDenominator(&poles, 1.0);
    EXPECT_NEAR(value.real(), expected.real(), kTolerance);
    EXPECT_NEAR(value.imag(), expected.imag(), kTolerance);
    delete planta;
}

TEST(KNGananciaExpr, ExplicitValuesRouteMatchesNominalRoute)
{
    // The all-numeric expression route (used by loop shaping) must agree with
    // the nominal evaluation for the same values.
    TimeConstantGain* planta = makeTimeConstantPlant();

    QVector<qreal> nume;
    QVector<qreal> deno{10.0, 20.0};
    const Complex viaValues = planta->evaluate(&nume, &deno, 5.0, 0.0, 1.0);
    const Complex viaNominals = planta->evaluate(1.0);

    EXPECT_NEAR(viaValues.real(), viaNominals.real(), kTolerance);
    EXPECT_NEAR(viaValues.imag(), viaNominals.imag(), kTolerance);
    delete planta;
}

TEST(KNGananciaExpr, VariableGainUsesItsRealName)
{
    // Fixed: the expression emitted the hardcoded identifier "kv" for a
    // variable gain; with any other name muParserX auto-created kv = 0 and
    // the whole plant silently evaluated to zero.
    TimeConstantGain planta(QStringLiteral("named"), vars({}), vars({new Parameter(10.0)}),
                      new Parameter(QStringLiteral("K1"), QPointF(1.0, 10.0), 5.0,
                              QStringLiteral("K1")),
                      new Parameter(0.0));

    EXPECT_TRUE(planta.expression(1.0).startsWith(QStringLiteral("K1*(")));
    EXPECT_TRUE(planta.expression().startsWith(QStringLiteral("K1*(")));

    const Complex s(0.0, 1.0);
    const Complex expected = 5.0 / (s / 10.0 + 1.0);
    const Complex value = planta.evaluate(1.0);
    EXPECT_NEAR(value.real(), expected.real(), kTolerance);
    EXPECT_NEAR(value.imag(), expected.imag(), kTolerance);
}

// ---------------------------------------------------------------------------
// PolynomialForm: polynomial-coefficient form
// ---------------------------------------------------------------------------

PolynomialForm* makePolynomialPlant()
{
    // P(s) = 1 / (s^2 + 2s + 3): numerator {1}, denominator {1, 2, 3}.
    return new PolynomialForm(QStringLiteral("poly"), vars({new Parameter(1.0)}),
                           vars({new Parameter(1.0), new Parameter(2.0), new Parameter(3.0)}),
                           new Parameter(1.0), new Parameter(0.0));
}

TEST(CPolinomiosExpr, Class)
{
    PolynomialForm* planta = makePolynomialPlant();
    EXPECT_EQ(planta->type(), LtiSystem::SystemType::PolynomialForm);
    delete planta;
}

TEST(CPolinomiosExpr, NominalEvaluationMatchesPolynomialForm)
{
    PolynomialForm* planta = makePolynomialPlant();

    const Complex s(0.0, 2.0); // w = 2
    const Complex expected = 1.0 / (s * s + 2.0 * s + 3.0);

    const Complex value = planta->evaluate(2.0);
    EXPECT_NEAR(value.real(), expected.real(), kTolerance);
    EXPECT_NEAR(value.imag(), expected.imag(), kTolerance);
    delete planta;
}

TEST(CPolinomiosExpr, VariableGainUsesItsRealName)
{
    // Fixed: same hardcoded "kv" as TimeConstantGain.
    PolynomialForm planta(QStringLiteral("named"), vars({new Parameter(1.0)}),
                       vars({new Parameter(1.0), new Parameter(2.0)}),
                       new Parameter(QStringLiteral("K1"), QPointF(1.0, 10.0), 2.0,
                               QStringLiteral("K1")),
                       new Parameter(0.0));

    EXPECT_TRUE(planta.expression(1.0).startsWith(QStringLiteral("(K1*(")));

    const Complex s(0.0, 1.0);
    const Complex expected = 2.0 / (s + 2.0);
    const Complex value = planta.evaluate(1.0);
    EXPECT_NEAR(value.real(), expected.real(), kTolerance);
    EXPECT_NEAR(value.imag(), expected.imag(), kTolerance);
}

TEST(CPolinomiosExpr, FixedDelayEvaluatesAsNegativeExponential)
{
    // P(s) = 1/(s^2+2s+3) with a pure delay of 0.7s. The symbolic form used
    // to concatenate the delay without a '*' (a parse error) — now fixed.
    PolynomialForm planta(QStringLiteral("delayed"), vars({new Parameter(1.0)}),
                       vars({new Parameter(1.0), new Parameter(2.0), new Parameter(3.0)}),
                       new Parameter(1.0), new Parameter(0.7));

    const qreal w = 2.0;
    const Complex s(0.0, w);
    const Complex expected = std::exp(-s * 0.7) / (s * s + 2.0 * s + 3.0);

    const Complex value = planta.evaluate(w);
    EXPECT_NEAR(value.real(), expected.real(), kTolerance);
    EXPECT_NEAR(value.imag(), expected.imag(), kTolerance);

    EXPECT_TRUE(planta.expression().endsWith(QStringLiteral(" * e^(-s*0.7)")));
}

TEST(KNGananciaExpr, FixedDelayEvaluatesAsNegativeExponential)
{
    // P(s) = 5/(s/10+1) with a pure delay of 0.3s.
    TimeConstantGain planta(QStringLiteral("delayed"), vars({}), vars({new Parameter(10.0)}),
                      new Parameter(5.0), new Parameter(0.3));

    const qreal w = 1.0;
    const Complex s(0.0, w);
    const Complex expected = 5.0 * std::exp(-s * 0.3) / (s / 10.0 + 1.0);

    const Complex value = planta.evaluate(w);
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
        QStringLiteral("cervera"),
        vars({new Parameter(QStringLiteral("a"), QPointF(0.5, 2.0), 2.0, QStringLiteral("a"))}),
        vars({new Parameter(QStringLiteral("a"), QPointF(0.5, 2.0), 2.0, QStringLiteral("a"))}),
        new Parameter(1.0), new Parameter(0.0), QStringLiteral("a"),
        QStringLiteral("(s^2)*((s^2) + a)"));
}

TEST(FormatoLibreExpr, Class)
{
    FreeForm* planta = makeCerveraPlant();
    EXPECT_EQ(planta->type(), LtiSystem::SystemType::FreeForm);
    delete planta;
}

TEST(FormatoLibreExpr, StoredExpressionsAreVisible)
{
    FreeForm* planta = makeCerveraPlant();
    EXPECT_EQ(planta->numeratorString(), QStringLiteral("a"));
    EXPECT_EQ(planta->denominatorString(), QStringLiteral("(s^2)*((s^2) + a)"));
    delete planta;
}

TEST(FormatoLibreExpr, SymbolicExpression)
{
    FreeForm* planta = makeCerveraPlant();
    EXPECT_EQ(planta->expression(), QStringLiteral("1*(a)/((s^2)*((s^2) + a))"));
    delete planta;
}

TEST(FormatoLibreExpr, NumericExpressionSubstitutesS)
{
    FreeForm* planta = makeCerveraPlant();
    EXPECT_EQ(planta->expression(0.1),
              QStringLiteral("1*(a)/(((0.1*i)^2)*(((0.1*i)^2) + a))"));
    delete planta;
}

TEST(FormatoLibreExpr, NominalEvaluation)
{
    FreeForm* planta = makeCerveraPlant();

    // a = 2 at s = 0.1j: 2 / (s^2 (s^2 + 2)) = 2 / (-0.01 * 1.99).
    const Complex s(0.0, 0.1);
    const Complex expected = 2.0 / ((s * s) * (s * s + 2.0));

    const Complex value = planta->evaluate(0.1);
    EXPECT_NEAR(value.real(), expected.real(), kTolerance);
    EXPECT_NEAR(value.imag(), expected.imag(), kTolerance);
    delete planta;
}

TEST(FormatoLibreExpr, FixedDelayEvaluatesAsNegativeExponential)
{
    // P(s) = 1/(s+2) with a pure delay of 0.4s. The symbolic form used to
    // ignore the delay entirely — now both forms emit e^(-s*tau).
    FreeForm planta(QStringLiteral("delayed"), vars({}), vars({}),
                        new Parameter(1.0), new Parameter(0.4), QStringLiteral("1"),
                        QStringLiteral("s+2"));

    const qreal w = 1.0;
    const Complex s(0.0, w);
    const Complex expected = std::exp(-s * 0.4) / (s + 2.0);

    const Complex value = planta.evaluate(w);
    EXPECT_NEAR(value.real(), expected.real(), kTolerance);
    EXPECT_NEAR(value.imag(), expected.imag(), kTolerance);

    EXPECT_TRUE(planta.expression().endsWith(QStringLiteral(" * e^(-s*0.4)")));
}

TEST(FormatoLibreExpr, CloneKeepsTheDenominator)
{
    // clone() must produce an independent, complete deep copy (N/D, not
    // N/N); both objects own their data and can be destroyed independently.
    FreeForm* planta = makeCerveraPlant();
    LtiSystem* copia = planta->clone();
    ASSERT_NE(copia, nullptr);

    delete planta;

    EXPECT_EQ(copia->numeratorString(), QStringLiteral("a"));
    EXPECT_EQ(copia->denominatorString(), QStringLiteral("(s^2)*((s^2) + a)"));
    EXPECT_EQ(copia->expression(), QStringLiteral("1*(a)/((s^2)*((s^2) + a))"));
    delete copia;
}

TEST(SistemaOwnership, CloneAndDestroyBothOwners)
{
    // The plant owns its Vars and vectors; clone() deep-copies them, so
    // destroying original and clone in any order must be safe (checked for
    // leaks and double frees under ASan builds).
    ZeroPoleGain* planta = makePlanta1();
    LtiSystem* copia = planta->clone();

    delete planta;

    const Complex s(0.0, 0.1);
    const Complex expected = 1.0 / ((s + 5.0) * (s + 30.0));
    const Complex value = copia->evaluate(0.1);
    EXPECT_NEAR(value.real(), expected.real(), kTolerance);
    EXPECT_NEAR(value.imag(), expected.imag(), kTolerance);
    delete copia;
}

TEST(SistemaInvoke, NullDelayBecomesZeroConstant)
{
    // Fixed: create() declares ret = NULL as default, but ZeroPoleGain's guard
    // was `ret = NULL ? ... : ret` (assignment, not comparison) and the
    // other types had no guard at all: expression dereferenced a null pointer.
    ZeroPoleGain proto(QStringLiteral("p"), new QVector<Parameter*>(), new QVector<Parameter*>(),
                    new Parameter(1.0), new Parameter(0.0));

    LtiSystem* built = proto.create(QStringLiteral("built"), vars({}),
                                  vars({new Parameter(5.0)}), new Parameter(2.0));
    ASSERT_NE(built, nullptr);
    ASSERT_NE(built->delay(), nullptr);
    EXPECT_FALSE(built->delay()->isUncertain());
    EXPECT_DOUBLE_EQ(built->delay()->nominal(), 0.0);

    const Complex s(0.0, 1.0);
    const Complex expected = 2.0 / (s + 5.0);
    const Complex value = built->evaluate(1.0);
    EXPECT_NEAR(value.real(), expected.real(), kTolerance);
    EXPECT_NEAR(value.imag(), expected.imag(), kTolerance);
    delete built;
}

TEST(FormatoLibreExpr, GetPuntoWithExplicitValuesIsAnUnimplementedStub)
{
    // BUG: FreeForm::evaluate(nume, deno, k, ret, w) is a "//TODO" stub
    // returning 0 silently; the loop-shaping algorithms do call it.
    FreeForm* planta = makeCerveraPlant();

    QVector<qreal> nume{2.0};
    QVector<qreal> deno{2.0};
    const Complex value = planta->evaluate(&nume, &deno, 1.0, 0.0, 0.1);
    EXPECT_EQ(value, Complex(0.0, 0.0));
    delete planta;
}

} // namespace
