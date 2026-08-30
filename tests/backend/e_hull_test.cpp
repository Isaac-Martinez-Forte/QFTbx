// Characterisation tests for Templates::e_hull, the C++ port of Montoya's
// EPSHULL.M (the epsilon-hull contour algorithm defined in Nordin 1993).
// Synthetic point clouds pin the current behaviour; the "// BUG:" cases
// document the known divergences from the MATLAB reference (D2: no unique(),
// D9: the previous point is excluded from the candidates, D14: exceeding
// MAXP truncates instead of failing) and must be flipped when each is fixed.

#include <gtest/gtest.h>

#include <complex>

#include <QVector>

#include "Modelo/Templates/templates.h"

namespace {

using Complex = std::complex<qreal>;

QVector<Complex> cloud(std::initializer_list<Complex> points)
{
    return QVector<Complex>(points);
}

bool containsPoint(const QVector<Complex>* contour, Complex point)
{
    for (const Complex& c : *contour) {
        if (c == point) {
            return true;
        }
    }
    return false;
}

TEST(EHull, IrregularQuadKeepsItsFourCorners)
{
    // Four points, all mutually within epsilon: every one is on the contour.
    // The walk starts at the point with the largest imaginary part (current
    // C++ behaviour; MATLAB and the PFC text start at the largest real part
    // - pending alignment, D1).
    QVector<Complex> nube = cloud({{0.0, 0.0}, {2.0, 0.0}, {2.0, 1.0}, {0.0, 1.5}});

    Templates t;
    QVector<Complex>* contorno = t.e_hull(&nube, 2.6);
    ASSERT_NE(contorno, nullptr);
    EXPECT_EQ(contorno->size(), 4);
    EXPECT_EQ(contorno->at(0), Complex(0.0, 1.5));
    for (const Complex& p : nube) {
        EXPECT_TRUE(containsPoint(contorno, p));
    }
    delete contorno;
}

TEST(EHull, RegularGridProducesABogusContour)
{
    // BUG (D12/D9): on a 5x5 unit grid with epsilon reaching the
    // 4-neighbours, psi ties abound and the tie-breaking diverges from
    // MATLAB (which walks the unique()-sorted input): the walk closes
    // prematurely after 4 points and even includes an interior point.
    // Expected after the fix: exactly the 16 border points.
    QVector<Complex> nube;
    for (int x = 0; x < 5; ++x) {
        for (int y = 0; y < 5; ++y) {
            nube.append(Complex(x, y));
        }
    }

    Templates t;
    QVector<Complex>* contorno = t.e_hull(&nube, 1.2);
    ASSERT_NE(contorno, nullptr);
    EXPECT_EQ(contorno->size(), 4);
    EXPECT_TRUE(containsPoint(contorno, Complex(1.0, 3.0))); // interior!
    delete contorno;
}

TEST(EHull, TriangleWithCenterDropsTheCenter)
{
    // With epsilon larger than the diameter the epsilon-hull degenerates to
    // the convex hull: the centroid must be dropped.
    QVector<Complex> nube = cloud({{0.0, 0.0}, {4.0, 0.0}, {2.0, 3.0}, {2.0, 1.0}});

    Templates t;
    QVector<Complex>* contorno = t.e_hull(&nube, 10.0);
    ASSERT_NE(contorno, nullptr);
    EXPECT_EQ(contorno->size(), 3);
    EXPECT_FALSE(containsPoint(contorno, Complex(2.0, 1.0)));
    delete contorno;
}

TEST(EHull, TinyEpsilonReturnsNull)
{
    // No candidate within epsilon of the starting point: the current
    // contract is a silent null (should become a typed exception).
    QVector<Complex> nube = cloud({{0.0, 0.0}, {2.0, 0.0}, {2.0, 1.0}, {0.0, 1.0}});

    Templates t;
    EXPECT_EQ(t.e_hull(&nube, 0.1), nullptr);
}

TEST(EHull, CollinearPointsReturnNull)
{
    // BUG (D9): EPSHULL.M deliberately keeps the previous point among the
    // candidates (its exclusion is commented out), so the walk can turn
    // back at the end of a spike and traverse it in both directions. The
    // C++ port excludes it, so a purely collinear cloud dead-ends at the
    // far extreme and e_hull gives up with a null.
    QVector<Complex> nube = cloud(
        {{0.0, 0.0}, {1.0, 0.0}, {2.0, 0.0}, {3.0, 0.0}, {4.0, 0.0}});

    Templates t;
    EXPECT_EQ(t.e_hull(&nube, 1.5), nullptr);
}

TEST(EHull, TwoPointsReturnNull)
{
    // BUG (D9): same dead-end with the minimal spike: two points.
    QVector<Complex> nube = cloud({{0.0, 0.0}, {1.0, 0.0}});

    Templates t;
    EXPECT_EQ(t.e_hull(&nube, 2.0), nullptr);
}

TEST(EHull, DuplicatedVertexTruncatesSilently)
{
    // BUG (D2 + D14): MATLAB unique()s the input and treats exceeding MAXP
    // as an error; the port does neither. With a duplicated vertex the
    // stop condition (index-based) can fail to close and the walk runs
    // until MAXP, returning a truncated contour as if it were valid.
    QVector<Complex> nube = cloud({{0.0, 0.0}, {2.0, 0.0}, {2.0, 1.0},
                                   {0.0, 1.5}, {0.0, 1.5}, {0.0, 1.5}});

    Templates t;
    QVector<Complex>* contorno = t.e_hull(&nube, 2.6);
    // Pin only the loose contract: it "succeeds" and stays within 3n points.
    ASSERT_NE(contorno, nullptr);
    EXPECT_LE(contorno->size(), 3 * nube.size());
    delete contorno;
}

TEST(EHull, IsDeterministic)
{
    QVector<Complex> nube;
    for (int x = 0; x < 5; ++x) {
        for (int y = 0; y < 5; ++y) {
            nube.append(Complex(x * 1.1, y * 0.9));
        }
    }

    Templates t;
    QVector<Complex>* primero = t.e_hull(&nube, 1.3);
    ASSERT_NE(primero, nullptr);
    for (int run = 0; run < 20; ++run) {
        QVector<Complex>* otra = t.e_hull(&nube, 1.3);
        ASSERT_NE(otra, nullptr);
        EXPECT_EQ(*otra, *primero);
        delete otra;
    }
    delete primero;
}

} // namespace
