/**
 * @file
 * @brief Walking the cartesian product of the sweep grids into one
 * polynomial pair per plant of the family.
 */

#include "src/core/loopshaping/common/swept_family.h"

#include <algorithm>
#include <optional>

namespace qftbx {

SweptFamily::SweptFamily(LtiSystem & plant, const ParameterGrids & sweep)
{
    if (sweep.empty()) {
        m_state = State::NoSweepRecord;
        return;
    }
    if (hasDelay(plant)) {
        m_state = State::Delay;
        return;
    }

    const auto sweptBy = [&](const Parameter & parameter) {
        if (!parameter.isUncertain()) {
            return;
        }
        const auto grid = sweep.find(parameter.name());
        if (grid == sweep.end() || grid->second.empty()
                || std::find(m_names.begin(), m_names.end(), parameter.name()) != m_names.end()) {
            return;
        }
        m_names.push_back(parameter.name());
        m_grids.push_back(grid->second);
    };
    for (const Parameter & parameter : plant.numerator()) { sweptBy(parameter); }
    for (const Parameter & parameter : plant.denominator()) { sweptBy(parameter); }
    sweptBy(plant.gain());

    std::size_t count = 1;
    for (const std::vector<double> & grid : m_grids) {
        count *= grid.size();
    }

    std::vector<double> numerator = nominalValues(plant.numerator());
    std::vector<double> denominator = nominalValues(plant.denominator());
    std::vector<double> digit(m_names.size());

    const auto valueOf = [&](const Parameter & parameter) {
        for (std::size_t j = 0; j < m_names.size(); ++j) {
            if (m_names[j] == parameter.name()) {
                return digit[j];
            }
        }
        return parameter.nominal();
    };

    m_members.reserve(count);
    for (std::size_t member = 0; member < count; ++member) {
        std::size_t rest = member;
        for (std::size_t j = 0; j < m_grids.size(); ++j) {
            digit[j] = m_grids[j][rest % m_grids[j].size()];
            rest /= m_grids[j].size();
        }
        for (std::size_t c = 0; c < numerator.size(); ++c) {
            numerator[c] = valueOf(plant.numerator()[c]);
        }
        for (std::size_t c = 0; c < denominator.size(); ++c) {
            denominator[c] = valueOf(plant.denominator()[c]);
        }

        const std::optional<LtiSystem::Polynomials> polynomials =
                plant.polynomialsAt(numerator, denominator, valueOf(plant.gain()));
        if (!polynomials.has_value()) {
            m_members.clear();
            m_state = State::NotRational;
            return;
        }
        m_members.push_back(*polynomials);
    }

    m_state = State::Usable;
}

std::vector<std::pair<std::string, double>> SweptFamily::valuesOf(std::size_t index) const
{
    std::vector<std::pair<std::string, double>> values;
    values.reserve(m_names.size());
    std::size_t rest = index;
    for (std::size_t j = 0; j < m_grids.size(); ++j) {
        values.emplace_back(m_names[j], m_grids[j][rest % m_grids[j].size()]);
        rest /= m_grids[j].size();
    }
    return values;
}

}
