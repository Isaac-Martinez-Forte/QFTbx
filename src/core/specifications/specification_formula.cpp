#include "src/core/specifications/specification_formula.h"

namespace qftbx {

namespace {

//The symbols of the literature: the loop L = PC, the plant P, the
//controller C and the prefilter F.
const char kLoop[] = "L";
const char kPlant[] = "P";
const char kController[] = "C";
const char kPrefilter[] = "F";

//1 + L, which every one of them divides by. Without parentheses: it is
//always the denominator of a fraction, and a fraction encloses its halves.
Formula onePlusLoop()
{
    return formula::sum(formula::number("1"), formula::symbol(kLoop));
}

//The closed loop, L/(1+L), which four of the six bound.
Formula closedLoop()
{
    return formula::fraction(formula::symbol(kLoop), onePlusLoop());
}

Formula magnitudeOf(SpecificationType type)
{
    switch (type) {
    //Tracking carries the prefilter, as (1.6) of the thesis writes it. The
    //loop shaping never sees it - see requirementOf() - but the
    //specification the user is stating is the one the literature states.
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

//The lower bound of the tracking band is the one that bounds from below.
const char * signOf(SpecificationType type)
{
    return type == SpecificationType::TrackingLower ? "≥" : "≤";
}

} // namespace

Formula requirementOf(SpecificationType type)
{
    return formula::bars(magnitudeOf(type));
}

Formula requirementOf(SpecificationType type, Formula bound)
{
    return formula::row({requirementOf(type), formula::op(signOf(type)), std::move(bound)});
}

} // namespace qftbx
