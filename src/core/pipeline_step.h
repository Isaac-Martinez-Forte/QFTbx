#ifndef QFTBX_PIPELINE_STEP_H
#define QFTBX_PIPELINE_STEP_H

#include <array>
#include <cstddef>
#include <initializer_list>

namespace qftbx {

/**
 * @brief The seven steps of a QFT design, in the order they depend on each
 * other.
 *
 * The order is the dependency order and it is not arbitrary: everything a
 * step COMPUTES is a function of the steps above it, which is why publishing
 * one drops what was computed from the ones below.
 */
enum class Step {
    Plant,
    Specifications,
    Frequencies,
    Templates,
    Boundaries,
    Controller,
    LoopShaping
};

inline constexpr std::size_t kStepCount = 7;

/**
 * @brief A set of steps.
 *
 * It replaces the std::vector<bool> that said the same thing positionally,
 * read as flags.at(0) through flags.at(6) - with an eighth element that was
 * not a step at all but whether the file carried a contour. A positional
 * container with two meanings in it is exactly the shape this refactor has
 * been removing everywhere else.
 */
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

} // namespace qftbx

#endif // QFTBX_PIPELINE_STEP_H
