/**
 * @file
 * @brief Checking a designed controller against every specification.
 *
 * At each design frequency the nominal loop is evaluated and the five
 * closed-loop magnitudes are bounded in the worst case over the family at
 * that loop value. The excess over a bound is in decibels, and a value that
 * is not finite violates by an infinite amount, so it can never read as
 * satisfied. The tracking band is governed by its lower bound, the upper one
 * only sets the cut height. Every member of the swept family is closed with
 * the controller's numerator and denominator, and a member is stable only
 * when every root of its characteristic polynomial is strictly in the left
 * half-plane, the tolerance of the axis being the one the roots are
 * computed to.
 */

#include "src/core/loopshaping/common/specification_checker.h"

#include <algorithm>
#include <cmath>
#include <optional>

#include "src/core/boundaries/closed_loop_worst_case.h"
#include "src/core/common/exception.h"
#include "src/core/math/polynomial.h"

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

double valueOf(const WorstCase & worst, SpecificationType type)
{
    switch (type) {
    case SpecificationType::TrackingLower:
        return linearToDb(worst.stabilityNoise) - linearToDb(worst.trackingMin);
    case SpecificationType::Stability:
    case SpecificationType::SensorNoise:
        return linearToDb(worst.stabilityNoise);
    case SpecificationType::OutputDisturbance:
        return linearToDb(worst.outputDisturbance);
    case SpecificationType::InputDisturbance:
        return linearToDb(worst.inputDisturbance);
    case SpecificationType::ControlEffort:
        return linearToDb(worst.controlEffort);
    case SpecificationType::TrackingUpper:
        break;
    }
    return std::numeric_limits<double>::quiet_NaN();
}

FamilyStability::NotChecked reasonOf(SweptFamily::State state)
{
    switch (state) {
    case SweptFamily::State::Delay:       return FamilyStability::NotChecked::Delay;
    case SweptFamily::State::NotRational: return FamilyStability::NotChecked::NotRational;
    case SweptFamily::State::NoSweepRecord:
    case SweptFamily::State::Usable:
        break;
    }
    return FamilyStability::NotChecked::NoSweepRecord;
}

}

SpecificationReference::SpecificationReference(LtiSystem & plant, const std::vector<double> & omega,
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

    m_frequencies.reserve(omega.size());

    for (std::size_t i = 0; i < omega.size(); ++i) {
        const double w = omega[i];
        const ComplexCloud & valueSet = templates[i];

        if (valueSet.empty()) {
            continue;
        }

        FrequencyReference at;
        at.index = i;
        at.omega = w;
        at.nominalPlant = plant.evaluate(w);
        at.valueSet = &valueSet;
        at.nominalOverValueSet = nominalOverValueSet(at.nominalPlant, valueSet);

        if (trackingLower.appliesAt(w)) {
            if (!trackingUpper.used()) {
                throw InvalidInput(QFTBX_TR("Core", "The tracking check needs both tracking specifications (T_L and T_U)."));
            }
            at.bounds.push_back({SpecificationType::TrackingLower, specifications.trackingSpreadDb(w)});
        }
        if (stability.appliesAt(w)) {
            at.bounds.push_back({SpecificationType::Stability, stability.boundDb(w)});
        }
        if (sensorNoise.appliesAt(w)) {
            at.bounds.push_back({SpecificationType::SensorNoise, sensorNoise.boundDb(w)});
        }
        if (outputDisturbance.appliesAt(w)) {
            at.bounds.push_back({SpecificationType::OutputDisturbance, outputDisturbance.boundDb(w)});
        }
        if (inputDisturbance.appliesAt(w)) {
            at.bounds.push_back({SpecificationType::InputDisturbance, inputDisturbance.boundDb(w)});
        }
        if (controlEffort.appliesAt(w)) {
            at.bounds.push_back({SpecificationType::ControlEffort, controlEffort.boundDb(w)});
        }

        m_frequencies.push_back(std::move(at));
    }
}

void SpecificationReference::recordExcesses(const FrequencyReference & at, std::complex<double> loop,
                                            SpecificationCheck & check) const
{
    const WorstCase worst = worstCaseAt(at.nominalPlant, loop, *at.valueSet, at.nominalOverValueSet);

    for (const FrequencyReference::Bound & bound : at.bounds) {
        record(check, at.index, at.omega, bound.type, valueOf(worst, bound.type), bound.boundDb);
    }
}

double SpecificationReference::worstExcessAt(const FrequencyReference & at, std::complex<double> loop) const
{
    const WorstCase worst = worstCaseAt(at.nominalPlant, loop, *at.valueSet, at.nominalOverValueSet);

    double worstExcess = -std::numeric_limits<double>::infinity();
    for (const FrequencyReference::Bound & bound : at.bounds) {
        worstExcess = std::max(worstExcess, excessOf(valueOf(worst, bound.type), bound.boundDb));
    }
    return worstExcess;
}

FamilyStability familyStabilityAt(const SweptFamily & family, const LtiSystem::Polynomials & loop)
{
    FamilyStability result;

    if (!family.usable()) {
        result.notChecked = reasonOf(family.state());
        return result;
    }

    for (std::size_t member = 0; member < family.size(); ++member) {
        const LtiSystem::Polynomials & plant = family.member(member);
        const std::vector<double> characteristic =
                math::polynomialSum(math::polynomialProduct(plant.numerator, loop.numerator),
                                    math::polynomialProduct(plant.denominator, loop.denominator));
        const std::vector<std::complex<double>> roots = math::polynomialRoots(characteristic);

        double realPart = -std::numeric_limits<double>::infinity();
        double largest = 0.0;
        for (const std::complex<double> & root : roots) {
            realPart = std::max(realPart, root.real());
            largest = std::max(largest, std::abs(root));
        }
        if (roots.empty() || realPart > -1e-7 * largest) {
            ++result.unstableMembers;
        }
        if (realPart > result.worstRealPart) {
            result.worstRealPart = realPart;
            result.worstMember = family.valuesOf(member);
        }
    }

    result.checked = true;
    result.notChecked = FamilyStability::NotChecked::No;
    result.members = family.size();
    return result;
}

FamilyStability familyStabilityAt(const SweptFamily & family, LtiSystem & controller)
{
    FamilyStability result;

    if (family.state() == SweptFamily::State::NoSweepRecord) {
        result.notChecked = FamilyStability::NotChecked::NoSweepRecord;
        return result;
    }
    if (family.state() == SweptFamily::State::Delay || hasDelay(controller)) {
        result.notChecked = FamilyStability::NotChecked::Delay;
        return result;
    }

    const std::optional<LtiSystem::Polynomials> loop =
            controller.polynomialsAt(nominalValues(controller.numerator()), nominalValues(controller.denominator()),
                                     controller.gain().nominal());
    if (!loop.has_value()) {
        result.notChecked = FamilyStability::NotChecked::NotRational;
        return result;
    }

    return familyStabilityAt(family, *loop);
}

SpecificationCheck checkAgainstSpecifications(LtiSystem & controller, LtiSystem & plant,
                                              const std::vector<double> & omega,
                                              const CloudSet & templates,
                                              const SpecificationSet & specifications,
                                              const ParameterGrids * sweep)
{
    const SpecificationReference reference(plant, omega, templates, specifications);

    SpecificationCheck check;

    for (const FrequencyReference & at : reference.frequencies()) {
        reference.recordExcesses(at, controller.evaluate(at.omega) * at.nominalPlant, check);
    }

    check.family = familyStabilityAt(SweptFamily(plant, sweep == nullptr ? ParameterGrids() : *sweep), controller);

    return check;
}

}
