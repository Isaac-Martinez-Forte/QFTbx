#ifndef QFTBX_EXACT_POINT_CHECK_H
#define QFTBX_EXACT_POINT_CHECK_H

#include <complex>
#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

#include "src/core/loopshaping/common/point_controller.h"
#include "src/core/loopshaping/common/specification_checker.h"
#include "src/core/specifications/specification.h"
#include "src/core/system/lti_system.h"
#include "src/core/templates/cloud_set.h"

/**
 * @file
 * @brief The verifier's specification criterion, asked of one candidate of
 * the search at a time.
 *
 * The searches decide about single controllers with the boundaries: the
 * point's degenerate box is projected onto the Nichols chart and read
 * against the columns of the phase grid. That reading is not the
 * specification. Between two phase nodes a boundary can fall tens of
 * decibels - a type-1 loop sits at the notch of its low-frequency tracking
 * bound, 33 dB deep inside one degree - so the nearest node lets designs
 * through that violate the bound, and the two bracketing nodes together
 * refuse designs that meet it by twenty decibels. With the zeros and poles
 * fixed the loop has an exact phase and magnitude at every design
 * frequency, and the specification can be evaluated there directly, over
 * the whole value set, as the verifier does on the returned design.
 *
 * This asks exactly that. It holds the SpecificationReference of the
 * problem and a clone of the controller structure, evaluates the loop at
 * each design frequency through the same valueAt the verifier reaches on
 * the returned system, and admits a point only when every excess is at or
 * below minus the tolerance, 1e-12 dB: the verifier accepts an excess of
 * zero, and a candidate put on the very edge of a bound by a root formula
 * must not be returned to be refused by a rounding. So whatever is admitted
 * here the verifier accepts, on the specifications, and the two cannot
 * disagree by an inlined copy of the comparison, because there is one.
 * Exact means exact with respect to the sampled value set, the verifier's
 * own reference: the sampling of the family stays the input.
 *
 * A rejection leaves at the first frequency that fails, and the frequency
 * that failed last is asked first next time, since the candidates of a
 * search resemble one another; an admission always walks every plant of
 * every frequency. A project whose templates do not cover every design
 * frequency has no reference: the check is then unusable and the caller
 * keeps to the columns, as the verifier reports such a design unverified.
 * checkOf gives the full list of a point's excesses, for reporting and for
 * the test that pins the identity with the verifier; the family is not its
 * business.
 */
namespace qftbx {

class ExactPointCheck
{
public:
    static constexpr double kToleranceDb = 1e-12;

    ExactPointCheck(LtiSystem & plant, LtiSystem * controller, const std::vector<double> & omega,
                    const CloudSet & templates, const SpecificationSet & specifications);

    bool usable() const { return m_reference.has_value(); }

    bool admits(const PointController & point);

    SpecificationCheck checkOf(const PointController & point) const;

    struct Statistics {
        std::size_t verdicts = 0;
        std::size_t rejections = 0;
        std::size_t kernelPasses = 0;
    };
    const Statistics & statistics() const { return m_statistics; }

private:
    std::complex<double> loopAt(const FrequencyReference & at, const PointController & point) const;

    void requireUsable() const;

    std::unique_ptr<LtiSystem> m_controller;
    std::optional<SpecificationReference> m_reference;
    std::size_t m_firstToAsk = 0;
    Statistics m_statistics;
};

}

#endif
