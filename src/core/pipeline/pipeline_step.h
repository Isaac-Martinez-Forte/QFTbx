/**
 * @file
 * @brief The steps of a QFT design and sets of them.
 *
 * The seven steps are listed in dependency order, and the order is not
 * arbitrary: what a step computes is a function of the steps above it,
 * which is why publishing one drops what was computed from the ones below.
 * A step set is a small fixed-size collection of flags over them, with
 * membership, count and equality, for saying which steps a file carries or
 * a change touches.
 */

#ifndef QFTBX_PIPELINE_STEP_H
#define QFTBX_PIPELINE_STEP_H

#include <array>
#include <cstddef>
#include <initializer_list>

namespace qftbx {

enum class Step {
    Plant,
    Specifications,
    Frequencies,
    Templates,
    Boundaries,
    Controller,
    LoopShaping
};

inline constexpr std::size_t kStepCount = static_cast<std::size_t>(Step::LoopShaping) + 1;

class StepSet
{
public:
    StepSet() = default;

    StepSet(std::initializer_list<Step> steps)
    {
        for (const Step step : steps) { add(step); }
    }

    void add(Step step) { m_present.at(index(step)) = true; }
    void remove(Step step) { m_present.at(index(step)) = false; }
    bool has(Step step) const { return m_present.at(index(step)); }

    bool empty() const
    {
        for (const bool present : m_present) {
            if (present) { return false; }
        }
        return true;
    }

    std::size_t count() const
    {
        std::size_t total = 0;
        for (const bool present : m_present) {
            if (present) { ++total; }
        }
        return total;
    }

    bool operator==(const StepSet & other) const { return m_present == other.m_present; }
    bool operator!=(const StepSet & other) const { return !(*this == other); }

private:
    static std::size_t index(Step step) { return static_cast<std::size_t>(step); }

    std::array<bool, kStepCount> m_present{};
};

}

#endif
