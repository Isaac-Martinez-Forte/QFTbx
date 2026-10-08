#ifndef QFTBX_SPECIFICATION_CHECKER_H
#define QFTBX_SPECIFICATION_CHECKER_H

#include <complex>
#include <cstddef>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "src/core/boundaries/closed_loop_worst_case.h"
#include "src/core/loopshaping/common/swept_family.h"
#include "src/core/specifications/specification.h"
#include "src/core/system/lti_system.h"
#include "src/core/templates/cloud_set.h"
#include "src/core/templates/parameter_grids.h"

/**
 * @file
 * @brief A returned controller checked against the specifications
 * themselves, not against the boundaries computed from them.
 *
 * The boundaries approximate, with a phase grid read at its nearest node
 * and a sampled template, and not always on the safe side, so a controller
 * the search certifies against them can still violate its specifications.
 * The checker evaluates the closed-loop magnitudes at the controller's own
 * loop value, at every design frequency and over the whole value set, and
 * compares them with the bounds there: the only approximation left is the
 * sampling of the plant family, which is the input. Each entry gives the
 * value, the bound and the excess in decibels, a positive excess being a
 * violation.
 *
 * Meeting the bounds at the design frequencies does not make the loop
 * stable, so the checker also closes the loop with every plant of the sweep
 * and with the nominal plant, and finds the roots of N_P N_C + D_P D_C. A
 * loop is not asymptotically stable with a pole in the right half-plane or
 * on the imaginary axis within 1e-7 of the largest pole. A delay, a plant
 * that is not rational or a project with no record of its sweep is reported
 * as unchecked, never approved. satisfied() asks for no excess, no unstable
 * plant of the family and, where it was checked, a stable nominal loop.
 *
 * What does not depend on the controller is gathered once in a
 * SpecificationReference: per design frequency, the nominal plant value,
 * the quotients P_0 / P of the value set, the bounds in force in decibels
 * and the closed-loop magnitudes they need. recordExcesses and
 * worstExcessAt are the one place the comparison is written, shared by the
 * verifier and the exact point check (exact_point_check.h). The reference
 * points into the value sets it was built from and must not outlive them.
 * familyStabilityAt and rootVerdictOf are the family criterion on the same
 * terms. A parameter the sweep has no grid for stays at its nominal value.
 */
namespace qftbx {

struct SpecificationExcess
{
    std::size_t frequencyIndex = 0;
    double omega = 0.0;
    SpecificationType type = SpecificationType::Stability;
    double valueDb = 0.0;
    double boundDb = 0.0;
    double excessDb = 0.0;
};

struct FamilyStability
{
    enum class NotChecked { No, NoSweepRecord, Delay, NotRational };

    bool checked = false;
    NotChecked notChecked = NotChecked::NoSweepRecord;
    std::size_t members = 0;
    std::size_t unstableMembers = 0;
    double worstRealPart = -std::numeric_limits<double>::infinity();
    std::vector<std::pair<std::string, double>> worstMember;
};

struct NominalStability
{
    bool checked = false;
    bool stable = false;
    double worstRealPart = -std::numeric_limits<double>::infinity();
};

struct SpecificationCheck
{
    std::vector<SpecificationExcess> entries;
    double worstExcessDb = -std::numeric_limits<double>::infinity();
    FamilyStability family;
    NominalStability nominal;

    bool satisfied() const
    {
        return !(worstExcessDb > 0.0) && family.unstableMembers == 0 && !(nominal.checked && !nominal.stable);
    }
};

struct FrequencyReference
{
    struct Bound
    {
        SpecificationType type;
        double boundDb;
    };

    std::size_t index = 0;
    double omega = 0.0;
    std::complex<double> nominalPlant;
    const ComplexCloud * valueSet = nullptr;
    std::vector<std::complex<double>> nominalOverValueSet;
    std::vector<Bound> bounds;
    WorstCaseMask mask;

    void recordExcesses(std::complex<double> loop, SpecificationCheck & check) const;

    double worstExcessAt(std::complex<double> loop) const;
};

class SpecificationReference
{
public:
    SpecificationReference(LtiSystem & plant, const std::vector<double> & omega,
                           const CloudSet & templates, const SpecificationSet & specifications);

    const std::vector<FrequencyReference> & frequencies() const { return m_frequencies; }

private:
    std::vector<FrequencyReference> m_frequencies;
};

struct RootVerdict
{
    bool stable = false;
    double worstRealPart = -std::numeric_limits<double>::infinity();
};

RootVerdict rootVerdictOf(const std::vector<double> & characteristic);

FamilyStability familyStabilityAt(const SweptFamily & family, const LtiSystem::Polynomials & loop);


SpecificationCheck checkAgainstSpecifications(LtiSystem & controller, LtiSystem & plant,
                                              const std::vector<double> & omega,
                                              const CloudSet & templates,
                                              const SpecificationSet & specifications,
                                              const ParameterGrids * sweep = nullptr);

}

#endif
