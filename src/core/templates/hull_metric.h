#ifndef QFTBX_HULL_METRIC_H
#define QFTBX_HULL_METRIC_H

#include <string>

/**
 * @file
 * @brief The plane the epsilon of the template contour is measured in.
 *
 * The walk of the epsilon-hull only ever asks how far apart two points are,
 * so the plane those distances are taken in is a choice, and it decides
 * whether one epsilon can serve a whole template (see TemplateEngine). The
 * choice belongs with the epsilon: an epsilon without its plane is a number
 * without a unit, so the project keeps the two together and the file stores
 * them together.
 */
namespace qftbx {

enum class HullMetric {
    /// Distances in the plane of the plant's response: the historical
    /// reading, and what every project file predating the choice used.
    ComplexPlane,
    /// Distances in degrees and decibels on the Nichols chart, one decibel
    /// weighing as much as so many degrees.
    Nichols
};

/// The epsilon's plane and, for the Nichols plane, its weighting.
struct EpsilonMetric
{
    HullMetric metric = HullMetric::ComplexPlane;
    double dbPerDegree = 1.0;

    bool operator==(const EpsilonMetric & other) const
    {
        return metric == other.metric && dbPerDegree == other.dbPerDegree;
    }
    bool operator!=(const EpsilonMetric & other) const { return !(*this == other); }
};

/// The name the file and the settings use: "complex" or "nichols".
inline const char * hullMetricName(HullMetric metric)
{
    return metric == HullMetric::Nichols ? "nichols" : "complex";
}

} // namespace qftbx

#endif // QFTBX_HULL_METRIC_H
