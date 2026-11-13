/**
 * @file
 * @brief The rounding, the climb and the rungs of the epsilon proposal.
 *
 * The rounding goes through the text of the number, so that the value is
 * the one its three figures read as.
 */

#include "src/core/templates/epsilon_climb.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace qftbx {

double roundedUpToThreeFigures(double value)
{
    if (!(value > 0.0) || !std::isfinite(value)){
        return value;
    }
    const double scale = std::pow(10.0, std::floor(std::log10(value)) - 2.0);
    char text[32];
    std::snprintf(text, sizeof text, "%.3g", std::ceil(value / scale) * scale);
    return std::strtod(text, nullptr);
}

EpsilonClimb::EpsilonClimb(double longest, double diameter)
    : m_raw(longest), m_diameter(diameter)
{
}

std::optional<double> EpsilonClimb::next()
{
    while (!m_done){
        const bool last = m_raw >= m_diameter;
        const double candidate = roundedUpToThreeFigures(last ? m_diameter : m_raw);
        if (last){
            m_done = true;
        } else {
            m_raw *= 1.01;
        }
        if (candidate > m_previous){
            m_previous = candidate;
            return candidate;
        }
    }
    return std::nullopt;
}

std::vector<double> rungCandidates(double closing, double diameter)
{
    constexpr int kRungs = 4;
    constexpr double kStep = 1.5;

    std::vector<double> rungs;
    double raw = closing;
    for (int rung = 0; rung < kRungs; ++rung){
        raw *= kStep;
        if (raw > diameter){
            break;
        }
        rungs.push_back(roundedUpToThreeFigures(raw));
    }
    return rungs;
}

}
