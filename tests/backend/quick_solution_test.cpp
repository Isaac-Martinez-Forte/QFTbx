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


// The phase cutting equations of algorithm MC (Martinez-Forte and Cervera,
// Int. J. Robust Nonlinear Control 31, 2021, QS2 stage 2), checked as
// properties: every configuration on the removed side of the cut stays
// inside the forbidden phase strip (soundness), and just past the cut the
// closest-to-allowed corner leaves it (tightness).

qreal loopPhase(qreal phi0, const QVector<qreal> & zeros,
                const QVector<qreal> & poles, qreal w)
{
    qreal phase = phi0;
    foreach (qreal z, zeros) {
        phase += std::atan2(w, z);
    }
    foreach (qreal p, poles) {
        phase -= std::atan2(w, p);
    }
    return phase;
}

class QuickSolutionPhaseCuts : public ::testing::Test
{
protected:
    const qreal w = 2.0;
    const qreal phi0 = -2.0;
    const QVector<qreal> zeroInfs{0.5, 1.0};
    const QVector<qreal> zeroSups{50.0, 20.0};
    const QVector<qreal> poleInfs{0.2};
    const QVector<qreal> poleSups{80.0};
};

TEST_F(QuickSolutionPhaseCuts, HighStripZeroCutIsSoundAndTight)
{
    //Poles far enough that a positive margin exists for the first zero.
    const QVector<qreal> farPoleInfs{20.0};
    const qreal thetaMax = phi0 + 0.35;

    const qreal cut = qs::zeroPhaseCutHigh(thetaMax, phi0, zeroSups, farPoleInfs, 0, w);
    ASSERT_GT(cut, zeroInfs.at(0));
    ASSERT_LT(cut, zeroSups.at(0));

    for (qreal z0 : {zeroInfs.at(0), 0.5 * cut, 0.999 * cut}) {
        for (qreal z1 : {zeroInfs.at(1), zeroSups.at(1)}) {
            for (qreal p : {20.0, 80.0}) {
                EXPECT_GT(loopPhase(phi0, {z0, z1}, {p}, w), thetaMax);
            }
        }
    }

    EXPECT_LE(loopPhase(phi0, {1.001 * cut, zeroSups.at(1)}, {20.0}, w), thetaMax);
}

TEST_F(QuickSolutionPhaseCuts, HighStripPoleCutIsSoundAndTight)
{
    const qreal thetaMax = phi0 - 0.2;

    const qreal cut = qs::polePhaseCutHigh(thetaMax, phi0, zeroSups, poleInfs, 0, w);
    ASSERT_GT(cut, poleInfs.at(0));
    ASSERT_LT(cut, poleSups.at(0));

    for (qreal p : {1.001 * cut, 2.0 * cut, poleSups.at(0)}) {
        for (qreal z0 : {zeroInfs.at(0), zeroSups.at(0)}) {
            for (qreal z1 : {zeroInfs.at(1), zeroSups.at(1)}) {
                EXPECT_GT(loopPhase(phi0, {z0, z1}, {p}, w), thetaMax);
            }
        }
    }

    EXPECT_LE(loopPhase(phi0, {zeroSups.at(0), zeroSups.at(1)}, {0.999 * cut}, w), thetaMax);
}

TEST_F(QuickSolutionPhaseCuts, LowStripZeroCutIsSoundAndTight)
{
    const qreal thetaMin = phi0 + 1.4;

    const qreal cut = qs::zeroPhaseCutLow(thetaMin, phi0, zeroInfs, poleSups, 0, w);
    ASSERT_GT(cut, zeroInfs.at(0));
    ASSERT_LT(cut, zeroSups.at(0));

    for (qreal z0 : {1.001 * cut, 2.0 * cut, zeroSups.at(0)}) {
        for (qreal z1 : {zeroInfs.at(1), zeroSups.at(1)}) {
            for (qreal p : {poleInfs.at(0), poleSups.at(0)}) {
                EXPECT_LT(loopPhase(phi0, {z0, z1}, {p}, w), thetaMin);
            }
        }
    }

    EXPECT_GE(loopPhase(phi0, {0.999 * cut, zeroInfs.at(1)}, {poleSups.at(0)}, w), thetaMin);
}

TEST_F(QuickSolutionPhaseCuts, LowStripPoleCutIsSoundAndTight)
{
    const qreal thetaMin = phi0 + 2.1;

    const qreal cut = qs::polePhaseCutLow(thetaMin, phi0, zeroInfs, poleSups, 0, w);
    ASSERT_GT(cut, poleInfs.at(0));
    ASSERT_LT(cut, poleSups.at(0));

    for (qreal p : {poleInfs.at(0), 0.5 * cut, 0.999 * cut}) {
        for (qreal z0 : {zeroInfs.at(0), zeroSups.at(0)}) {
            for (qreal z1 : {zeroInfs.at(1), zeroSups.at(1)}) {
                EXPECT_LT(loopPhase(phi0, {z0, z1}, {p}, w), thetaMin);
            }
        }
    }

    EXPECT_GE(loopPhase(phi0, {zeroInfs.at(0), zeroInfs.at(1)}, {1.001 * cut}, w), thetaMin);
}

TEST_F(QuickSolutionPhaseCuts, MarginOutsideItsBranchReturnsNegative)
{
    //Margin below zero (threshold unreachable towards the allowed side)
    //and margin beyond pi/2 (every value of the parameter reaches it).
    const QVector<qreal> farPoleInfs{20.0};
    EXPECT_LT(qs::zeroPhaseCutHigh(phi0 - 1.0, phi0, zeroSups, farPoleInfs, 0, w), 0.0);
    EXPECT_LT(qs::zeroPhaseCutHigh(phi0 + 2.0, phi0, zeroSups, farPoleInfs, 0, w), 0.0);
}

} // namespace
