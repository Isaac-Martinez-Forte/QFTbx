// Tests for the design-frequency machinery: Omega and the sequence and
// text helpers that feed it.

#include <gtest/gtest.h>

#include <string>

#include <vector>

#include <optional>

#include <cmath>

#include "src/core/exception.h"
#include "src/core/text_tokens.h"
#include "src/core/math/sequence_vectors.h"
#include "src/core/frequencies/omega.h"
#include "src/core/math/sequences.h"

namespace {

// ---------------------------------------------------------------------------
// Omega DTO
// ---------------------------------------------------------------------------

TEST(Omega, ConstructorStoresFieldsVerbatim)
{
    const std::vector<double> values{0.1, 5.0, 10.0, 100.0};
    Omega omega(0.1, 100.0, 4, values, Omega::Manual);

    EXPECT_DOUBLE_EQ(omega.start(), 0.1);
    EXPECT_DOUBLE_EQ(omega.end(), 100.0);
    EXPECT_EQ(omega.pointCount(), 4);
    EXPECT_EQ(omega.type(), Omega::Manual);
    EXPECT_EQ(*omega.values(), values); // the set holds them by value
}

TEST(Omega, ConstructorEnforcesTheSizeInvariant)
{
    // Hardened: nPuntos is always valores->size(); the constructor argument
    // is ignored on purpose (old files carry a desynchronised <nPuntos>).
    Omega omega(1.0, 2.0, 99, std::vector<double>{1.0, 2.0}, Omega::Manual);
    EXPECT_EQ(omega.pointCount(), 2);
}

TEST(Omega, ConstructorRejectsEmptyValues)
{
    //There is no null case left to reject: the frequencies arrive by value.
    EXPECT_THROW(Omega(0.0, 0.0, 0, std::vector<double>(), Omega::Manual),
                 qftbx::InvalidInput);
}

TEST(Omega, SetOmegaKeepsTheInvariant)
{
    // Hardened: setOmega keeps pointCount == values().size(), tolerates
    // being handed the very frequencies it holds, and rejects empty sets.
    Omega omega(1.0, 3.0, 3, std::vector<double>{1.0, 2.0, 3.0}, Omega::Manual);

    omega.setOmega(std::vector<double>{5.0, 6.0});
    EXPECT_EQ(omega.values()->size(), 2);
    EXPECT_EQ(omega.pointCount(), 2);

    omega.setOmega(*omega.values()); // self-assignment must be safe
    EXPECT_EQ(omega.pointCount(), 2);
    EXPECT_EQ(*omega.values(), std::vector<double>({5.0, 6.0}));

    EXPECT_THROW(omega.setOmega(std::vector<double>()), qftbx::InvalidInput);
    EXPECT_EQ(omega.values()->size(), 2); // unchanged after the throw
}

// ---------------------------------------------------------------------------
// tools::linspace
// ---------------------------------------------------------------------------

TEST(Linspace, TwoPointsAreExact)
{
    const std::vector<double> v = tools::linspace(1.0, 5.0, 2);
    ASSERT_EQ(v.size(), 2);
    EXPECT_DOUBLE_EQ(v.at(0), 1.0);
    EXPECT_DOUBLE_EQ(v.at(1), 5.0);
}

TEST(Linspace, InteriorPointsFollowStep)
{
    const std::vector<double> v = tools::linspace(0.0, 1.0, 5);
    ASSERT_EQ(v.size(), 5);
    for (int i = 0; i < 5; ++i) {
        EXPECT_NEAR(v.at(i), 0.25 * i, 1e-12);
    }
}

TEST(Linspace, LastElementIsExactlyTheEndpoint)
{
    // Fixed: values used to accumulate (val += h), so the endpoint could
    // drift; the canonical implementation pins it exactly, like MATLAB.
    const std::vector<double> v = tools::linspace(0.0, 0.3, 4);
    ASSERT_EQ(v.size(), 4);
    EXPECT_DOUBLE_EQ(v.back(), 0.3);
}

TEST(Linspace, SinglePointReturnsStart)
{
    // Fixed: N == 1 used to divide by zero when computing the step.
    const std::vector<double> v = tools::linspace(2.0, 7.0, 1);
    ASSERT_EQ(v.size(), 1);
    EXPECT_DOUBLE_EQ(v.at(0), 2.0);
}

TEST(Linspace, NonPositiveCountReturnsEmpty)
{
    // Documented contract: an invalid count yields an empty vector. The
    // GUI must validate the count before building an Omega (pending).
    const std::vector<double> v = tools::linspace(0.0, 1.0, 0);
    EXPECT_TRUE(v.empty());
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
    const std::vector<double> v = tools::linspace(5.0, 1.0, 3);
    ASSERT_EQ(v.size(), 3);
    EXPECT_DOUBLE_EQ(v.at(0), 5.0);
    EXPECT_DOUBLE_EQ(v.at(1), 3.0);
}

// ---------------------------------------------------------------------------
// tools::logspace
// ---------------------------------------------------------------------------

TEST(Logspace, ArgumentsAreExponents)
{
    const std::vector<double> v = tools::logspace(-1.0, 2.0, 4);
    ASSERT_EQ(v.size(), 4);
    EXPECT_NEAR(v.at(0), 0.1, 1e-12);
    EXPECT_NEAR(v.at(1), 1.0, 1e-12);
    EXPECT_NEAR(v.at(2), 10.0, 1e-12);
    EXPECT_NEAR(v.at(3), 100.0, 1e-12);
}

TEST(Logspace, MatchesTenToTheLinspace)
{
    const std::vector<double> exponents = tools::linspace(-2.0, 3.0, 7);
    const std::vector<double> v = tools::logspace(-2.0, 3.0, 7);
    ASSERT_EQ(v.size(), exponents.size());
    for (int i = 0; i < v.size(); ++i) {
        EXPECT_DOUBLE_EQ(v.at(i), std::pow(10.0, exponents.at(i)));
    }
}

// ---------------------------------------------------------------------------
// qftbx::text::reals (feeds the manual and file frequency modes and the
// XML parser)
// ---------------------------------------------------------------------------

TEST(SrToVectorReal, ParsesSpaceSeparatedValues)
{
    const std::optional<std::vector<double>> v = qftbx::text::reals(std::string("1 2.5 10"));
    ASSERT_TRUE(v.has_value());
    ASSERT_EQ(v->size(), 3);
    EXPECT_DOUBLE_EQ(v->at(0), 1.0);
    EXPECT_DOUBLE_EQ(v->at(1), 2.5);
    EXPECT_DOUBLE_EQ(v->at(2), 10.0);
}

TEST(SrToVectorReal, SkipsRepeatedSpaces)
{
    const std::optional<std::vector<double>> v = qftbx::text::reals(std::string("1   2"));
    ASSERT_TRUE(v.has_value());
    ASSERT_EQ(v->size(), 2);
}

TEST(SrToVectorReal, InvalidTokenReturnsNothing)
{
    // The sentinel used to be a null pointer, dereferenced unchecked by six
    // call sites in the XML parser and by the manual-frequency dialog. An
    // empty optional cannot be read by mistake.
    EXPECT_FALSE(qftbx::text::reals(std::string("1 x 3")).has_value());
}

TEST(SrToVectorReal, SplitsOnAnyWhitespace)
{
    // Fixed: the split used to be on single spaces only, so a frequencies
    // file with one value per line produced an unparseable token.
    const std::optional<std::vector<double>> v = qftbx::text::reals(std::string("1.0\n2.0\t3"));
    ASSERT_TRUE(v.has_value());
    ASSERT_EQ(v->size(), 3);
    EXPECT_DOUBLE_EQ(v->at(1), 2.0);
}

} // namespace
