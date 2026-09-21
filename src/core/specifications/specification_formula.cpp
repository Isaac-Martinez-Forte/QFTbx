/**
 * @file
 * @brief Builds the requirement formulas from the symbols of the literature.
 *
 * The loop, the plant, the controller and the prefilter are the single
 * letters of the literature. The term 1 + L is built without parentheses
 * because it is always the denominator of a fraction, which encloses its
 * halves. Only the lower tracking bound bounds from below; every other
 * slot is an upper bound.
 */

#include "src/core/specifications/specification_formula.h"

namespace qftbx {

namespace {

const char kLoop[] = "L";
const char kPlant[] = "P";
const char kController[] = "C";
const char kPrefilter[] = "F";

Formula onePlusLoop()
{
    return formula::sum(formula::number("1"), formula::symbol(kLoop));
}

Formula closedLoop()
{
    return formula::fraction(formula::symbol(kLoop), onePlusLoop());
}

Formula magnitudeOf(SpecificationType type)
{
    switch (type) {
    case SpecificationType::TrackingLower:
    case SpecificationType::TrackingUpper:
        return formula::product(formula::symbol(kPrefilter), closedLoop());

    case SpecificationType::Stability:
    case SpecificationType::SensorNoise:
        return closedLoop();

    case SpecificationType::OutputDisturbance:
        return formula::fraction(formula::number("1"), onePlusLoop());

    case SpecificationType::InputDisturbance:
        return formula::fraction(formula::symbol(kPlant), onePlusLoop());

    case SpecificationType::ControlEffort:
        return formula::fraction(formula::symbol(kController), onePlusLoop());
    }

    return Formula();
}

const char * signOf(SpecificationType type)
{
    return type == SpecificationType::TrackingLower ? "≥" : "≤";
}

}

Formula requirementOf(SpecificationType type)
{
    return formula::bars(magnitudeOf(type));
}

Formula requirementOf(SpecificationType type, Formula bound)
{
    return formula::row({requirementOf(type), formula::op(signOf(type)), std::move(bound)});
}

}
