/**
 * @file
 * @brief Tests of polynomial roots, coefficient recovery and pole counting.
 *
 * Roots are checked on products with known factors: a quadratic with real
 * roots, a conjugate pair, roots at the origin with leading zeros dropped, and
 * a sixth degree with a double root beside another at 1.001 and a far pair on
 * the axis. Coefficients recovered from values alone must match the polynomial
 * and refuse a delay, a rational function, a transcendental and a degree
 * beyond the allowed one. The maglev denominator s^2 + 430.25 and the ACC'90
 * denominator s^2 (s^2 + 0.02 s + 1) check that far roots keep their precision
 * and that a double root at the origin is not read as a pair on the axis,
 * which would put an indentation where the plant has none.
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <complex>
#include <limits>
#include <random>
#include <vector>

#include "src/core/math/polynomial.h"

using namespace qftbx::math;

namespace {

std::vector<std::complex<double>> sorted(std::vector<std::complex<double>> roots)
{
    std::sort(roots.begin(), roots.end(), [](const std::complex<double> & a, const std::complex<double> & b) {
        return a.real() != b.real() ? a.real() < b.real() : a.imag() < b.imag();
    });
    return roots;
}

}

TEST(Polynomial, RealRootsOfAQuadratic)
{
    const auto roots = sorted(polynomialRoots({1.0, 3.0, 2.0}));
    ASSERT_EQ(roots.size(), 2u);
    EXPECT_NEAR(roots[0].real(), -2.0, 1e-12);
    EXPECT_EQ(roots[0].imag(), 0.0);
    EXPECT_NEAR(roots[1].real(), -1.0, 1e-12);
    EXPECT_EQ(roots[1].imag(), 0.0);
}

TEST(Polynomial, ComplexRootsComeAsAConjugatePair)
{
    const auto roots = sorted(polynomialRoots({1.0, 2.0, 5.0}));
    ASSERT_EQ(roots.size(), 2u);
    EXPECT_NEAR(roots[0].real(), -1.0, 1e-12);
    EXPECT_NEAR(roots[0].imag(), -2.0, 1e-12);
    EXPECT_NEAR(roots[1].real(), -1.0, 1e-12);
    EXPECT_NEAR(roots[1].imag(), 2.0, 1e-12);
}

TEST(Polynomial, RootsAtTheOriginAreExactAndLeadingZerosAreDropped)
{
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
    const auto cubic = polynomialCoefficients([](std::complex<double> s) { return s * s * s - 2.0 * s + 1.0; });
    ASSERT_TRUE(cubic.has_value());
    ASSERT_EQ(cubic->size(), 4u);
    EXPECT_NEAR((*cubic)[0], 1.0, 1e-12);
    EXPECT_NEAR((*cubic)[1], 0.0, 1e-12);
    EXPECT_NEAR((*cubic)[2], -2.0, 1e-12);
    EXPECT_NEAR((*cubic)[3], 1.0, 1e-12);

    const auto constant = polynomialCoefficients([](std::complex<double>) { return std::complex<double>(4.0, 0.0); });
    ASSERT_TRUE(constant.has_value());
    EXPECT_EQ(constant->size(), 1u);
    EXPECT_TRUE(polynomialRoots(*constant).empty());
}

TEST(Polynomial, RootsFarFromOneKeepTheirPrecision)
{
    const auto coefficients = polynomialCoefficients([](std::complex<double> s) { return s * s + 430.25; });
    ASSERT_TRUE(coefficients.has_value());
    const auto roots = polynomialRoots(*coefficients);
    ASSERT_EQ(roots.size(), 2u);
    const auto axis = imaginaryAxisFrequencies(roots);
    ASSERT_EQ(axis.size(), 1u);
    EXPECT_NEAR(axis[0], std::sqrt(430.25), 1e-9);
    EXPECT_EQ(rightHalfPlaneCount(roots), 0);

    const auto twin = polynomialCoefficients([](std::complex<double> s) { return s * s - 430.25; });
    ASSERT_TRUE(twin.has_value());
    EXPECT_EQ(rightHalfPlaneCount(polynomialRoots(*twin)), 1);
    EXPECT_TRUE(imaginaryAxisFrequencies(polynomialRoots(*twin)).empty());
}

TEST(Polynomial, WhatIsNotAPolynomialIsRefused)
{
    EXPECT_FALSE(polynomialCoefficients([](std::complex<double> s) { return s + std::exp(-s); }).has_value());
    EXPECT_FALSE(polynomialCoefficients([](std::complex<double> s) { return 1.0 / (s + 1.0); }).has_value());
    EXPECT_FALSE(polynomialCoefficients([](std::complex<double> s) { return std::sin(s); }).has_value());
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
    const auto coefficients = polynomialCoefficients(
        [](std::complex<double> s) { return s * s * (s * s + 0.02 * s + 1.0); });
    ASSERT_TRUE(coefficients.has_value());

    const auto roots = polynomialRoots(*coefficients);
    ASSERT_EQ(roots.size(), 4u);
    EXPECT_TRUE(imaginaryAxisFrequencies(roots).empty()) << "the origin is not an indentation";
    EXPECT_EQ(rightHalfPlaneCount(roots), 0);

    int nearTheRay = 0;
    for (const std::complex<double> & root : roots) {
        if (std::abs(std::abs(root.imag()) - 0.99995) < 1e-3) {
            ++nearTheRay;
            EXPECT_NEAR(root.real(), -0.01, 1e-6);
        }
    }
    EXPECT_EQ(nearTheRay, 2);
}

TEST(Polynomial, ANonMonicPolynomialIsDividedThroughByItsLeadingCoefficient)
{
    const std::vector<std::complex<double>> roots = qftbx::math::polynomialRoots({0.5, 1.5, 1.0});
    ASSERT_EQ(roots.size(), 2u);
    std::vector<double> real{roots[0].real(), roots[1].real()};
    std::sort(real.begin(), real.end());
    EXPECT_NEAR(real[0], -2.0, 1e-12);
    EXPECT_NEAR(real[1], -1.0, 1e-12);
    EXPECT_NEAR(roots[0].imag(), 0.0, 1e-12);

    const std::vector<std::complex<double>> cubic =
            qftbx::math::polynomialRoots({0.04218110876, 1.01792955, 5.332893779, 9.151434189});
    ASSERT_EQ(cubic.size(), 3u);
    for (const std::complex<double> & root : cubic) {
        EXPECT_LT(root.real(), -3.0) << "the closed loop of the FOPDT example is stable";
    }
}

TEST(Polynomial, HurwitzAgreesWithTheRootsAndRejectsTheAxis)
{
    std::mt19937 generator(7);
    std::uniform_real_distribution<double> coefficient(-3.0, 3.0);
    int hurwitz = 0;
    for (int degree = 1; degree <= 7; ++degree) {
        for (int trial = 0; trial < 400; ++trial) {
            std::vector<double> poly(std::size_t(degree) + 1);
            for (double & c : poly) c = coefficient(generator);
            if (poly.front() == 0.0) continue;
            const std::vector<std::complex<double>> roots = qftbx::math::polynomialRoots(poly);
            double worst = -std::numeric_limits<double>::infinity();
            double largest = 0.0;
            for (const std::complex<double> & r : roots) {
                worst = std::max(worst, r.real());
                largest = std::max(largest, std::abs(r));
            }
            const bool byRoots = worst < -1e-9 * std::max(largest, 1.0);
            EXPECT_EQ(qftbx::math::isHurwitz(poly), byRoots)
                << "degree " << degree << " worst real part " << worst;
            hurwitz += byRoots;
        }
    }
    EXPECT_GT(hurwitz, 50) << "the sample must exercise both verdicts";

    EXPECT_FALSE(qftbx::math::isHurwitz({1.0, 0.0, 1.0})) << "s^2 + 1: a pair on the axis";
    EXPECT_FALSE(qftbx::math::isHurwitz({1.0, 0.0})) << "a pole at the origin";
    EXPECT_TRUE(qftbx::math::isHurwitz({1.0, 2.0, 1.0}));
    EXPECT_TRUE(qftbx::math::isHurwitz({-1.0, -2.0, -1.0})) << "the sign of the leading coefficient does not matter";
    EXPECT_FALSE(qftbx::math::isHurwitz({1.0, 1.0, 1.0, 100.0})) << "the Routh table catches what the signs do not";
}
