// Tests for TemplateEngine::epsilonHull, now a faithful port of Montoya's EPSHULL.M
// (the epsilon-hull contour algorithm defined in Nordin 1993), validated
// against a line-by-line Python oracle of the MATLAB. Faithful semantics:
// unique()d input in MATLAB complex order, max-real starting point, the
// previous point stays a candidate (spikes are traversed both ways), and
// contours are CLOSED (the last point repeats the first). When the
// reference walk cycles (its known limitation on clustered clouds) epsilonHull
// falls back to the relaxed historical walk with a warning; the golden
// tests exercise that path.

#include <gtest/gtest.h>

#include <complex>

#include <QVector>

#include "src/core/templates/template_engine.h"

namespace {

using Complex = std::complex<qreal>;

qftbx::ComplexCloud cloud(std::initializer_list<Complex> points)
{
    return qftbx::ComplexCloud(points);
}

bool containsPoint(const qftbx::ComplexCloud & contour, Complex point)
{
    for (const Complex& c : contour) {
        if (c == point) {
            return true;
        }
    }
    return false;
}

TEST(EHull, IrregularQuadKeepsItsFourCornersClosed)
{
    // Faithful EPSHULL.M semantics: the contour is CLOSED (the last point
    // repeats the first) and starts at the max-real point; among real-part
    // ties the first in MATLAB unique order (by modulus, then phase) wins.
    const qftbx::ComplexCloud nube = cloud({{0.0, 0.0}, {2.0, 0.0}, {2.0, 1.0}, {0.0, 1.5}});

    TemplateEngine t;
    const qftbx::ComplexCloud contorno = t.epsilonHull(nube, 2.6);
    ASSERT_FALSE(contorno.empty());
    EXPECT_EQ(static_cast<int>(contorno.size()), 5);
    EXPECT_EQ(contorno.front(), Complex(2.0, 0.0));
    EXPECT_EQ(contorno.front(), contorno.back());
    for (const Complex& p : nube) {
        EXPECT_TRUE(containsPoint(contorno, p));
    }
    
}

TEST(EHull, RegularGridKeepsExactlyTheBorder)
{
    // Fixed (D12/D2): walking the unique()-sorted cloud like MATLAB, the
    // psi ties resolve correctly: the 16 border points, closed, and no
    // interior point (the old walk closed after 4 points including one).
    qftbx::ComplexCloud nube;
    for (int x = 0; x < 5; ++x) {
        for (int y = 0; y < 5; ++y) {
            nube.push_back(Complex(x, y));
        }
    }

    TemplateEngine t;
    const qftbx::ComplexCloud contorno = t.epsilonHull(nube, 1.2);
    ASSERT_FALSE(contorno.empty());
    EXPECT_EQ(static_cast<int>(contorno.size()), 17);
    EXPECT_EQ(contorno.front(), contorno.back());
    for (const Complex& p : contorno) {
        const bool border = p.real() == 0.0 || p.real() == 4.0 ||
                            p.imag() == 0.0 || p.imag() == 4.0;
        EXPECT_TRUE(border) << "interior point in contour: " << p.real()
                            << "," << p.imag();
    }
    
}

TEST(EHull, TriangleWithCenterDropsTheCenter)
{
    // With epsilon larger than the diameter the epsilon-hull degenerates to
    // the convex hull: the centroid must be dropped.
    const qftbx::ComplexCloud nube = cloud({{0.0, 0.0}, {4.0, 0.0}, {2.0, 3.0}, {2.0, 1.0}});

    TemplateEngine t;
    const qftbx::ComplexCloud contorno = t.epsilonHull(nube, 10.0);
    ASSERT_FALSE(contorno.empty());
    EXPECT_EQ(static_cast<int>(contorno.size()), 4); // 3 vertices + closing point
    EXPECT_EQ(contorno.front(), contorno.back());
    EXPECT_FALSE(containsPoint(contorno, Complex(2.0, 1.0)));
    
}

TEST(EHull, TinyEpsilonReturnsNull)
{
    // No candidate within epsilon of the starting point: the current
    // contract is a silent null (should become a typed exception).
    const qftbx::ComplexCloud nube = cloud({{0.0, 0.0}, {2.0, 0.0}, {2.0, 1.0}, {0.0, 1.0}});

    TemplateEngine t;
    EXPECT_TRUE(t.epsilonHull(nube, 0.1).empty());
}

TEST(EHull, CollinearPointsTraverseTheSpikeBothWays)
{
    // Fixed (D9): like EPSHULL.M, the previous point stays among the
    // candidates, so the walk traverses the spike out and back:
    // 4-3-2-1-0-1-2-3-4 (2n-1 points, closed, repetitions kept as real
    // geometric information).
    const qftbx::ComplexCloud nube = cloud(
        {{0.0, 0.0}, {1.0, 0.0}, {2.0, 0.0}, {3.0, 0.0}, {4.0, 0.0}});

    TemplateEngine t;
    const qftbx::ComplexCloud contorno = t.epsilonHull(nube, 1.5);
    ASSERT_FALSE(contorno.empty());
    EXPECT_EQ(static_cast<int>(contorno.size()), 9);
    EXPECT_EQ(contorno.front(), contorno.back());
    for (const Complex& p : nube) {
        EXPECT_TRUE(containsPoint(contorno, p));
    }
    
}

TEST(EHull, TwoPointsFormTheMinimalClosedContour)
{
    // Fixed (D9): the minimal spike closes by walking back: b1-b2-b1.
    const qftbx::ComplexCloud nube = cloud({{0.0, 0.0}, {1.0, 0.0}});

    TemplateEngine t;
    const qftbx::ComplexCloud contorno = t.epsilonHull(nube, 2.0);
    ASSERT_FALSE(contorno.empty());
    EXPECT_EQ(static_cast<int>(contorno.size()), 3);
    EXPECT_EQ(contorno.front(), contorno.back());
    
}

TEST(EHull, DuplicatedVerticesAreUniqued)
{
    // Fixed (D2): the input is unique()d like in MATLAB, so duplicated
    // vertices cannot derail the index-based stop condition any more.
    const qftbx::ComplexCloud nube = cloud({{0.0, 0.0}, {2.0, 0.0}, {2.0, 1.0},
                                   {0.0, 1.5}, {0.0, 1.5}, {0.0, 1.5}});

    TemplateEngine t;
    const qftbx::ComplexCloud contorno = t.epsilonHull(nube, 2.6);
    ASSERT_FALSE(contorno.empty());
    EXPECT_EQ(static_cast<int>(contorno.size()), 5); // 4 unique corners + closing point
    EXPECT_EQ(contorno.front(), contorno.back());
    
}

TEST(EHull, IsDeterministic)
{
    qftbx::ComplexCloud nube;
    for (int x = 0; x < 5; ++x) {
        for (int y = 0; y < 5; ++y) {
            nube.push_back(Complex(x * 1.1, y * 0.9));
        }
    }

    TemplateEngine t;
    const qftbx::ComplexCloud primero = t.epsilonHull(nube, 1.3);
    ASSERT_FALSE(primero.empty());
    for (int run = 0; run < 20; ++run) {
        const qftbx::ComplexCloud otra = t.epsilonHull(nube, 1.3);
        ASSERT_FALSE(otra.empty());
        EXPECT_EQ(otra, primero);
    }
}

} // namespace
