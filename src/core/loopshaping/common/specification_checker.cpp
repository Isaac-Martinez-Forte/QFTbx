/**
 * @file
 * @brief Checking a designed controller against every specification.
 *
 * At each design frequency the nominal loop is evaluated and the five
 * closed-loop magnitudes are bounded in the worst case over the family at
 * that loop value. The excess over a bound is in decibels, and a value that
 * is not finite violates by an infinite amount, so it can never read as
 * satisfied. The tracking band is governed by its lower bound, the upper one
 * only sets the cut height.
 */

#include "src/core/loopshaping/common/specification_checker.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <limits>

#include "src/core/boundaries/closed_loop_worst_case.h"
#include "src/core/common/exception.h"

namespace qftbx {

namespace {

double excessOf(double valueDb, double boundDb)
{
    if (std::isnan(valueDb)) {
        return std::numeric_limits<double>::infinity();
    }

    return valueDb - boundDb;
}

void record(SpecificationCheck & check, std::size_t index, double omega, SpecificationType type,
            double valueDb, double boundDb)
{
    const double excess = excessOf(valueDb, boundDb);

    check.entries.push_back({index, omega, type, valueDb, boundDb, excess});
    check.worstExcessDb = std::max(check.worstExcessDb, excess);
}

}

SpecificationCheck checkAgainstSpecifications(LtiSystem & controller, LtiSystem & plant,
                                              const std::vector<double> & omega,
                                              const CloudSet & templates,
                                              const SpecificationSet & specifications)
{
    if (templates.size() < omega.size()) {
        throw InvalidInput(QFTBX_TR("Core", "The specification check needs a value set for every design frequency: %1 given for %2 frequencies.")
                           .arg(templates.size()).arg(omega.size()));
    }

    const Specification & trackingLower = specifications.at(SpecificationType::TrackingLower);
    const Specification & trackingUpper = specifications.at(SpecificationType::TrackingUpper);
    const Specification & stability = specifications.at(SpecificationType::Stability);
    const Specification & sensorNoise = specifications.at(SpecificationType::SensorNoise);
    const Specification & outputDisturbance = specifications.at(SpecificationType::OutputDisturbance);
    const Specification & inputDisturbance = specifications.at(SpecificationType::InputDisturbance);
    const Specification & controlEffort = specifications.at(SpecificationType::ControlEffort);

    SpecificationCheck check;

    for (std::size_t i = 0; i < omega.size(); ++i) {
        const double w = omega[i];
        const ComplexCloud & valueSet = templates[i];

        if (valueSet.empty()) {
            continue;
        }

        const std::complex<double> p0 = plant.evaluate(w);
        const std::complex<double> L0 = controller.evaluate(w) * p0;
        const WorstCase worst = worstCaseAt(p0, L0, valueSet, nominalOverValueSet(p0, valueSet));

        if (trackingLower.appliesAt(w)) {
            if (!trackingUpper.used()) {
                throw InvalidInput(QFTBX_TR("Core", "The tracking check needs both tracking specifications (T_L and T_U)."));
            }
            record(check, i, w, SpecificationType::TrackingLower,
                   linearToDb(worst.stabilityNoise) - linearToDb(worst.trackingMin),
                   specifications.trackingSpreadDb(w));
        }
        if (stability.appliesAt(w)) {
            record(check, i, w, SpecificationType::Stability,
                   linearToDb(worst.stabilityNoise), stability.boundDb(w));
        }
        if (sensorNoise.appliesAt(w)) {
            record(check, i, w, SpecificationType::SensorNoise,
                   linearToDb(worst.stabilityNoise), sensorNoise.boundDb(w));
        }
        if (outputDisturbance.appliesAt(w)) {
            record(check, i, w, SpecificationType::OutputDisturbance,
                   linearToDb(worst.outputDisturbance), outputDisturbance.boundDb(w));
        }
        if (inputDisturbance.appliesAt(w)) {
            record(check, i, w, SpecificationType::InputDisturbance,
                   linearToDb(worst.inputDisturbance), inputDisturbance.boundDb(w));
        }
        if (controlEffort.appliesAt(w)) {
            record(check, i, w, SpecificationType::ControlEffort,
                   linearToDb(worst.controlEffort), controlEffort.boundDb(w));
        }
    }

    return check;
}

}
