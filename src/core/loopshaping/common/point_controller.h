#ifndef QFTBX_LOOPSHAPING_POINT_CONTROLLER_H
#define QFTBX_LOOPSHAPING_POINT_CONTROLLER_H

#include <memory>
#include <vector>

#include "src/core/system/lti_system.h"

/**
 * @brief One controller of a zero-pole-gain box: the gain and the values of
 * its zeros and poles, nothing else.
 *
 * The searches ask questions about single controllers all the time: is this
 * corner of the box feasible, is it nominally stable, what is its loop at
 * this frequency. Every such question used to build a whole LtiSystem for
 * the point (a name, two parameter vectors, a formatted name per value) and
 * throw it away a moment later. The projection and the stability check
 * accept this record directly and compute exactly what they compute for the
 * equivalent system; pointFromBox() still builds the system when a point
 * leaves the search as a result.
 */
namespace qftbx {

struct PointController
{
    double gain = 0.0;
    std::vector<double> zeros;
    std::vector<double> poles;
};

/// The corner of a box that pointFromBox() takes: the lower corner of every
/// parameter when 'lower' is true, otherwise the maximum gain and zeros with
/// the poles at their minimum (the anti-blocking rule). Fixed parameters
/// contribute their nominal value.
inline PointController cornerOf(LtiSystem * box, bool lower)
{
    PointController point;

    point.zeros.reserve(box->numerator().size());
    for (Parameter & v : box->numerator()) {
        point.zeros.push_back(!v.isUncertain() ? v.nominal()
                              : (lower ? v.range().min : v.range().max));
    }

    point.poles.reserve(box->denominator().size());
    for (Parameter & v : box->denominator()) {
        point.poles.push_back(v.isUncertain() ? v.range().min : v.nominal());
    }

    point.gain = lower ? box->gain().range().min : box->gain().range().max;

    return point;
}

/// The point as a system of the prototype's structure, every value a fixed
/// parameter: what pointFromBox() builds, from the values instead of the box.
inline std::unique_ptr<LtiSystem> systemFromPoint(LtiSystem * prototype, const PointController & point)
{
    std::vector<Parameter> numerator;
    numerator.reserve(point.zeros.size());
    for (const double z : point.zeros) {
        numerator.emplace_back(z);
    }

    std::vector<Parameter> denominator;
    denominator.reserve(point.poles.size());
    for (const double p : point.poles) {
        denominator.emplace_back(p);
    }

    return prototype->create(prototype->name(), std::move(numerator), std::move(denominator),
                             Parameter(point.gain), prototype->delay());
}

} // namespace qftbx

#endif // QFTBX_LOOPSHAPING_POINT_CONTROLLER_H
