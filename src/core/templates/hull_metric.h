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
 * them together. The default is the plane of the plant's response, which is
 * also what a project file that names no plane means; on the Nichols chart
 * distances are in degrees and decibels, weighed against each other by
 * dbPerDegree. The file and the settings name the planes "complex" and
 * "nichols".
 */
namespace qftbx {

enum class HullMetric {
    ComplexPlane,
    Nichols
};

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

inline const char * hullMetricName(HullMetric metric)
{
    return metric == HullMetric::Nichols ? "nichols" : "complex";
}

}

#endif
