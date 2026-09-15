#include "src/core/loopshaping/common/specification_checker.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <limits>

#include "src/core/boundaries/closed_loop_worst_case.h"
#include "src/core/common/exception.h"

namespace qftbx {

namespace {

//The excess of a value over a bound, both in dB. A value that is not finite
//(the loop at the critical point, or a plant value of zero) is stated to
//violate by an infinite amount: a NaN would compare false against every
//bound and read as satisfied.
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

} // namespace

SpecificationCheck checkAgainstSpecifications(LtiSystem & controller, LtiSystem & plant,
                                              const std::vector<double> & omega,
                                              const CloudSet & templates,
                                              const SpecificationSet & specifications)
{
    if (templates.size() < omega.size()) {
        throw InvalidInput(QFTBX_TR("Core", "The specification check needs a value set for every design frequency: %1 given for %2 frequencies.")
                           .arg(templates.size()).arg(omega.size()));
    }

    //As in the boundary computation, the tracking band is governed by T_L
    //and T_U only provides the cut height; asking for a band without T_U is
    //the one inconsistency the records allow.
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

        //The nominal loop at this frequency, and the five magnitudes in the
        //worst case over the family at exactly that loop value.
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

} // namespace qftbx
