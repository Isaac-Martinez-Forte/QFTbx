// Characterisation tests for the design-frequency machinery as it stands
// today: the Omega DTO and the sequence/string helpers in tools that feed it.
// Behaviours marked "// BUG:" are known defects pinned on purpose; fixing one
// must flip its expectation in a dedicated commit.

#include <gtest/gtest.h>

#include <QString>
#include <QVector>

#include "Modelo/Herramientas/tools.h"
#include "Modelo/Objetos/omega.h"

namespace {

// ---------------------------------------------------------------------------
// Omega DTO
// ---------------------------------------------------------------------------

TEST(Omega, ConstructorStoresFieldsVerbatim)
{
    auto* values = new QVector<qreal>{0.1, 5.0, 10.0, 100.0};
    Omega omega(0.1, 100.0, 4, values, tools::manual);

    EXPECT_DOUBLE_EQ(omega.getInicio(), 0.1);
    EXPECT_DOUBLE_EQ(omega.getFinal(), 100.0);
    EXPECT_EQ(omega.getNPuntos(), 4);
    EXPECT_EQ(omega.getTipo(), tools::manual);
    EXPECT_EQ(omega.getValores(), values); // internal pointer, no copy
}

TEST(Omega, SetOmegaDoesNotUpdateNPuntos)
{
    // BUG: setOmega replaces the vector (leaking the old one) but leaves
    // nPuntos stale, so a .qft saved after templates/boundaries reorder the
    // frequencies carries an inconsistent <nPuntos>.
    auto* values = new QVector<qreal>{1.0, 2.0, 3.0};
    Omega omega(1.0, 3.0, 3, values, tools::manual);

    omega.setOmega(new QVector<qreal>{5.0, 6.0});
    EXPECT_EQ(omega.getValores()->size(), 2);
    EXPECT_EQ(omega.getNPuntos(), 3);
}

// ---------------------------------------------------------------------------
// tools::linspace
// ---------------------------------------------------------------------------

TEST(Linspace, TwoPointsAreExact)
{
    QVector<qreal>* v = tools::linspace(1.0, 5.0, 2);
    ASSERT_EQ(v->size(), 2);
    EXPECT_DOUBLE_EQ(v->at(0), 1.0);
    EXPECT_DOUBLE_EQ(v->at(1), 5.0);
    delete v;
}

TEST(Linspace, InteriorPointsFollowStep)
{
    QVector<qreal>* v = tools::linspace(0.0, 1.0, 5);
    ASSERT_EQ(v->size(), 5);
    for (int i = 0; i < 5; ++i) {
        EXPECT_NEAR(v->at(i), 0.25 * i, 1e-12);
    }
    delete v;
}

TEST(Linspace, LastElementIsOnlyApproximatelyTheEndpoint)
{
    // BUG: values accumulate (val += h) instead of a + i*h, so the last
    // element is only guaranteed near the endpoint, not equal to it (MATLAB
    // pins it exactly). Whether the drift shows depends on the input and
    // the compiler, so this only asserts closeness; the fix will tighten it
    // to exact equality.
    QVector<qreal>* v = tools::linspace(0.0, 0.3, 4);
    ASSERT_EQ(v->size(), 4);
    EXPECT_NEAR(v->last(), 0.3, 1e-12);
    delete v;
}

TEST(Linspace, SinglePointReturnsStart)
{
    // N == 1 divides by zero when computing the step; the value is appended
    // before the step is used, so the output happens to be right.
    QVector<qreal>* v = tools::linspace(2.0, 7.0, 1);
    ASSERT_EQ(v->size(), 1);
    EXPECT_DOUBLE_EQ(v->at(0), 2.0);
    delete v;
}

TEST(Linspace, NonPositiveCountReturnsEmpty)
{
    // BUG: an invalid count silently produces an empty vector; the GUI then
    // builds an Omega with zero frequencies without noticing.
    QVector<qreal>* v = tools::linspace(0.0, 1.0, 0);
    ASSERT_NE(v, nullptr);
    EXPECT_TRUE(v->isEmpty());
    delete v;
}

TEST(Linspace, InvertedRangeDescendsSilently)
{
    QVector<qreal>* v = tools::linspace(5.0, 1.0, 3);
    ASSERT_EQ(v->size(), 3);
    EXPECT_DOUBLE_EQ(v->at(0), 5.0);
    EXPECT_DOUBLE_EQ(v->at(1), 3.0);
    delete v;
}

// ---------------------------------------------------------------------------
// tools::logspace
// ---------------------------------------------------------------------------

TEST(Logspace, ArgumentsAreExponents)
{
    QVector<qreal>* v = tools::logspace(-1.0, 2.0, 4);
    ASSERT_EQ(v->size(), 4);
    EXPECT_NEAR(v->at(0), 0.1, 1e-12);
    EXPECT_NEAR(v->at(1), 1.0, 1e-12);
    EXPECT_NEAR(v->at(2), 10.0, 1e-12);
    EXPECT_NEAR(v->at(3), 100.0, 1e-12);
    delete v;
}

TEST(Logspace, MatchesTenToTheLinspace)
{
    QVector<qreal>* exponents = tools::linspace(-2.0, 3.0, 7);
    QVector<qreal>* v = tools::logspace(-2.0, 3.0, 7);
    ASSERT_EQ(v->size(), exponents->size());
    for (int i = 0; i < v->size(); ++i) {
        EXPECT_DOUBLE_EQ(v->at(i), qPow(10.0, exponents->at(i)));
    }
    delete exponents;
    delete v;
}

// ---------------------------------------------------------------------------
// tools::srtovectorReal (feeds the manual and file frequency modes and the
// XML parser)
// ---------------------------------------------------------------------------

TEST(SrToVectorReal, ParsesSpaceSeparatedValues)
{
    QVector<qreal>* v = tools::srtovectorReal(QStringLiteral("1 2.5 10"));
    ASSERT_NE(v, nullptr);
    ASSERT_EQ(v->size(), 3);
    EXPECT_DOUBLE_EQ(v->at(0), 1.0);
    EXPECT_DOUBLE_EQ(v->at(1), 2.5);
    EXPECT_DOUBLE_EQ(v->at(2), 10.0);
    delete v;
}

TEST(SrToVectorReal, SkipsRepeatedSpaces)
{
    QVector<qreal>* v = tools::srtovectorReal(QStringLiteral("1   2"));
    ASSERT_NE(v, nullptr);
    ASSERT_EQ(v->size(), 2);
    delete v;
}

TEST(SrToVectorReal, InvalidTokenReturnsNull)
{
    // BUG: the null sentinel is dereferenced unchecked by 6 call sites in
    // the XML parser and by the manual-frequency dialog. Will become a
    // qftbx::ParseError.
    EXPECT_EQ(tools::srtovectorReal(QStringLiteral("1 x 3")), nullptr);
}

TEST(SrToVectorReal, NewlineSeparatedValuesFail)
{
    // BUG: the split is on single spaces only, so a frequencies file with
    // one value per line produces an unparseable token.
    EXPECT_EQ(tools::srtovectorReal(QStringLiteral("1.0\n2.0")), nullptr);
}

} // namespace
