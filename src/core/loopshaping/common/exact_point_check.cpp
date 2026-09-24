/**
 * @file
 * @brief One candidate of the search against the specifications themselves.
 */

#include "src/core/loopshaping/common/exact_point_check.h"

#include "src/core/common/exception.h"

namespace qftbx {

ExactPointCheck::ExactPointCheck(LtiSystem & plant, LtiSystem * controller, const std::vector<double> & omega,
                                 const CloudSet & templates, const SpecificationSet & specifications)
{
    if (controller == nullptr || templates.size() < omega.size()) {
        return;
    }

    m_controller = controller->clone();
    m_reference.emplace(plant, omega, templates, specifications);
}

void ExactPointCheck::requireUsable() const
{
    if (!usable()) {
        throw InvalidInput(QFTBX_TR("Core", "The exact point check has no value set for every design frequency."));
    }
}

std::complex<double> ExactPointCheck::loopAt(const FrequencyReference & at, const PointController & point) const
{
    return m_controller->valueAt(at.omega, point.zeros, point.poles, point.gain, m_controller->delay().nominal())
           * at.nominalPlant;
}

bool ExactPointCheck::admits(const PointController & point)
{
    requireUsable();

    ++m_statistics.verdicts;

    const std::vector<FrequencyReference> & frequencies = m_reference->frequencies();
    const std::size_t count = frequencies.size();

    for (std::size_t step = 0; step < count; ++step) {
        const std::size_t i = (m_firstToAsk + step) % count;
        ++m_statistics.kernelPasses;
        if (!(m_reference->worstExcessAt(frequencies[i], loopAt(frequencies[i], point)) <= -kToleranceDb)) {
            m_firstToAsk = i;
            ++m_statistics.rejections;
            return false;
        }
    }

    return true;
}

SpecificationCheck ExactPointCheck::checkOf(const PointController & point) const
{
    requireUsable();

    SpecificationCheck check;
    for (const FrequencyReference & at : m_reference->frequencies()) {
        m_reference->recordExcesses(at, loopAt(at, point), check);
    }
    return check;
}

}
