#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <complex>
#include <vector>

#include "src/core/math/polynomial.h"

using namespace qftbx::math;

namespace {

//Roots sorted by real part, then imaginary part, so a test can name them.
std::vector<std::complex<double>> sorted(std::vector<std::complex<double>> roots)
{
    std::sort(roots.begin(), roots.end(), [](const std::complex<double> & a, const std::complex<double> & b) {
        return a.real() != b.real() ? a.real() < b.real() : a.imag() < b.imag();
    });
    return roots;
}

} // namespace

TEST(Polynomial, RealRootsOfAQuadratic)
{
    //(s + 1)(s + 2) = s^2 + 3 s + 2
    const auto roots = sorted(polynomialRoots({1.0, 3.0, 2.0}));
    ASSERT_EQ(roots.size(), 2u);
    EXPECT_NEAR(roots[0].real(), -2.0, 1e-12);
    EXPECT_EQ(roots[0].imag(), 0.0);
    EXPECT_NEAR(roots[1].real(), -1.0, 1e-12);
    EXPECT_EQ(roots[1].imag(), 0.0);
}

TEST(Polynomial, ComplexRootsComeAsAConjugatePair)
{
    //s^2 + 2 s + 5 = (s + 1 - 2j)(s + 1 + 2j)
    const auto roots = sorted(polynomialRoots({1.0, 2.0, 5.0}));
    ASSERT_EQ(roots.size(), 2u);
    EXPECT_NEAR(roots[0].real(), -1.0, 1e-12);
    EXPECT_NEAR(roots[0].imag(), -2.0, 1e-12);
    EXPECT_NEAR(roots[1].real(), -1.0, 1e-12);
    EXPECT_NEAR(roots[1].imag(), 2.0, 1e-12);
}

TEST(Polynomial, RootsAtTheOriginAreExactAndLeadingZerosAreDropped)
{
    //0 s^4 + s^3 + 2 s^2 = s^2 (s + 2)
    const auto roots = sorted(polynomialRoots({0.0, 1.0, 2.0, 0.0, 0.0}));
    ASSERT_EQ(roots.size(), 3u);
    EXPECT_NEAR(roots[0].real(), -2.0, 1e-12);
    EXPECT_EQ(roots[1], std::complex<double>(0.0, 0.0));
    EXPECT_EQ(roots[2], std::complex<double>(0.0, 0.0));

    EXPECT_TRUE(polynomialRoots({3.0}).empty());
    EXPECT_TRUE(polynomialRoots({}).empty());
}

TEST(Polynomial, ASixthDegreeWithClusteredRootsIsResolved)
{
    //(s + 1)^2 (s + 1.001)^2 (s^2 + 100): a repeated root next to another,
    //and a far pair on the axis, in one polynomial.
    std::vector<double> p{1.0};
    const auto multiply = [&](std::vector<double> factor) {
        std::vector<double> product(p.size() + factor.size() - 1, 0.0);
        for (std::size_t i = 0; i < p.size(); ++i)
            for (std::size_t j = 0; j < factor.size(); ++j)
                product[i + j] += p[i] * factor[j];
        p = product;
    };
    multiply({1.0, 1.0}); multiply({1.0, 1.0});
    multiply({1.0, 1.001}); multiply({1.0, 1.001});
    multiply({1.0, 0.0, 100.0});

    const auto roots = sorted(polynomialRoots(p));
    ASSERT_EQ(roots.size(), 6u);
    //The four near -1 within the accuracy a double root allows (sqrt of eps).
    for (int i = 0; i < 4; ++i) {
        EXPECT_NEAR(roots[i].real(), -1.0005, 1e-3);
        EXPECT_NEAR(roots[i].imag(), 0.0, 1e-3);
    }
    EXPECT_NEAR(std::abs(roots[4].imag()), 10.0, 1e-9);
    EXPECT_NEAR(std::abs(roots[5].imag()), 10.0, 1e-9);
    EXPECT_LT(roots[4].imag() * roots[5].imag(), 0.0);
    EXPECT_NEAR(roots[4].real(), 0.0, 1e-9);
}

