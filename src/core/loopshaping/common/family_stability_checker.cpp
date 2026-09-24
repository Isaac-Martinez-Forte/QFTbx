/**
 * @file
 * @brief Closing the loop with every plant of the sweep, by the Routh table.
 *
 * The family is the cartesian product of the sweep grids of the plant's
 * uncertain parameters, by name, so a name the plant uses twice takes one
 * value; a parameter without a grid stays at its nominal value. Every
 * member's numerator and denominator are built once. A verdict multiplies
 * them by the candidate's and asks whether the sum is Hurwitz.
 */

#include "src/core/loopshaping/common/family_stability_checker.h"

#include <algorithm>
#include <string>

#include "src/core/math/polynomial.h"

namespace qftbx {

namespace {

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

}

FamilyStabilityChecker::FamilyStabilityChecker(LtiSystem * plant, LtiSystem * controller,
                                               const ParameterGrids & sweep)
{
    if (plant == nullptr || controller == nullptr || sweep.empty()
            || hasDelay(*plant) || hasDelay(*controller)) {
        return;
    }

    m_controller = controller->clone();

    std::vector<std::string> names;
    std::vector<const std::vector<double> *> grids;
    const auto sweptBy = [&](const Parameter & parameter) {
        if (!parameter.isUncertain()) {
            return;
        }
        const auto grid = sweep.find(parameter.name());
        if (grid == sweep.end() || grid->second.empty()
                || std::find(names.begin(), names.end(), parameter.name()) != names.end()) {
            return;
        }
        names.push_back(parameter.name());
        grids.push_back(&grid->second);
    };
    for (const Parameter & parameter : plant->numerator()) { sweptBy(parameter); }
    for (const Parameter & parameter : plant->denominator()) { sweptBy(parameter); }
    sweptBy(plant->gain());

    std::size_t count = 1;
    for (const std::vector<double> * grid : grids) {
        count *= grid->size();
    }

    std::vector<double> numerator = nominalsOf(plant->numerator());
    std::vector<double> denominator = nominalsOf(plant->denominator());
    std::vector<double> digit(names.size());

    const auto valueOf = [&](const Parameter & parameter) {
        for (std::size_t j = 0; j < names.size(); ++j) {
            if (names[j] == parameter.name()) {
                return digit[j];
            }
        }
        return parameter.nominal();
    };

    m_members.reserve(count);
    for (std::size_t member = 0; member < count; ++member) {
        std::size_t rest = member;
        for (std::size_t j = 0; j < grids.size(); ++j) {
            digit[j] = (*grids[j])[rest % grids[j]->size()];
            rest /= grids[j]->size();
        }
        for (std::size_t c = 0; c < numerator.size(); ++c) {
            numerator[c] = valueOf(plant->numerator()[c]);
        }
        for (std::size_t c = 0; c < denominator.size(); ++c) {
            denominator[c] = valueOf(plant->denominator()[c]);
        }

        const std::optional<LtiSystem::Polynomials> polynomials =
                plant->polynomialsAt(numerator, denominator, valueOf(plant->gain()));
        if (!polynomials.has_value()) {
            m_members.clear();
            return;
        }
        m_members.push_back(*polynomials);
    }

    m_usable = !m_members.empty();
}

bool FamilyStabilityChecker::isStable(const PointController & point)
{
    if (!m_usable) {
        return true;
    }

    ++m_statistics.verdicts;

    const std::optional<LtiSystem::Polynomials> loop =
            m_controller->polynomialsAt(point.zeros, point.poles, point.gain);
    if (!loop.has_value()) {
        return true;
    }

    for (const LtiSystem::Polynomials & member : m_members) {
        const std::vector<double> characteristic =
                math::polynomialSum(math::polynomialProduct(member.numerator, loop->numerator),
                                    math::polynomialProduct(member.denominator, loop->denominator));
        if (!math::isHurwitz(characteristic)) {
            return false;
        }
    }

    return true;
}

}
