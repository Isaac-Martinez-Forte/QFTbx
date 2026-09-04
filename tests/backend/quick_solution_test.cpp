// The Quick Solution cutting equations of algorithm NK, validated against
// the worked numerical example of the paper (Paluri/Nataraj and Kubal,
// Int. J. Robust Nonlinear Control 17:251-264, 2007, sec. 3.3.1): the DC
// motor design at omega = 0.1 rad/s with |Bi|min = 37.2342 dB and the box
//   z = ([0.1, 2010597.38], [0.7, 3.3], [1025.5, 4834.5], [193.55, 912.45])
// reduces, in the paper, to
//   k'  = 439429.20, z1' = 0.7146, p1' = 4692.15, p2' = 885.58.
// The nominal plant is P0(s) = 1/(s(s+1)).

#include <gtest/gtest.h>

#include <vector>

#include <complex>


#include "src/core/loopshaping/quick_solution.h"

using namespace qftbx;

namespace {

namespace qs = qftbx::quick_solution;

class QuickSolutionPaperExample : public ::testing::Test
{
protected:
    const double w = 0.1;
    const std::complex<double> p0 = 1.0 /
            (std::complex<double>(0.0, 0.1) * std::complex<double>(1.0, 0.1));
    const double boundMin = std::pow(10.0, 37.2342 / 20.0);
    const double gainSup = 2010597.38;
    const std::vector<double> zeroSups{3.3};
    const std::vector<double> poleInfs{1025.5, 193.55};
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


// The phase cutting equations of algorithm MC (Martinez-Forte and Cervera,
// Int. J. Robust Nonlinear Control 31, 2021, QS2 stage 2), checked as
// properties: every configuration on the removed side of the cut stays
// inside the forbidden phase strip (soundness), and just past the cut the
// closest-to-allowed corner leaves it (tightness).

double loopPhase(double phi0, const std::vector<double> & zeros,
                const std::vector<double> & poles, double w)
{
    double phase = phi0;
    for (double z : zeros) {
        phase += std::atan2(w, z);
    }
    for (double p : poles) {
        phase -= std::atan2(w, p);
    }
    return phase;
}

class QuickSolutionPhaseCuts : public ::testing::Test
{
protected:
    const double w = 2.0;
    const double phi0 = -2.0;
    const std::vector<double> zeroInfs{0.5, 1.0};
    const std::vector<double> zeroSups{50.0, 20.0};
    const std::vector<double> poleInfs{0.2};
    const std::vector<double> poleSups{80.0};
};

TEST_F(QuickSolutionPhaseCuts, HighStripZeroCutIsSoundAndTight)
{
    //Poles far enough that a positive margin exists for the first zero.
    const std::vector<double> farPoleInfs{20.0};
    const double thetaMax = phi0 + 0.35;

    const double cut = qs::zeroPhaseCutHigh(thetaMax, phi0, zeroSups, farPoleInfs, 0, w);
    ASSERT_GT(cut, zeroInfs[0]);
    ASSERT_LT(cut, zeroSups[0]);

    for (double z0 : {zeroInfs[0], 0.5 * cut, 0.999 * cut}) {
        for (double z1 : {zeroInfs[1], zeroSups[1]}) {
            for (double p : {20.0, 80.0}) {
                EXPECT_GT(loopPhase(phi0, {z0, z1}, {p}, w), thetaMax);
            }
        }
    }

    EXPECT_LE(loopPhase(phi0, {1.001 * cut, zeroSups[1]}, {20.0}, w), thetaMax);
}

TEST_F(QuickSolutionPhaseCuts, HighStripPoleCutIsSoundAndTight)
{
    const double thetaMax = phi0 - 0.2;

    const double cut = qs::polePhaseCutHigh(thetaMax, phi0, zeroSups, poleInfs, 0, w);
    ASSERT_GT(cut, poleInfs[0]);
    ASSERT_LT(cut, poleSups[0]);

    for (double p : {1.001 * cut, 2.0 * cut, poleSups[0]}) {
        for (double z0 : {zeroInfs[0], zeroSups[0]}) {
            for (double z1 : {zeroInfs[1], zeroSups[1]}) {
                EXPECT_GT(loopPhase(phi0, {z0, z1}, {p}, w), thetaMax);
            }
        }
    }

    EXPECT_LE(loopPhase(phi0, {zeroSups[0], zeroSups[1]}, {0.999 * cut}, w), thetaMax);
}

TEST_F(QuickSolutionPhaseCuts, LowStripZeroCutIsSoundAndTight)
{
    const double thetaMin = phi0 + 1.4;

    const double cut = qs::zeroPhaseCutLow(thetaMin, phi0, zeroInfs, poleSups, 0, w);
    ASSERT_GT(cut, zeroInfs[0]);
    ASSERT_LT(cut, zeroSups[0]);

    for (double z0 : {1.001 * cut, 2.0 * cut, zeroSups[0]}) {
        for (double z1 : {zeroInfs[1], zeroSups[1]}) {
            for (double p : {poleInfs[0], poleSups[0]}) {
                EXPECT_LT(loopPhase(phi0, {z0, z1}, {p}, w), thetaMin);
            }
        }
    }

    EXPECT_GE(loopPhase(phi0, {0.999 * cut, zeroInfs[1]}, {poleSups[0]}, w), thetaMin);
}

TEST_F(QuickSolutionPhaseCuts, LowStripPoleCutIsSoundAndTight)
{
    const double thetaMin = phi0 + 2.1;

    const double cut = qs::polePhaseCutLow(thetaMin, phi0, zeroInfs, poleSups, 0, w);
    ASSERT_GT(cut, poleInfs[0]);
    ASSERT_LT(cut, poleSups[0]);

    for (double p : {poleInfs[0], 0.5 * cut, 0.999 * cut}) {
        for (double z0 : {zeroInfs[0], zeroSups[0]}) {
            for (double z1 : {zeroInfs[1], zeroSups[1]}) {
                EXPECT_LT(loopPhase(phi0, {z0, z1}, {p}, w), thetaMin);
            }
        }
    }

    EXPECT_GE(loopPhase(phi0, {zeroInfs[0], zeroInfs[1]}, {1.001 * cut}, w), thetaMin);
}

TEST_F(QuickSolutionPhaseCuts, MarginOutsideItsBranchReturnsNegative)
{
    //Margin below zero (threshold unreachable towards the allowed side)
    //and margin beyond pi/2 (every value of the parameter reaches it).
    const std::vector<double> farPoleInfs{20.0};
    EXPECT_LT(qs::zeroPhaseCutHigh(phi0 - 1.0, phi0, zeroSups, farPoleInfs, 0, w), 0.0);
    EXPECT_LT(qs::zeroPhaseCutHigh(phi0 + 2.0, phi0, zeroSups, farPoleInfs, 0, w), 0.0);
}

} // namespace
