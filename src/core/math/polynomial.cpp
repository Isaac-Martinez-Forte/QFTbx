/**
 * @file
 * @brief Root finding and coefficient recovery for real polynomials.
 *
 * Roots come from Aberth-Ehrlich iteration on the monic polynomial, started
 * on a circle inside Cauchy's bound, after trailing zero coefficients are
 * taken as exact roots at the origin. Coefficients that are rounding noise
 * beside the largest one are zeroed first: a polynomial recovered from
 * values otherwise reads a double root at the origin as a pair a little off
 * it. Coefficient recovery samples the function on two circles and accepts
 * only when both agree, which is what separates a polynomial from an entire
 * function such as a delay.
 */

#include "src/core/math/polynomial.h"

#include <algorithm>
#include <cmath>

#include "src/core/math/constants.h"

namespace qftbx {
namespace math {

namespace {

void evaluateWithDerivative(const std::vector<double> & monic, std::complex<double> z,
                            std::complex<double> & p, std::complex<double> & dp)
{
    p = std::complex<double>(1.0, 0.0);
    dp = std::complex<double>(0.0, 0.0);
    for (std::size_t k = 1; k < monic.size(); ++k) {
        dp = dp * z + p;
        p = p * z + monic[k];
    }
}

}

std::vector<std::complex<double>> polynomialRoots(const std::vector<double> & coefficients)
{
    std::vector<double> a = coefficients;

    double largest = 0.0;
    for (const double c : a) {
        largest = std::max(largest, std::abs(c));
    }
    for (double & c : a) {
        if (std::abs(c) <= 1e-13 * largest) {
            c = 0.0;
        }
    }

    while (a.size() > 1 && a.front() == 0.0) {
        a.erase(a.begin());
    }

    std::vector<std::complex<double>> roots;
    if (a.size() < 2) {
        return roots;
    }

    while (a.size() > 1 && a.back() == 0.0) {
        a.pop_back();
        roots.emplace_back(0.0, 0.0);
    }

    const std::size_t degree = a.size() - 1;
    if (degree == 0) {
        return roots;
    }

    const double leading = a.front();
    for (double & c : a) {
        c /= leading;
    }
    a.front() = 1.0;

    if (degree == 1) {
        roots.emplace_back(-a[1], 0.0);
        return roots;
    }

    double bound = 0.0;
    for (std::size_t k = 1; k < a.size(); ++k) {
        bound = std::max(bound, std::abs(a[k]));
    }
    bound += 1.0;

    std::vector<std::complex<double>> z(degree);
    for (std::size_t j = 0; j < degree; ++j) {
        const double angle = 2.0 * kPi * static_cast<double>(j) / static_cast<double>(degree) + 0.4;
        z[j] = std::polar(bound, angle);
    }

    for (int iteration = 0; iteration < 500; ++iteration) {
        double largestStep = 0.0;

        for (std::size_t j = 0; j < degree; ++j) {
            std::complex<double> p, dp;
            evaluateWithDerivative(a, z[j], p, dp);

            if (dp == std::complex<double>(0.0, 0.0)) {
                z[j] += std::complex<double>(1e-8 * bound, 1e-8 * bound);
                largestStep = std::max(largestStep, 1.0);
                continue;
            }

            const std::complex<double> newton = p / dp;

            std::complex<double> repulsion(0.0, 0.0);
            for (std::size_t k = 0; k < degree; ++k) {
                if (k != j) {
                    repulsion += 1.0 / (z[j] - z[k]);
                }
            }

            const std::complex<double> step = newton / (1.0 - newton * repulsion);
            z[j] -= step;
            largestStep = std::max(largestStep, std::abs(step) / (1.0 + std::abs(z[j])));
        }

        if (largestStep < 1e-15) {
            break;
        }
    }

    for (std::complex<double> & root : z) {
        if (std::abs(root.imag()) <= 1e-10 * (1.0 + std::abs(root.real()))) {
            root = std::complex<double>(root.real(), 0.0);
        }
        roots.push_back(root);
    }

    return roots;
}

std::optional<std::vector<double>> polynomialCoefficients(
        const std::function<std::complex<double>(std::complex<double>)> & value, int maxDegree)
{
    constexpr int kSamples = 64;
    if (maxDegree < 0 || 2 * (maxDegree + 1) > kSamples) {
        return std::nullopt;
    }

    const auto pass = [&](double radius, std::vector<double> & coefficients) -> bool {
        std::vector<std::complex<double>> samples(kSamples);
        for (int j = 0; j < kSamples; ++j) {
            const std::complex<double> s = std::polar(radius, 2.0 * kPi * j / kSamples);
            samples[j] = value(s);
            if (!std::isfinite(samples[j].real()) || !std::isfinite(samples[j].imag())) {
                return false;
            }
        }

        std::vector<std::complex<double>> scaled(kSamples);
        double largest = 0.0;
        for (int k = 0; k < kSamples; ++k) {
            std::complex<double> sum(0.0, 0.0);
            for (int j = 0; j < kSamples; ++j) {
                sum += samples[j] * std::polar(1.0, -2.0 * kPi * j * k / kSamples);
            }
            scaled[k] = sum / static_cast<double>(kSamples);
            largest = std::max(largest, std::abs(scaled[k]));
        }

        if (largest == 0.0) {
            coefficients.assign(1, 0.0);
            return true;
        }

        for (int k = maxDegree + 1; k < kSamples; ++k) {
            if (std::abs(scaled[k]) > 1e-7 * largest) {
                return false;
            }
        }
        for (int k = 0; k <= maxDegree; ++k) {
            if (std::abs(scaled[k].imag()) > 1e-7 * largest) {
                return false;
            }
        }

        int degree = 0;
        for (int k = maxDegree; k >= 0; --k) {
            if (std::abs(scaled[k].real()) > 1e-9 * largest) {
                degree = k;
                break;
            }
        }

        coefficients.assign(static_cast<std::size_t>(degree) + 1, 0.0);
        for (int k = 0; k <= degree; ++k) {
            const double real = scaled[k].real();
            coefficients[static_cast<std::size_t>(degree - k)] =
                std::abs(real) > 1e-9 * largest ? real / std::pow(radius, k) : 0.0;
        }
        return true;
    };

    std::vector<double> coefficients;
    if (!pass(1.0, coefficients)) {
        return std::nullopt;
    }

    double largestRoot = 0.0;
    for (const std::complex<double> & root : polynomialRoots(coefficients)) {
        largestRoot = std::max(largestRoot, std::abs(root));
    }
    const double radius = std::max(4.0, 4.0 * largestRoot);

    std::vector<double> wider;
    if (!pass(radius, wider) || wider.size() != coefficients.size()) {
        return std::nullopt;
    }

    double largestScaled = 0.0;
    const std::size_t degree = wider.size() - 1;
    for (std::size_t k = 0; k <= degree; ++k) {
        largestScaled = std::max(largestScaled, std::abs(wider[degree - k]) * std::pow(radius, static_cast<double>(k)));
    }
    for (std::size_t k = 0; k <= degree; ++k) {
        const double difference = std::abs(wider[degree - k] - coefficients[degree - k]) * std::pow(radius, static_cast<double>(k));
        if (difference > 1e-7 * largestScaled) {
            return std::nullopt;
        }
    }

    coefficients = std::move(wider);
    return coefficients;
}

namespace {

bool onTheAxis(const std::complex<double> & root, double largest)
{
    return std::abs(root.real()) <= 1e-7 * std::max(largest, std::abs(root));
}

double largestMagnitude(const std::vector<std::complex<double>> & roots)
{
    double largest = 1.0;
    for (const std::complex<double> & root : roots) {
        largest = std::max(largest, std::abs(root));
    }
    return largest;
}

}

int rightHalfPlaneCount(const std::vector<std::complex<double>> & roots)
{
    const double largest = largestMagnitude(roots);
    int count = 0;
    for (const std::complex<double> & root : roots) {
        if (!onTheAxis(root, largest) && root.real() > 0.0) {
            ++count;
        }
    }
    return count;
}

std::vector<double> imaginaryAxisFrequencies(const std::vector<std::complex<double>> & roots)
{
    const double largest = largestMagnitude(roots);
    std::vector<double> frequencies;
    for (const std::complex<double> & root : roots) {
        if (onTheAxis(root, largest) && root.imag() > 1e-9 * largest) {
            frequencies.push_back(root.imag());
        }
    }
    std::sort(frequencies.begin(), frequencies.end());
    return frequencies;
}

bool isHurwitz(const std::vector<double> & coefficients)
{
    std::vector<double> a = coefficients;
    while (!a.empty() && a.front() == 0.0) {
        a.erase(a.begin());
    }
    if (a.size() < 2) {
        return !a.empty();
    }

    const double sign = a.front() < 0.0 ? -1.0 : 1.0;
    for (double & c : a) {
        c *= sign;
        if (c <= 0.0) {
            return false;
        }
    }

    const std::size_t degree = a.size() - 1;
    std::vector<double> previous((degree + 2) / 2, 0.0);
    std::vector<double> current((degree + 2) / 2, 0.0);
    for (std::size_t i = 0; i * 2 < a.size(); ++i) {
        previous[i] = a[i * 2];
    }
    for (std::size_t i = 0; i * 2 + 1 < a.size(); ++i) {
        current[i] = a[i * 2 + 1];
    }

    std::vector<double> next(previous.size(), 0.0);
    for (std::size_t row = 2; row <= degree; ++row) {
        if (current.front() <= 0.0) {
            return false;
        }
        for (std::size_t i = 0; i + 1 < previous.size(); ++i) {
            next[i] = current[0] * previous[i + 1] - previous[0] * current[i + 1];
            next[i] /= current[0];
        }
        next.back() = 0.0;
        previous.swap(current);
        current.swap(next);
    }

    return current.front() > 0.0;
}

std::vector<double> polynomialProduct(const std::vector<double> & a, const std::vector<double> & b)
{
    if (a.empty()) {
        return b.empty() ? std::vector<double>{1.0} : b;
    }
    if (b.empty()) {
        return a;
    }
    std::vector<double> product(a.size() + b.size() - 1, 0.0);
    for (std::size_t i = 0; i < a.size(); ++i) {
        for (std::size_t j = 0; j < b.size(); ++j) {
            product[i + j] += a[i] * b[j];
        }
    }
    return product;
}

std::vector<double> polynomialSum(const std::vector<double> & a, const std::vector<double> & b)
{
    const std::vector<double> & longer = a.size() >= b.size() ? a : b;
    const std::vector<double> & shorter = a.size() >= b.size() ? b : a;
    std::vector<double> sum = longer;
    const std::size_t offset = longer.size() - shorter.size();
    for (std::size_t i = 0; i < shorter.size(); ++i) {
        sum[offset + i] += shorter[i];
    }
    return sum;
}

}

}
