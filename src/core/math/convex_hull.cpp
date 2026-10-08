/**
 * @file
 * @brief Andrew's monotone chain.
 */

#include "src/core/math/convex_hull.h"

#include <algorithm>
#include <numeric>

namespace qftbx {
namespace math {

namespace {

double turn(const std::complex<double> & o, const std::complex<double> & a, const std::complex<double> & b)
{
    return (a.real() - o.real()) * (b.imag() - o.imag()) - (a.imag() - o.imag()) * (b.real() - o.real());
}

}

std::vector<std::size_t> convexHullVertices(const std::vector<std::complex<double>> & points)
{
    std::vector<std::size_t> order(points.size());
    std::iota(order.begin(), order.end(), std::size_t(0));
    std::sort(order.begin(), order.end(), [&](std::size_t i, std::size_t j) {
        return points[i].real() < points[j].real()
               || (points[i].real() == points[j].real() && points[i].imag() < points[j].imag());
    });
    order.erase(std::unique(order.begin(), order.end(), [&](std::size_t i, std::size_t j) {
        return points[i] == points[j];
    }), order.end());

    if (order.size() < 3) {
        return order;
    }

    std::vector<std::size_t> hull(2 * order.size());
    std::size_t k = 0;

    for (std::size_t i = 0; i < order.size(); ++i) {
        while (k >= 2 && turn(points[hull[k - 2]], points[hull[k - 1]], points[order[i]]) <= 0.0) {
            --k;
        }
        hull[k++] = order[i];
    }

    const std::size_t lower = k + 1;
    for (std::size_t i = order.size() - 1; i-- > 0;) {
        while (k >= lower && turn(points[hull[k - 2]], points[hull[k - 1]], points[order[i]]) <= 0.0) {
            --k;
        }
        hull[k++] = order[i];
    }

    hull.resize(k - 1);
    return hull;
}

}
}
