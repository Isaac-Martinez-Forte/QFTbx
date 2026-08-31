// Characterisation tests for the design-frequency machinery as it stands
// today: the Omega DTO and the sequence/string helpers in tools that feed it.
// Behaviours marked "// BUG:" are known defects pinned on purpose; fixing one
// must flip its expectation in a dedicated commit.

#include <gtest/gtest.h>

#include <QString>
#include <QVector>
#include <cmath>

#include "src/core/exception.h"
#include "Modelo/Herramientas/tools.h"
#include "src/core/frequencies/omega.h"
#include "src/core/math/sequences.h"

namespace {

// ---------------------------------------------------------------------------
// Omega DTO
// ---------------------------------------------------------------------------

TEST(Omega, ConstructorStoresFieldsVerbatim)
{
    auto* values = new QVector<qreal>{0.1, 5.0, 10.0, 100.0};
    Omega omega(0.1, 100.0, 4, values, Omega::Manual);

    EXPECT_DOUBLE_EQ(omega.start(), 0.1);
    EXPECT_DOUBLE_EQ(omega.end(), 100.0);
    EXPECT_EQ(omega.pointCount(), 4);
    EXPECT_EQ(omega.type(), Omega::Manual);
    EXPECT_EQ(omega.values(), values); // internal pointer, no copy
}

TEST(Omega, ConstructorEnforcesTheSizeInvariant)
{
    // Hardened: nPuntos is always valores->size(); the constructor argument
    // is ignored on purpose (old files carry a desynchronised <nPuntos>).
    auto* values = new QVector<qreal>{1.0, 2.0};
    Omega omega(1.0, 2.0, 99, values, Omega::Manual);
    EXPECT_EQ(omega.pointCount(), 2);
}

TEST(Omega, ConstructorRejectsNullOrEmptyValues)
{
    EXPECT_THROW(Omega(0.0, 0.0, 0, nullptr, Omega::Manual), qftbx::InvalidInput);
    EXPECT_THROW(Omega(0.0, 0.0, 0, new QVector<qreal>(), Omega::Manual),
                 qftbx::InvalidInput);
}

TEST(Omega, SetOmegaKeepsTheInvariantAndOwnership)
{
    // Hardened: setValues deletes the previous vector, keeps
    // nPuntos == valores->size(), tolerates being handed the vector it
    // already owns, and rejects null/empty sets.
    auto* values = new QVector<qreal>{1.0, 2.0, 3.0};
    Omega omega(1.0, 3.0, 3, values, Omega::Manual);

    omega.setValues(new QVector<qreal>{5.0, 6.0});
    EXPECT_EQ(omega.values()->size(), 2);
    EXPECT_EQ(omega.pointCount(), 2);

    omega.setValues(omega.values()); // self-assignment must be safe
    EXPECT_EQ(omega.pointCount(), 2);

    EXPECT_THROW(omega.setValues(nullptr), qftbx::InvalidInput);
    EXPECT_EQ(omega.values()->size(), 2); // unchanged after the throw
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

TEST(Linspace, LastElementIsExactlyTheEndpoint)
{
    // Fixed: values used to accumulate (val += h), so the endpoint could
    // drift; the canonical implementation pins it exactly, like MATLAB.
    QVector<qreal>* v = tools::linspace(0.0, 0.3, 4);
    ASSERT_EQ(v->size(), 4);
    EXPECT_DOUBLE_EQ(v->last(), 0.3);
    delete v;
}

TEST(Linspace, SinglePointReturnsStart)
{
    // Fixed: N == 1 used to divide by zero when computing the step.
    QVector<qreal>* v = tools::linspace(2.0, 7.0, 1);
    ASSERT_EQ(v->size(), 1);
    EXPECT_DOUBLE_EQ(v->at(0), 2.0);
    delete v;
}

TEST(Linspace, NonPositiveCountReturnsEmpty)
{
    // Documented contract: an invalid count yields an empty vector. The
    // GUI must validate the count before building an Omega (pending).
    QVector<qreal>* v = tools::linspace(0.0, 1.0, 0);
    ASSERT_NE(v, nullptr);
    EXPECT_TRUE(v->isEmpty());
    delete v;
}

TEST(MathSequences, LinspaceMatchesMatlabSemantics)
{
    const std::vector<double> v = qftbx::math::linspace(0.0, 0.3, 4);
    ASSERT_EQ(v.size(), 4u);
    EXPECT_DOUBLE_EQ(v.front(), 0.0);
    EXPECT_NEAR(v[1], 0.1, 1e-15);
    EXPECT_DOUBLE_EQ(v.back(), 0.3);

    EXPECT_TRUE(qftbx::math::linspace(1.0, 2.0, 0).empty());
    EXPECT_EQ(qftbx::math::linspace(3.0, 9.0, 1), std::vector<double>{3.0});
}

TEST(MathSequences, LogspaceIsTenToTheLinspace)
{
    const std::vector<double> v = qftbx::math::logspace(-1.0, 2.0, 4);
    ASSERT_EQ(v.size(), 4u);
    EXPECT_DOUBLE_EQ(v.front(), 0.1);
    EXPECT_DOUBLE_EQ(v.back(), 100.0);
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
        EXPECT_DOUBLE_EQ(v->at(i), std::pow(10.0, exponents->at(i)));
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

TEST(SrToVectorReal, SplitsOnAnyWhitespace)
{
    // Fixed: the split used to be on single spaces only, so a frequencies
    // file with one value per line produced an unparseable token.
    QVector<qreal>* v = tools::srtovectorReal(QStringLiteral("1.0\n2.0\t3"));
    ASSERT_NE(v, nullptr);
    ASSERT_EQ(v->size(), 3);
    EXPECT_DOUBLE_EQ(v->at(1), 2.0);
    delete v;
}

} // namespace
