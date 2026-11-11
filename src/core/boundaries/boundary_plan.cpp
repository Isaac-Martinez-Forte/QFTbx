/**
 * @file
 * @brief The checks and the specifications of every frequency.
 */

#include "src/core/boundaries/boundary_plan.h"

#include <algorithm>

#include "src/core/common/exception.h"

namespace qftbx {

std::array<bool, 5> BoundaryPlan::sheetsUsed(std::size_t frequency) const
{
    return {stability.at(frequency) || noise.at(frequency), tracking.at(frequency),
            outputDisturbance.at(frequency), inputDisturbance.at(frequency), controlEffort.at(frequency)};
}

std::vector<SheetReading> BoundaryPlan::readings(std::size_t frequency, double omega) const
{
    std::vector<SheetReading> out;
    if (tracking.at(frequency)) {
        out.push_back({"Tracking", 1, specifications.trackingSpreadDb(omega)});
    }
    const auto bound = [&](bool used, const char * name, std::size_t sheet, SpecificationType type) {
        if (used) {
            out.push_back({name, sheet, specifications.at(type).boundDb(omega)});
        }
    };
    bound(stability.at(frequency), "Stability", 0, SpecificationType::Stability);
    bound(noise.at(frequency), "SensorNoise", 0, SpecificationType::SensorNoise);
    bound(outputDisturbance.at(frequency), "OutputDisturbance", 2, SpecificationType::OutputDisturbance);
    bound(inputDisturbance.at(frequency), "InputDisturbance", 3, SpecificationType::InputDisturbance);
    bound(controlEffort.at(frequency), "ControlEffort", 4, SpecificationType::ControlEffort);
    return out;
}

BoundaryPlan planBoundaries(const std::vector<double> & omega, const SpecificationRecords & specifications,
                            Range phaseRange, std::int32_t phaseCount,
                            Range magnitudeRange, std::int32_t magnitudeCount)
{
    BoundaryPlan plan;
    plan.specifications = toSpecificationSet(specifications);

    if (phaseCount < 2 || magnitudeCount < 2) {
        throw InvalidInput(QFTBX_TR("Core", "The Nichols grid needs at least two points on each axis."));
    }
    if (!(phaseRange.width() > 0.0) || !(magnitudeRange.width() > 0.0)) {
        throw InvalidInput(QFTBX_TR("Core", "The Nichols grid needs a non-empty phase range and magnitude range."));
    }

    const SpecificationSet & set = plan.specifications;
    for (const double o : omega) {
        plan.tracking.push_back(set.at(SpecificationType::TrackingLower).appliesAt(o));
        plan.stability.push_back(set.at(SpecificationType::Stability).appliesAt(o));
        plan.noise.push_back(set.at(SpecificationType::SensorNoise).appliesAt(o));
        plan.outputDisturbance.push_back(set.at(SpecificationType::OutputDisturbance).appliesAt(o));
        plan.inputDisturbance.push_back(set.at(SpecificationType::InputDisturbance).appliesAt(o));
        plan.controlEffort.push_back(set.at(SpecificationType::ControlEffort).appliesAt(o));
    }

    if (std::find(plan.tracking.begin(), plan.tracking.end(), true) != plan.tracking.end()
            && !set.at(SpecificationType::TrackingUpper).used()) {
        throw InvalidInput(QFTBX_TR("Core", "The tracking boundary needs both tracking "
                                            "specifications (T_L and T_U)."));
    }

    return plan;
}

}
