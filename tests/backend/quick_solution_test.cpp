// The Quick Solution cutting equations of algorithm NK, validated against
// the worked numerical example of the paper (Paluri/Nataraj and Kubal,
// Int. J. Robust Nonlinear Control 17:251-264, 2007, sec. 3.3.1): the DC
// motor design at omega = 0.1 rad/s with |Bi|min = 37.2342 dB and the box
//   z = ([0.1, 2010597.38], [0.7, 3.3], [1025.5, 4834.5], [193.55, 912.45])
// reduces, in the paper, to
//   k'  = 439429.20, z1' = 0.7146, p1' = 4692.15, p2' = 885.58.
// The nominal plant is P0(s) = 1/(s(s+1)).

#include <gtest/gtest.h>

#include <complex>

#include <QVector>

#include "Modelo/LoopShaping/quick_solution.h"

namespace {

namespace qs = qftbx::quick_solution;

class QuickSolutionPaperExample : public ::testing::Test
{
protected:
    const qreal w = 0.1;
    const std::complex<qreal> p0 = 1.0 /
            (std::complex<qreal>(0.0, 0.1) * std::complex<qreal>(1.0, 0.1));
    const qreal boundMin = std::pow(10.0, 37.2342 / 20.0);
    const qreal gainSup = 2010597.38;
    const QVector<qreal> zeroSups{3.3};
    const QVector<qreal> poleInfs{1025.5, 193.55};
};

TEST_F(QuickSolutionPaperExample, GainCutMatchesThePaper)
{
    //The paper rounds |Bi|min to 37.2342 dB: a 0.1% tolerance absorbs it.
    EXPECT_NEAR(qs::gainCut(boundMin, zeroSups, poleInfs, w, p0),
                439429.20, 0.001 * 439429.20);
}

TEST_F(QuickSolutionPaperExample, ZeroCutMatchesThePaper)
{
    EXPECT_NEAR(qs::zeroCut(boundMin, gainSup, zeroSups, poleInfs, 0, w, p0),
                0.7146, 0.001);
}

TEST_F(QuickSolutionPaperExample, PoleCutsMatchThePaper)
{
    EXPECT_NEAR(qs::poleCut(boundMin, gainSup, zeroSups, poleInfs, 0, w, p0),
                4692.15, 0.001 * 4692.15);
    EXPECT_NEAR(qs::poleCut(boundMin, gainSup, zeroSups, poleInfs, 1, w, p0),
                885.58, 0.001 * 885.58);
}

TEST_F(QuickSolutionPaperExample, NoRealSolutionReturnsNegative)
{
    //A bound low enough pushes the zero equation below omega^2.
    EXPECT_LT(qs::zeroCut(1e-9, gainSup, zeroSups, poleInfs, 0, w, p0), 0.0);
}

} // namespace
