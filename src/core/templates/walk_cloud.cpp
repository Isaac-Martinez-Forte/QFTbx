/**
 * @file
 * @brief The order, the projection, the neighbour grid and the parts of a
 * cloud before its walk.
 *
 * The keys of the order are computed once per point and the sort is
 * stable. The parts come from a union-find that hangs the larger root under
 * the smaller, so every part is named by its first point whatever the
 * order the pairs are met in.
 */

#include "src/core/templates/walk_cloud.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "src/core/math/constants.h"

namespace qftbx {

ComplexCloud projectedCloud(const ComplexCloud & points, HullMetric metric, double dbPerDegree)
{
    if (metric == HullMetric::ComplexPlane || points.empty()){
        return points;
    }

    std::vector<double> phases;
    phases.reserve(points.size());
    for (const std::complex<double> & z : points){
        phases.push_back(std::arg(z) * 180.0 / qftbx::math::kPi);
    }

    std::vector<double> sorted = phases;
    std::sort(sorted.begin(), sorted.end());
    double widest = sorted.front() + 360.0 - sorted.back();
    bool shiftBelow = false;
    double cut = 0.0;
    for (std::size_t i = 1; i < sorted.size(); ++i){
        const double gap = sorted[i] - sorted[i - 1];
        if (gap > widest){
            widest = gap;
            cut = sorted[i - 1];
            shiftBelow = true;
        }
    }

    ComplexCloud out;
    out.reserve(points.size());
    for (std::size_t i = 0; i < points.size(); ++i){
        double phase = phases[i];
        if (shiftBelow && phase <= cut){
            phase += 360.0;
        }
        const double magnitude = std::abs(points[i]);
        const double db = magnitude > 0.0 ? 20.0 * std::log10(magnitude) : -400.0;
        out.emplace_back(phase, db / dbPerDegree);
    }
    return out;
}

SortedCloud sortedCloud(const ComplexCloud & cloud, HullMetric metric, double dbPerDegree)
{
    struct Keyed {
        double magnitude;
        double angle;
        std::complex<double> point;
    };

    std::vector<Keyed> keyed;
    keyed.reserve(cloud.size());
    for (const std::complex<double> & z : cloud){
        keyed.push_back({std::abs(z), std::arg(z), z});
    }
    std::stable_sort(keyed.begin(), keyed.end(), [](const Keyed & a, const Keyed & b){
        return a.magnitude != b.magnitude ? a.magnitude < b.magnitude : a.angle < b.angle;
    });

    SortedCloud sorted;
    sorted.points.reserve(keyed.size());
    for (const Keyed & k : keyed){
        if (sorted.points.empty() || !(k.point == sorted.points.back())){
            sorted.points.push_back(k.point);
        }
    }
    sorted.walk = projectedCloud(sorted.points, metric, dbPerDegree);
    return sorted;
}

NeighbourGrid::NeighbourGrid(const ComplexCloud & points, double epsilon)
    : m_pointCount(points.size())
{
    double minRe = std::numeric_limits<double>::infinity();
    double minIm = std::numeric_limits<double>::infinity();
    double maxRe = -std::numeric_limits<double>::infinity();
    double maxIm = -std::numeric_limits<double>::infinity();

    for (const std::complex<double> & point : points){
        minRe = std::min(minRe, point.real());
        maxRe = std::max(maxRe, point.real());
        minIm = std::min(minIm, point.imag());
        maxIm = std::max(maxIm, point.imag());
    }

    const double side = epsilon / kCellsPerEpsilon;
    const bool finite = points.empty() ||
            (std::isfinite(minRe) && std::isfinite(maxRe) && std::isfinite(minIm) && std::isfinite(maxIm));
    const bool indexable = finite && side > 0 && std::isfinite(side) &&
            (maxRe - minRe) / side < kMaxCells && (maxIm - minIm) / side < kMaxCells;

    if (!indexable){
        return;
    }

    m_indexed = true;
    m_side = side;
    m_minRe = minRe;
    m_minIm = minIm;

    m_cells.reserve(points.size());
    for (std::size_t i = 0; i < points.size(); ++i){
        m_cells.push_back({cellOf(points[i]), static_cast<std::int32_t>(i)});
    }

    std::sort(m_cells.begin(), m_cells.end());
}

void NeighbourGrid::candidates(const std::complex<double> & centre, std::vector<std::int32_t> & out) const
{
    out.clear();

    if (!m_indexed){
        for (std::size_t i = 0; i < m_pointCount; ++i){
            out.push_back(static_cast<std::int32_t>(i));
        }
        return;
    }

    const Cell cell = cellOf(centre);

    for (std::int64_t x = cell.x - kReach; x <= cell.x + kReach; ++x){
        const Cell first{x, cell.y - kReach};
        const Cell last{x, cell.y + kReach};

        auto entry = std::lower_bound(m_cells.begin(), m_cells.end(), Entry{first, 0});

        for (; entry != m_cells.end() && !(last < entry->cell); ++entry){
            out.push_back(entry->index);
        }
    }
}

NeighbourGrid::Cell NeighbourGrid::cellOf(const std::complex<double> & point) const
{
    return {static_cast<std::int64_t>(std::floor((point.real() - m_minRe) / m_side)),
            static_cast<std::int64_t>(std::floor((point.imag() - m_minIm) / m_side))};
}

CloudParts cloudParts(const ComplexCloud & walk, double epsilon, const NeighbourGrid & neighbours)
{
    const std::size_t n = walk.size();

    std::vector<std::int32_t> parent(n);
    for (std::size_t i = 0; i < n; ++i){
        parent[i] = static_cast<std::int32_t>(i);
    }

    const auto find = [&parent](std::int32_t x){
        while (parent[static_cast<std::size_t>(x)] != x){
            parent[static_cast<std::size_t>(x)] = parent[static_cast<std::size_t>(parent[static_cast<std::size_t>(x)])];
            x = parent[static_cast<std::size_t>(x)];
        }
        return x;
    };

    std::vector<std::int32_t> nearby;
    for (std::size_t i = 0; i < n; ++i){
        neighbours.candidates(walk[i], nearby);
        std::int32_t ri = find(static_cast<std::int32_t>(i));
        for (const std::int32_t j : nearby){
            if (static_cast<std::size_t>(j) <= i || !withinReach(walk[static_cast<std::size_t>(j)], walk[i], epsilon)){
                continue;
            }
            const std::int32_t rj = find(j);
            if (rj < ri){
                parent[static_cast<std::size_t>(ri)] = rj;
                ri = rj;
            } else if (rj > ri){
                parent[static_cast<std::size_t>(rj)] = ri;
            }
        }
    }

    std::vector<std::vector<std::int32_t>> byRoot(n);
    for (std::size_t i = 0; i < n; ++i){
        byRoot[static_cast<std::size_t>(find(static_cast<std::int32_t>(i)))].push_back(static_cast<std::int32_t>(i));
    }

    struct Part {
        double rightmost;
        std::vector<std::int32_t> members;
    };
    std::vector<Part> parts;
    for (std::vector<std::int32_t> & members : byRoot){
        if (members.empty()){
            continue;
        }
        double rightmost = -std::numeric_limits<double>::infinity();
        for (const std::int32_t index : members){
            rightmost = std::max(rightmost, walk[static_cast<std::size_t>(index)].real());
        }
        parts.push_back({rightmost, std::move(members)});
    }
    std::stable_sort(parts.begin(), parts.end(), [](const Part & a, const Part & b){
        return a.rightmost > b.rightmost;
    });

    CloudParts out;
    out.members.reserve(n);
    out.starts.reserve(parts.size());
    for (const Part & part : parts){
        out.starts.push_back(out.members.size());
        out.members.insert(out.members.end(), part.members.begin(), part.members.end());
    }
    return out;
}

}
