// Characterisation tests for the Sistema hierarchy: expression generation
// (getExpr) and nominal evaluation through muParserX (getPunto). They pin the
// CURRENT behaviour, including known defects marked "// BUG:"; fixing those
// must flip the expectation in a dedicated commit.
//
// Reminder of the plant forms:
//   k_ganancia     P(s) = k * prod(s + z) / prod(s + p)
//   k_no_ganancia  P(s) = k * prod(s/z + 1) / prod(s/p + 1)
//   cof_polinomios P(s) = k * (a0*s^(n-1)+...) / (b0*s^(m-1)+...)
//   formato_libre  P(s) = k * N(s)/D(s), N and D free text

#include <gtest/gtest.h>

#include <complex>

#include <QPointF>
#include <QString>
#include <QVector>

#include "Modelo/EstructuraSistema/cpolinomios.h"
#include "Modelo/EstructuraSistema/formatolibre.h"
#include "Modelo/EstructuraSistema/kganancia.h"
#include "Modelo/EstructuraSistema/knganancia.h"
#include "Modelo/EstructuraSistema/sistema.h"
#include "Modelo/EstructurasDatos/var.h"

namespace {

using Complex = std::complex<qreal>;

constexpr qreal kTolerance = 1e-9;

QVector<Var*>* vars(std::initializer_list<Var*> list)
{
    auto* result = new QVector<Var*>();
    for (Var* var : list) {
        result->append(var);
    }
    return result;
}

// ---------------------------------------------------------------------------
// KGanancia: zero/pole form, mirrors tests/data/planta1.qft
// ---------------------------------------------------------------------------

KGanancia* makePlanta1()
{
    // planta1.qft: empty numerator, denominator {a in [1,5] nom 5,
    // b in [20,30] nom 30}, k = "kv" variable in [1,10] nom 1, ret = 0.
    return new KGanancia(
        QStringLiteral("aa"), vars({}),
        vars({new Var(QStringLiteral("a"), QPointF(1.0, 5.0), 5.0, QStringLiteral("a")),
              new Var(QStringLiteral("b"), QPointF(20.0, 30.0), 30.0, QStringLiteral("b"))}),
        new Var(QStringLiteral("kv"), QPointF(1.0, 10.0), 1.0, QStringLiteral("kv")),
        new Var(0.0));
}

TEST(KGananciaExpr, Class)
{
    KGanancia* planta = makePlanta1();
    EXPECT_EQ(planta->getClass(), Sistema::k_ganancia);
    delete planta;
}

TEST(KGananciaExpr, NumericExpressionKeepsVariableNames)
{
    KGanancia* planta = makePlanta1();
    EXPECT_EQ(planta->getExpr(0.1),
              QStringLiteral("kv*(1) / (((0.1*i) + a) *((0.1*i) + b))"));
    delete planta;
}

TEST(KGananciaExpr, SymbolicExpressionOmitsZeroFixedDelay)
{
    // Fixed in the delay rework: a zero fixed delay is not emitted (it used
    // to append "* e^(s*0)" unconditionally, and with the wrong sign).
    KGanancia* planta = makePlanta1();
    EXPECT_EQ(planta->getExpr(), QStringLiteral("kv*(1) / ((s + a) *(s + b))"));
    delete planta;
}

TEST(KGananciaExpr, FixedDelayEvaluatesAsNegativeExponential)
{
    // P(s) = 1/(s+5) with a pure delay of 0.5s: P(jw)*e^(-j*w*0.5).
    KGanancia planta(QStringLiteral("delayed"), vars({}), vars({new Var(5.0)}),
                     new Var(1.0), new Var(0.5));

    const qreal w = 2.0;
    const Complex s(0.0, w);
    const Complex expected = std::exp(-s * 0.5) / (s + 5.0);

    const Complex value = planta.getPunto(w);
    EXPECT_NEAR(value.real(), expected.real(), kTolerance);
    EXPECT_NEAR(value.imag(), expected.imag(), kTolerance);

    EXPECT_TRUE(planta.getExpr().endsWith(QStringLiteral(" * e^(-s*0.5)")));
}

TEST(KGananciaExpr, VariableDelayWithZeroNominalStaysInExpression)
{
    // An uncertain delay must stay in the expression by name even when its
    // nominal is 0, so the template sweep can drive it (it used to vanish).
    KGanancia planta(
        QStringLiteral("delayed"), vars({}), vars({new Var(5.0)}), new Var(1.0),
        new Var(QStringLiteral("tau"), QPointF(0.0, 0.5), 0.0, QStringLiteral("tau")));

    EXPECT_TRUE(planta.getExpr(0.1).endsWith(QStringLiteral("* e^(-i*0.1*tau)")));
    EXPECT_TRUE(planta.getExpr().endsWith(QStringLiteral(" * e^(-s*tau)")));
}

TEST(KGananciaExpr, NominalEvaluation)
{
    KGanancia* planta = makePlanta1();

    // Nominals kv=1, a=5, b=30 at s = 0.1j: 1/((s+5)(s+30)).
    const Complex s(0.0, 0.1);
    const Complex expected = 1.0 / ((s + 5.0) * (s + 30.0));

    const Complex value = planta->getPunto(0.1);
    EXPECT_NEAR(value.real(), expected.real(), kTolerance);
    EXPECT_NEAR(value.imag(), expected.imag(), kTolerance);
    delete planta;
}

TEST(KGananciaExpr, CloneIsDeep)
{
    KGanancia* planta = makePlanta1();
    Sistema* copy = planta->clone();
    ASSERT_NE(copy, nullptr);

    EXPECT_EQ(copy->getClass(), Sistema::k_ganancia);
    EXPECT_EQ(copy->getExpr(), planta->getExpr());
    EXPECT_NE(copy->getK(), planta->getK());
    EXPECT_NE(copy->getNumerador(), planta->getNumerador());
    ASSERT_EQ(copy->getDenominador()->size(), 2);
    EXPECT_NE(copy->getDenominador()->at(0), planta->getDenominador()->at(0));
    EXPECT_EQ(copy->getDenominador()->at(0)->getNombre(), QStringLiteral("a"));

    delete copy;
    delete planta;
}

// ---------------------------------------------------------------------------
// KNGanancia: time-constant form
// ---------------------------------------------------------------------------

KNGanancia* makeTimeConstantPlant()
{
    // P(s) = 5 / ((s/10 + 1)(s/20 + 1)), all values fixed.
    return new KNGanancia(QStringLiteral("tc"), vars({}),
                          vars({new Var(10.0), new Var(20.0)}), new Var(5.0),
                          new Var(0.0));
}

TEST(KNGananciaExpr, Class)
{
    KNGanancia* planta = makeTimeConstantPlant();
    EXPECT_EQ(planta->getClass(), Sistema::k_no_ganancia);
    delete planta;
}

TEST(KNGananciaExpr, NominalEvaluationMatchesTimeConstantForm)
{
    KNGanancia* planta = makeTimeConstantPlant();

    const Complex s(0.0, 1.0); // w = 1
    const Complex expected = 5.0 / ((s / 10.0 + 1.0) * (s / 20.0 + 1.0));

    const Complex value = planta->getPunto(1.0);
    EXPECT_NEAR(value.real(), expected.real(), kTolerance);
    EXPECT_NEAR(value.imag(), expected.imag(), kTolerance);
    delete planta;
}

TEST(KNGananciaExpr, PuntoDenoSkipsFirstPole)
{
    // BUG: getPuntoDeno loops from i = 1 and then repeats the last element,
    // so with poles {10, 20} it evaluates (s/20+1)(s/20+1) and never sees
    // the first pole. The numeric getExpr(w) route uses the correct loop:
    // the same plant yields different results depending on the code path.
    KNGanancia* planta = makeTimeConstantPlant();

    QVector<qreal> poles{10.0, 20.0};
    const Complex s(0.0, 1.0);
    const Complex buggy = (s / 20.0 + 1.0) * (s / 20.0 + 1.0);

    const Complex value = planta->getPuntoDeno(&poles, 1.0);
    EXPECT_NEAR(value.real(), buggy.real(), kTolerance);
    EXPECT_NEAR(value.imag(), buggy.imag(), kTolerance);
    delete planta;
}

// ---------------------------------------------------------------------------
// CPolinomios: polynomial-coefficient form
// ---------------------------------------------------------------------------

CPolinomios* makePolynomialPlant()
{
    // P(s) = 1 / (s^2 + 2s + 3): numerator {1}, denominator {1, 2, 3}.
    return new CPolinomios(QStringLiteral("poly"), vars({new Var(1.0)}),
                           vars({new Var(1.0), new Var(2.0), new Var(3.0)}),
                           new Var(1.0), new Var(0.0));
}

TEST(CPolinomiosExpr, Class)
{
    CPolinomios* planta = makePolynomialPlant();
    EXPECT_EQ(planta->getClass(), Sistema::cof_polinomios);
    delete planta;
}

TEST(CPolinomiosExpr, NominalEvaluationMatchesPolynomialForm)
{
    CPolinomios* planta = makePolynomialPlant();

    const Complex s(0.0, 2.0); // w = 2
    const Complex expected = 1.0 / (s * s + 2.0 * s + 3.0);

    const Complex value = planta->getPunto(2.0);
    EXPECT_NEAR(value.real(), expected.real(), kTolerance);
    EXPECT_NEAR(value.imag(), expected.imag(), kTolerance);
    delete planta;
}

TEST(CPolinomiosExpr, FixedDelayEvaluatesAsNegativeExponential)
{
    // P(s) = 1/(s^2+2s+3) with a pure delay of 0.7s. The symbolic form used
    // to concatenate the delay without a '*' (a parse error) — now fixed.
    CPolinomios planta(QStringLiteral("delayed"), vars({new Var(1.0)}),
                       vars({new Var(1.0), new Var(2.0), new Var(3.0)}),
                       new Var(1.0), new Var(0.7));

    const qreal w = 2.0;
    const Complex s(0.0, w);
    const Complex expected = std::exp(-s * 0.7) / (s * s + 2.0 * s + 3.0);

    const Complex value = planta.getPunto(w);
    EXPECT_NEAR(value.real(), expected.real(), kTolerance);
    EXPECT_NEAR(value.imag(), expected.imag(), kTolerance);

    EXPECT_TRUE(planta.getExpr().endsWith(QStringLiteral(" * e^(-s*0.7)")));
}

TEST(KNGananciaExpr, FixedDelayEvaluatesAsNegativeExponential)
{
    // P(s) = 5/(s/10+1) with a pure delay of 0.3s.
    KNGanancia planta(QStringLiteral("delayed"), vars({}), vars({new Var(10.0)}),
                      new Var(5.0), new Var(0.3));

    const qreal w = 1.0;
    const Complex s(0.0, w);
    const Complex expected = 5.0 * std::exp(-s * 0.3) / (s / 10.0 + 1.0);

    const Complex value = planta.getPunto(w);
    EXPECT_NEAR(value.real(), expected.real(), kTolerance);
    EXPECT_NEAR(value.imag(), expected.imag(), kTolerance);
}

// ---------------------------------------------------------------------------
// FormatoLibre: free-format expression, mirrors tests/data/cervera.qft
// ---------------------------------------------------------------------------

FormatoLibre* makeCerveraPlant()
{
    // cervera.qft: P(s) = a / (s^2 (s^2 + a)), a in [0.5,2] nominal 2,
    // k = 1 fixed, ret = 0. Numerator and denominator each hold their own
    // Var "a" (two distinct objects with identical content).
    return new FormatoLibre(
        QStringLiteral("cervera"),
        vars({new Var(QStringLiteral("a"), QPointF(0.5, 2.0), 2.0, QStringLiteral("a"))}),
        vars({new Var(QStringLiteral("a"), QPointF(0.5, 2.0), 2.0, QStringLiteral("a"))}),
        new Var(1.0), new Var(0.0), QStringLiteral("a"),
        QStringLiteral("(s^2)*((s^2) + a)"));
}

TEST(FormatoLibreExpr, Class)
{
    FormatoLibre* planta = makeCerveraPlant();
    EXPECT_EQ(planta->getClass(), Sistema::formato_libre);
    delete planta;
}

TEST(FormatoLibreExpr, StoredExpressionsAreVisible)
{
    FormatoLibre* planta = makeCerveraPlant();
    EXPECT_EQ(planta->getNumeradorString(), QStringLiteral("a"));
    EXPECT_EQ(planta->getDenominadorString(), QStringLiteral("(s^2)*((s^2) + a)"));
    delete planta;
}

TEST(FormatoLibreExpr, SymbolicExpression)
{
    FormatoLibre* planta = makeCerveraPlant();
    EXPECT_EQ(planta->getExpr(), QStringLiteral("1*(a)/((s^2)*((s^2) + a))"));
    delete planta;
}

TEST(FormatoLibreExpr, NumericExpressionSubstitutesS)
{
    FormatoLibre* planta = makeCerveraPlant();
    EXPECT_EQ(planta->getExpr(0.1),
              QStringLiteral("1*(a)/(((0.1*i)^2)*(((0.1*i)^2) + a))"));
    delete planta;
}

TEST(FormatoLibreExpr, NominalEvaluation)
{
    FormatoLibre* planta = makeCerveraPlant();

    // a = 2 at s = 0.1j: 2 / (s^2 (s^2 + 2)) = 2 / (-0.01 * 1.99).
    const Complex s(0.0, 0.1);
    const Complex expected = 2.0 / ((s * s) * (s * s + 2.0));

    const Complex value = planta->getPunto(0.1);
    EXPECT_NEAR(value.real(), expected.real(), kTolerance);
    EXPECT_NEAR(value.imag(), expected.imag(), kTolerance);
    delete planta;
}

TEST(FormatoLibreExpr, FixedDelayEvaluatesAsNegativeExponential)
{
    // P(s) = 1/(s+2) with a pure delay of 0.4s. The symbolic form used to
    // ignore the delay entirely — now both forms emit e^(-s*tau).
    FormatoLibre planta(QStringLiteral("delayed"), vars({}), vars({}),
                        new Var(1.0), new Var(0.4), QStringLiteral("1"),
                        QStringLiteral("s+2"));

    const qreal w = 1.0;
    const Complex s(0.0, w);
    const Complex expected = std::exp(-s * 0.4) / (s + 2.0);

    const Complex value = planta.getPunto(w);
    EXPECT_NEAR(value.real(), expected.real(), kTolerance);
    EXPECT_NEAR(value.imag(), expected.imag(), kTolerance);

    EXPECT_TRUE(planta.getExpr().endsWith(QStringLiteral(" * e^(-s*0.4)")));
}

TEST(FormatoLibreExpr, GetPuntoWithExplicitValuesIsAnUnimplementedStub)
{
    // BUG: FormatoLibre::getPunto(nume, deno, k, ret, w) is a "//TODO" stub
    // returning 0 silently; the loop-shaping algorithms do call it.
    FormatoLibre* planta = makeCerveraPlant();

    QVector<qreal> nume{2.0};
    QVector<qreal> deno{2.0};
    const Complex value = planta->getPunto(&nume, &deno, 1.0, 0.0, 0.1);
    EXPECT_EQ(value, Complex(0.0, 0.0));
    delete planta;
}

} // namespace
