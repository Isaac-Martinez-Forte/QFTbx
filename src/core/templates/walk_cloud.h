/**
 * @file
 * @brief The cloud a contour is walked over, prepared apart from the walk.
 *
 * sortedCloud() puts a template in MATLAB's complex order, by magnitude
 * and then by angle, keeping the sweep's order among equal keys, drops the
 * repeated points and projects what is left onto the plane of the walk
 * (projectedCloud). None of it depends on epsilon, so a proposal that
 * walks one cloud at many epsilons prepares it once. cloudParts() finds
 * the epsilon-connected components of the projected points (Gutman, Nordin
 * and Cohen 2007) through a NeighbourGrid of cells a quarter of epsilon
 * wide: each part lists its points in their order, and the parts go
 * rightmost first, the one whose first point comes first on a tie. They are
 * what the template engine does before walking, and what an engine on
 * another processor may hand it instead.
 */

#ifndef QFTBX_WALK_CLOUD_H
#define QFTBX_WALK_CLOUD_H

#include <complex>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "src/core/templates/cloud_set.h"
#include "src/core/templates/hull_metric.h"

namespace qftbx {

ComplexCloud projectedCloud(const ComplexCloud & points, HullMetric metric, double dbPerDegree);

struct SortedCloud
{
    ComplexCloud points;
    ComplexCloud walk;
};

SortedCloud sortedCloud(const ComplexCloud & cloud, HullMetric metric, double dbPerDegree);

inline bool withinReach(const std::complex<double> & candidate, const std::complex<double> & centre, double epsilon)
{
    const double dx = candidate.real() - centre.real();
    const double dy = candidate.imag() - centre.imag();

    return dx * dx + dy * dy <= epsilon * epsilon * (1.0 + 1e-9);
}

class NeighbourGrid
{
public:
    NeighbourGrid(const ComplexCloud & points, double epsilon);

    void candidates(const std::complex<double> & centre, std::vector<std::int32_t> & out) const;

private:
    struct Cell {
        std::int64_t x;
        std::int64_t y;

        bool operator<(const Cell & other) const { return x != other.x ? x < other.x : y < other.y; }
    };

    struct Entry {
        Cell cell;
        std::int32_t index;

        bool operator<(const Entry & other) const
        {
            if (cell < other.cell) return true;
            if (other.cell < cell) return false;
            return index < other.index;
        }
    };

    static constexpr double kCellsPerEpsilon = 4.0;
    static constexpr std::int64_t kReach = 5;
    static constexpr double kMaxCells = 1e9;

    Cell cellOf(const std::complex<double> & point) const;

    std::size_t m_pointCount = 0;
    bool m_indexed = false;
    double m_side = 0.0;
    double m_minRe = 0.0;
    double m_minIm = 0.0;
    std::vector<Entry> m_cells;
};

struct CloudParts
{
    std::vector<std::int32_t> members;
    std::vector<std::size_t> starts;
};

CloudParts cloudParts(const ComplexCloud & walk, double epsilon, const NeighbourGrid & neighbours);

struct PreparedCloud
{
    SortedCloud sorted;
    CloudParts parts;
};

}

#endif
