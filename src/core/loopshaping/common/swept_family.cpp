/**
 * @file
 * @brief Walking the cartesian product of the sweep grids into one
 * polynomial pair per plant of the family.
 */

#include "src/core/loopshaping/common/swept_family.h"

#include <algorithm>
#include <optional>

namespace qftbx {

namespace {

constexpr std::size_t kNotSwept = static_cast<std::size_t>(-1);

}

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

    const auto slotOf = [&](const Parameter & parameter) {
        const auto named = std::find(m_names.begin(), m_names.end(), parameter.name());
        return named == m_names.end() ? kNotSwept : static_cast<std::size_t>(named - m_names.begin());
    };
    std::vector<std::size_t> numeratorSlots, denominatorSlots;
    for (const Parameter & parameter : plant.numerator()) { numeratorSlots.push_back(slotOf(parameter)); }
    for (const Parameter & parameter : plant.denominator()) { denominatorSlots.push_back(slotOf(parameter)); }
    const std::size_t gainSlot = slotOf(plant.gain());

    std::vector<double> numerator = nominalValues(plant.numerator());
    std::vector<double> denominator = nominalValues(plant.denominator());
    std::vector<double> digit;

    m_members.reserve(count);
    for (std::size_t member = 0; member < count; ++member) {
        decode(member, digit);
        for (std::size_t c = 0; c < numerator.size(); ++c) {
            if (numeratorSlots[c] != kNotSwept) {
                numerator[c] = digit[numeratorSlots[c]];
            }
        }
        for (std::size_t c = 0; c < denominator.size(); ++c) {
            if (denominatorSlots[c] != kNotSwept) {
                denominator[c] = digit[denominatorSlots[c]];
            }
        }
        const double gain = gainSlot != kNotSwept ? digit[gainSlot] : plant.gain().nominal();

        const std::optional<LtiSystem::Polynomials> polynomials = plant.polynomialsAt(numerator, denominator, gain);
        if (!polynomials.has_value()) {
            m_members.clear();
            m_state = State::NotRational;
            return;
        }
        m_members.push_back(*polynomials);
    }

    m_state = State::Usable;
}

void SweptFamily::decode(std::size_t index, std::vector<double> & values) const
{
    values.resize(m_grids.size());
    std::size_t rest = index;
    for (std::size_t j = 0; j < m_grids.size(); ++j) {
        values[j] = m_grids[j][rest % m_grids[j].size()];
        rest /= m_grids[j].size();
    }
}

std::vector<std::pair<std::string, double>> SweptFamily::valuesOf(std::size_t index) const
{
    std::vector<double> digit;
    decode(index, digit);
    std::vector<std::pair<std::string, double>> values;
    values.reserve(m_names.size());
    for (std::size_t j = 0; j < m_names.size(); ++j) {
        values.emplace_back(m_names[j], digit[j]);
    }
    return values;
}

}
