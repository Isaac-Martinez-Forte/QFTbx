/**
 * @file
 * @brief The worst case over a value set found in two passes is the one
 * found by evaluating every plant.
 *
 * Over random value sets of one to seven hundred plants, at every scale and
 * with every mask, the extremes worstCaseAt finds from the squares first are
 * compared, to the bit and with the index of the nearest sample, with those
 * of evaluating every plant. Some sets are built to stress the margin: every
 * plant at the same distance from -L, plants equidistant to within 1e-11, a
 * few plants repeated, a loop value whose square leaves the safe range, a
 * loop near that range with plants packed at its edge, and quotients spread
 * over many decades.
 */

#include <gtest/gtest.h>

#include <cmath>
#include <complex>
#include <cstddef>
#include <cstring>
#include <random>
#include <vector>

#include "src/core/boundaries/closed_loop_worst_case.h"

using namespace qftbx;

namespace {

bool sameBits(double a, double b)
{
    return std::memcmp(&a, &b, sizeof a) == 0;
}

bool same(const WorstCase & a, const WorstCase & b)
{
    return sameBits(a.stabilityNoise, b.stabilityNoise) && sameBits(a.trackingMin, b.trackingMin)
           && sameBits(a.outputDisturbance, b.outputDisturbance) && sameBits(a.inputDisturbance, b.inputDisturbance)
           && sameBits(a.controlEffort, b.controlEffort) && sameBits(a.nearestSample, b.nearestSample)
           && a.nearestIndex == b.nearestIndex;
}

}

TEST(ClosedLoopWorstCase, TheTwoPassesFindTheExtremesOfEveryPlant)
{
    constexpr double kPi = 3.14159265358979323846;
    std::mt19937_64 generator(12345);
    std::uniform_real_distribution<double> unit(-1.0, 1.0);
    std::uniform_real_distribution<double> decades(-6.0, 6.0);

    for (int trial = 0; trial < 21000; ++trial) {
        const int kind = trial % 7;
        const std::size_t n = 1 + generator() % 700;
        const double scale = std::pow(10.0, decades(generator));
        const std::complex<double> p0(unit(generator) * scale, unit(generator) * scale);
        std::complex<double> L(unit(generator) * std::pow(10.0, decades(generator)),
                               unit(generator) * std::pow(10.0, decades(generator)));
        ComplexCloud cloud(n);
        for (std::size_t i = 0; i < n; ++i) {
            cloud[i] = std::complex<double>(unit(generator) * scale * 3.0, unit(generator) * scale * 3.0);
            if (cloud[i] == std::complex<double>(0.0, 0.0)) {
                cloud[i] = 1.0;
            }
        }
        std::vector<std::complex<double>> q = nominalOverValueSet(p0, cloud);

        if (kind == 1) {
            const double radius = std::pow(10.0, decades(generator));
            for (std::size_t i = 0; i < n; ++i) {
                q[i] = -L + std::polar(radius, 2.0 * kPi * static_cast<double>(i) / static_cast<double>(n));
            }
        } else if (kind == 2) {
            for (std::size_t i = 0; i < n; ++i) {
                const double radius = 1.0 + 1e-11 * unit(generator);
                q[i] = -L + std::polar(radius * std::abs(L) * 1e-3 + 1e-300, unit(generator) * kPi);
            }
        } else if (kind == 3) {
            for (std::size_t i = 0; i < n; ++i) {
                q[i] = q[i % 3];
                cloud[i] = cloud[i % 3];
            }
        } else if (kind == 4) {
            L = std::complex<double>(unit(generator) * 1e140, unit(generator) * 1e140);
        } else if (kind == 5) {
            for (std::size_t i = 0; i < n; ++i) {
                q[i] *= std::pow(10.0, 3.0 * decades(generator));
            }
        } else if (kind == 6) {
            L = std::complex<double>(9e144 * (1.0 + 0.01 * unit(generator)), 1e140 * unit(generator));
            for (std::size_t i = 0; i < n; ++i) {
                const double radius = 1e-15 * (1.0 + 4e-16 * static_cast<double>(generator() % 8));
                q[i] = std::polar(radius, unit(generator) * kPi);
            }
        }

        WorstCaseMask mask;
        mask.stabilityNoiseTracking = (generator() & 1) != 0;
        mask.outputDisturbance = kind == 6 || (generator() & 1) != 0;
        mask.inputDisturbance = (generator() & 1) != 0;
        mask.controlEffort = (generator() & 1) != 0;

        const WorstCase twoPasses = worstCaseAt(p0, L, cloud, q, mask);
        const WorstCase everyPlant = detail::worstCaseEverywhere(p0, L, cloud, q, mask);
        EXPECT_TRUE(same(twoPasses, everyPlant)) << "trial " << trial << ", kind " << kind << ", " << n << " plants";
    }
}
