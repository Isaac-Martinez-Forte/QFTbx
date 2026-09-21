/**
 * @file
 * @brief Tests of the epsilon-hull walk on small hand-made clouds.
 *
 * The walk follows Montoya's EPSHULL.M, the epsilon-hull contour of Nordin
 * 1993: the input is made unique in MATLAB complex order, the walk starts at
 * the point of largest real part, the previous point stays a candidate so a
 * spike is traversed out and back, and the contour is closed with the last
 * point repeating the first. The cases pin those semantics on a quadrilateral,
 * a regular grid whose contour is exactly its border, a triangle whose centre
 * is dropped when epsilon exceeds the diameter, collinear points, two points,
 * duplicated vertices, an epsilon too small to reach a second point, which
 * gives an empty contour, and determinism over repeated runs.
 */

#include <gtest/gtest.h>

#include <complex>

#include "src/core/templates/template_engine.h"

using namespace qftbx;

namespace {

using Complex = std::complex<double>;

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
    const qftbx::ComplexCloud nube = cloud({{0.0, 0.0}, {4.0, 0.0}, {2.0, 3.0}, {2.0, 1.0}});

    TemplateEngine t;
    const qftbx::ComplexCloud contorno = t.epsilonHull(nube, 10.0);
    ASSERT_FALSE(contorno.empty());
    EXPECT_EQ(static_cast<int>(contorno.size()), 4);
    EXPECT_EQ(contorno.front(), contorno.back());
    EXPECT_FALSE(containsPoint(contorno, Complex(2.0, 1.0)));

}

TEST(EHull, TinyEpsilonReturnsNull)
{
    const qftbx::ComplexCloud nube = cloud({{0.0, 0.0}, {2.0, 0.0}, {2.0, 1.0}, {0.0, 1.0}});

    TemplateEngine t;
    EXPECT_TRUE(t.epsilonHull(nube, 0.1).empty());
}

TEST(EHull, CollinearPointsTraverseTheSpikeBothWays)
{
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
    const qftbx::ComplexCloud nube = cloud({{0.0, 0.0}, {1.0, 0.0}});

    TemplateEngine t;
    const qftbx::ComplexCloud contorno = t.epsilonHull(nube, 2.0);
    ASSERT_FALSE(contorno.empty());
    EXPECT_EQ(static_cast<int>(contorno.size()), 3);
    EXPECT_EQ(contorno.front(), contorno.back());

}

TEST(EHull, DuplicatedVerticesAreUniqued)
{
    const qftbx::ComplexCloud nube = cloud({{0.0, 0.0}, {2.0, 0.0}, {2.0, 1.0},
                                   {0.0, 1.5}, {0.0, 1.5}, {0.0, 1.5}});

    TemplateEngine t;
    const qftbx::ComplexCloud contorno = t.epsilonHull(nube, 2.6);
    ASSERT_FALSE(contorno.empty());
    EXPECT_EQ(static_cast<int>(contorno.size()), 5);
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

}
