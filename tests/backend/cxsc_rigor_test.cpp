// Rigour safety net over the vendored C-XSC library (phase 8c): every
// interval operation the loop-shaping stack relies on must ENCLOSE the true
// real result, which holds only while the library's directed rounding
// survives the compiler flags (the -O3 inliner was observed reordering the
// ldmxcsr assembly of interval.inl/r_ari.h until -frounding-math was added
// to the backend and test targets). These tests run under the exact flags
// of the production arithmetic and fail if a build change ever breaks the
// rounding again.
//
// The checks are containment properties on values with no exact binary
// representation (1/10, 1/3, pi...): a naive round-to-nearest evaluation
// produces a degenerate interval that misses the true value on one side,
// so a regression flips them deterministically.

#include <gtest/gtest.h>

//A failed comparison of C-XSC values must report, not crash: see the header.
#include "tests/backend/cxsc_printing.h"

#include <cmath>

#include <interval.hpp>
#include <imath.hpp>
#include <cinterval.hpp>


namespace {

using cxsc::interval;
using cxsc::real;

//True value strictly inside the interval evaluation of an expression that
//cannot be represented exactly.
TEST(CxscRigor, DivisionEnclosesTheTrueQuotient)
{
    const interval tenth = interval(1.0) / interval(10.0);

    //1/10 is not a binary float: a rigorous enclosure is a non-degenerate
    //interval whose bounds straddle the true value.
    EXPECT_LT(cxsc::Inf(tenth), cxsc::Sup(tenth));

    //0.1 rounded to nearest lies inside.
    EXPECT_LE(cxsc::Inf(tenth), 0.1);
    EXPECT_GE(cxsc::Sup(tenth), 0.1);

    //Multiplying back must enclose 1 exactly.
    const interval one = tenth * interval(10.0);
    EXPECT_LE(cxsc::Inf(one), 1.0);
    EXPECT_GE(cxsc::Sup(one), 1.0);
}

TEST(CxscRigor, AdditionRoundsOutwards)
{
    //x + y with a result needing more precision than a double: the sum
    //1 + 2^-60 collapses to 1 under round-to-nearest; a rigorous interval
    //must keep 1 + 2^-60 inside, so its supremum must be ABOVE 1.
    const interval tiny = interval(std::ldexp(1.0, -60));
    const interval sum = interval(1.0) + tiny;

    EXPECT_LE(cxsc::Inf(sum), 1.0);
    EXPECT_GT(cxsc::Sup(sum), 1.0);
}

TEST(CxscRigor, SqrtAndSqrEnclose)
{
    const interval two(2.0);
    const interval root = sqrt(two);

    //sqrt(2)^2 must enclose 2.
    const interval back = sqr(root);
    EXPECT_LE(cxsc::Inf(back), 2.0);
    EXPECT_GE(cxsc::Sup(back), 2.0);
}

TEST(CxscRigor, Log10AndAbsEnclose)
{
    //20*log10(|x|) is the decibel conversion of the whole Nichols stack.
    const interval three(3.0);
    const interval logOfThree = cxsc::log10(three);

    const double nearest = std::log10(3.0);
    EXPECT_LE(cxsc::Inf(logOfThree), nearest);
    EXPECT_GE(cxsc::Sup(logOfThree), nearest);

    const interval magnitude = abs(interval(-3.0, 2.0));
    EXPECT_EQ(cxsc::Inf(magnitude), 0.0);
    EXPECT_EQ(cxsc::Sup(magnitude), 3.0);
}

TEST(CxscRigor, AtanEnclosesPiOverFour)
{
    const interval one(1.0);
    const interval quarterPi = atan(one);

    EXPECT_LE(cxsc::Inf(quarterPi), M_PI / 4.0);
    EXPECT_GE(cxsc::Sup(quarterPi), M_PI / 4.0);

    //And it must be a genuine enclosure of the irrational value.
    EXPECT_LT(cxsc::Inf(quarterPi), cxsc::Sup(quarterPi));
}

TEST(CxscRigor, ComplexIntervalProductEncloses)
{
    //The Nichols box arithmetic multiplies complex rectangles; the product
    //of (1/3 + i/3) with its conjugate has |z|^2 = 2/9, not representable.
    const cxsc::interval third = interval(1.0) / interval(3.0);
    const cxsc::cinterval z(third, third);
    const cxsc::cinterval conjugate(third, -third);

    const cxsc::cinterval product = z * conjugate;

    EXPECT_LE(cxsc::Inf(cxsc::Re(product)), 2.0 / 9.0);
    EXPECT_GE(cxsc::Sup(cxsc::Re(product)), 2.0 / 9.0);
    EXPECT_LE(cxsc::Inf(cxsc::Im(product)), 0.0);
    EXPECT_GE(cxsc::Sup(cxsc::Im(product)), 0.0);
}

} // namespace
