#ifndef QFTBX_SWEPT_FAMILY_H
#define QFTBX_SWEPT_FAMILY_H

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "src/core/system/lti_system.h"
#include "src/core/templates/parameter_grids.h"

/**
 * @file
 * @brief The plants of the swept family as polynomials, built once for
 * everything that closes the loop with each of them.
 *
 * The family is the cartesian product of the sweep grids of the plant's
 * uncertain parameters, by name, so a name the plant uses twice takes one
 * value and a parameter without a grid stays at its nominal value; the
 * members come in the order of that product, the first grid varying fastest,
 * which is the order the templates were swept in. Every member's numerator
 * and denominator are built here, and the verifier and the searches' gate
 * ask for them by index, so the two walk the same plants with the same
 * coefficients and can only differ in the criterion they apply.
 *
 * A family is usable only when there is a record of the sweep, the plant has
 * no delay and every member is a rational function; why it is not is kept,
 * because the verifier reports it. valuesOf gives the parameter values a
 * member was built from, for naming the worst plant.
 */
namespace qftbx {



class SweptFamily
{
public:
    enum class State { Usable, NoSweepRecord, Delay, NotRational };

    SweptFamily() = default;
    SweptFamily(LtiSystem & plant, const ParameterGrids & sweep);

    State state() const { return m_state; }
    bool usable() const { return m_state == State::Usable; }

    std::size_t size() const { return m_members.size(); }
    const LtiSystem::Polynomials & member(std::size_t index) const { return m_members[index]; }

    std::vector<std::pair<std::string, double>> valuesOf(std::size_t index) const;

private:
    void decode(std::size_t index, std::vector<double> & values) const;

    State m_state = State::NoSweepRecord;
    std::vector<std::string> m_names;
    std::vector<std::vector<double>> m_grids;
    std::vector<LtiSystem::Polynomials> m_members;
};

}

#endif