TEST(Polynomial, CoefficientsAreRecoveredFromValuesAlone)
{
    //s^3 - 2 s + 1, with its zero coefficient.
    const auto cubic = polynomialCoefficients([](std::complex<double> s) { return s * s * s - 2.0 * s + 1.0; });
    ASSERT_TRUE(cubic.has_value());
    ASSERT_EQ(cubic->size(), 4u);
    EXPECT_NEAR((*cubic)[0], 1.0, 1e-12);
    EXPECT_NEAR((*cubic)[1], 0.0, 1e-12);
    EXPECT_NEAR((*cubic)[2], -2.0, 1e-12);
    EXPECT_NEAR((*cubic)[3], 1.0, 1e-12);

    //A constant is a polynomial of degree zero, without roots.
    const auto constant = polynomialCoefficients([](std::complex<double>) { return std::complex<double>(4.0, 0.0); });
    ASSERT_TRUE(constant.has_value());
    EXPECT_EQ(constant->size(), 1u);
    EXPECT_TRUE(polynomialRoots(*constant).empty());
}

TEST(Polynomial, RootsFarFromOneKeepTheirPrecision)
{
    //The maglev plant's denominator, s^2 + 430.25: poles on the axis at
    //+-j 20.742..., a hundred times farther than the sampling circle.
    const auto coefficients = polynomialCoefficients([](std::complex<double> s) { return s * s + 430.25; });
    ASSERT_TRUE(coefficients.has_value());
    const auto roots = polynomialRoots(*coefficients);
    ASSERT_EQ(roots.size(), 2u);
    const auto axis = imaginaryAxisFrequencies(roots);
    ASSERT_EQ(axis.size(), 1u);
    EXPECT_NEAR(axis[0], std::sqrt(430.25), 1e-9);
    EXPECT_EQ(rightHalfPlaneCount(roots), 0);

    //And its unstable twin, s^2 - 430.25: one pole on each side.
    const auto twin = polynomialCoefficients([](std::complex<double> s) { return s * s - 430.25; });
    ASSERT_TRUE(twin.has_value());
    EXPECT_EQ(rightHalfPlaneCount(polynomialRoots(*twin)), 1);
    EXPECT_TRUE(imaginaryAxisFrequencies(polynomialRoots(*twin)).empty());
}

TEST(Polynomial, WhatIsNotAPolynomialIsRefused)
{
    //A delay in the denominator, a rational function, and a transcendental.
    EXPECT_FALSE(polynomialCoefficients([](std::complex<double> s) { return s + std::exp(-s); }).has_value());
    EXPECT_FALSE(polynomialCoefficients([](std::complex<double> s) { return 1.0 / (s + 1.0); }).has_value());
    EXPECT_FALSE(polynomialCoefficients([](std::complex<double> s) { return std::sin(s); }).has_value());
    //Beyond the degree allowed.
    EXPECT_FALSE(polynomialCoefficients([](std::complex<double> s) { return std::pow(s, 30); }, 24).has_value());
}

TEST(Polynomial, RightHalfPlaneCountIgnoresTheAxis)
{
    const std::vector<std::complex<double>> roots{{1.0, 0.0}, {-1.0, 0.0}, {0.0, 2.0}, {1e-12, 3.0}, {0.5, -0.5}};
    EXPECT_EQ(rightHalfPlaneCount(roots), 2);
    const auto axis = imaginaryAxisFrequencies(roots);
    ASSERT_EQ(axis.size(), 2u);
    EXPECT_EQ(axis[0], 2.0);
    EXPECT_EQ(axis[1], 3.0);
}

TEST(Polynomial, ADoubleRootAtTheOriginDoesNotBecomeAPairOffIt)
{
    //The ACC'90 plant's denominator, s^2 (s^2 + 0.02 s + 1): recovered from
    //values, its two last coefficients come back as rounding noise instead
    //of zero, and the double root at the origin then solves as a pair about
    //1e-8 away - the square root of that noise - which reads as a pole ON
    //the axis and puts an indentation where the plant has none.
    const auto coefficients = polynomialCoefficients(
        [](std::complex<double> s) { return s * s * (s * s + 0.02 * s + 1.0); });
    ASSERT_TRUE(coefficients.has_value());

    const auto roots = polynomialRoots(*coefficients);
    ASSERT_EQ(roots.size(), 4u);
    EXPECT_TRUE(imaginaryAxisFrequencies(roots).empty()) << "the origin is not an indentation";
    EXPECT_EQ(rightHalfPlaneCount(roots), 0);

    //The lightly damped pair is still where it belongs.
    int nearTheRay = 0;
    for (const std::complex<double> & root : roots) {
        if (std::abs(std::abs(root.imag()) - 0.99995) < 1e-3) {
            ++nearTheRay;
            EXPECT_NEAR(root.real(), -0.01, 1e-6);
        }
    }
    EXPECT_EQ(nearTheRay, 2);
}
