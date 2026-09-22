/**
 * @file
 * @brief Checking a designed controller against every specification.
 *
 * At each design frequency the nominal loop is evaluated and the five
 * closed-loop magnitudes are bounded in the worst case over the family at
 * that loop value. The excess over a bound is in decibels, and a value that
 * is not finite violates by an infinite amount, so it can never read as
 * satisfied. The tracking band is governed by its lower bound, the upper one
 * only sets the cut height. The family is walked as the cartesian product of
 * the sweep grids of the plant's uncertain parameters, by name, so a name
 * the plant uses twice takes one value; every member's characteristic
 * polynomial is built from the plant's and the controller's numerator and
 * denominator and its roots counted in the right half-plane.
 */

#include "src/core/loopshaping/common/specification_checker.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <limits>

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

std::vector<double> nominalsOf(std::vector<Parameter> & parameters)
{
    std::vector<double> values;
    values.reserve(parameters.size());
    for (const Parameter & parameter : parameters) {
        values.push_back(parameter.nominal());
    }
    return values;
}

bool hasDelay(LtiSystem & system)
{
    return system.delay().isUncertain() || system.delay().nominal() != 0.0;
}

FamilyStability familyStability(LtiSystem & controller, LtiSystem & plant, const ParameterGrids * sweep)
{
    FamilyStability family;

    if (sweep == nullptr || sweep->empty()) {
        family.notChecked = FamilyStability::NotChecked::NoSweepRecord;
        return family;
    }
    if (hasDelay(plant) || hasDelay(controller)) {
        family.notChecked = FamilyStability::NotChecked::Delay;
        return family;
    }

    const std::optional<LtiSystem::Polynomials> loop =
            controller.polynomialsAt(nominalsOf(controller.numerator()), nominalsOf(controller.denominator()),
                                     controller.gain().nominal());
    if (!loop.has_value()) {
        family.notChecked = FamilyStability::NotChecked::NotRational;
        return family;
    }

    std::vector<std::string> names;
    std::vector<const std::vector<double> *> grids;
    const auto sweptBy = [&](const Parameter & parameter) {
        if (!parameter.isUncertain()) {
            return;
        }
        const auto grid = sweep->find(parameter.name());
        if (grid == sweep->end() || grid->second.empty()
                || std::find(names.begin(), names.end(), parameter.name()) != names.end()) {
            return;
        }
        names.push_back(parameter.name());
        grids.push_back(&grid->second);
    };
    for (const Parameter & parameter : plant.numerator()) { sweptBy(parameter); }
    for (const Parameter & parameter : plant.denominator()) { sweptBy(parameter); }
    sweptBy(plant.gain());

    std::size_t members = 1;
    for (const std::vector<double> * grid : grids) {
        members *= grid->size();
    }

    std::vector<double> numerator = nominalsOf(plant.numerator());
    std::vector<double> denominator = nominalsOf(plant.denominator());
    double gain = plant.gain().nominal();
    std::vector<double> digit(names.size());

    const auto valueOf = [&](const Parameter & parameter, double nominal) {
        for (std::size_t j = 0; j < names.size(); ++j) {
            if (names[j] == parameter.name()) {
                return digit[j];
            }
        }
        return nominal;
    };

    for (std::size_t member = 0; member < members; ++member) {
        std::size_t rest = member;
        for (std::size_t j = 0; j < grids.size(); ++j) {
            digit[j] = (*grids[j])[rest % grids[j]->size()];
            rest /= grids[j]->size();
        }
        for (std::size_t c = 0; c < numerator.size(); ++c) {
            numerator[c] = valueOf(plant.numerator()[c], plant.numerator()[c].nominal());
        }
        for (std::size_t c = 0; c < denominator.size(); ++c) {
            denominator[c] = valueOf(plant.denominator()[c], plant.denominator()[c].nominal());
        }
        gain = valueOf(plant.gain(), plant.gain().nominal());

        const std::optional<LtiSystem::Polynomials> member_ = plant.polynomialsAt(numerator, denominator, gain);
        if (!member_.has_value()) {
            family.notChecked = FamilyStability::NotChecked::NotRational;
            return family;
        }

        const std::vector<double> characteristic =
                math::polynomialSum(math::polynomialProduct(member_->numerator, loop->numerator),
                                    math::polynomialProduct(member_->denominator, loop->denominator));
        const std::vector<std::complex<double>> roots = math::polynomialRoots(characteristic);

        double realPart = -std::numeric_limits<double>::infinity();
        for (const std::complex<double> & root : roots) {
            realPart = std::max(realPart, root.real());
        }
        if (math::rightHalfPlaneCount(roots) > 0) {
            ++family.unstableMembers;
        }
        if (realPart > family.worstRealPart) {
            family.worstRealPart = realPart;
            family.worstMember.clear();
            for (std::size_t j = 0; j < names.size(); ++j) {
                family.worstMember.emplace_back(names[j], digit[j]);
            }
        }
    }

    family.checked = true;
    family.notChecked = FamilyStability::NotChecked::No;
    family.members = members;
    return family;
}

}

SpecificationCheck checkAgainstSpecifications(LtiSystem & controller, LtiSystem & plant,
                                              const std::vector<double> & omega,
                                              const CloudSet & templates,
                                              const SpecificationSet & specifications,
                                              const ParameterGrids * sweep)
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

    check.family = familyStability(controller, plant, sweep);

    return check;
}

}
