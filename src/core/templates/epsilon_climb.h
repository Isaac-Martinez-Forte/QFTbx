/**
 * @file
 * @brief The epsilons the proposal tries, the same on every processor.
 *
 * Every epsilon proposed is rounded up to three significant figures.
 * EpsilonClimb gives the climb from the connecting epsilon, the longest
 * edge of the minimum spanning tree, one per cent at a time in three
 * figures and each greater than the last, up to the diameter of the cloud,
 * which is the last. rungCandidates() are the at most four steps of 1.5
 * from an epsilon whose walk closes but passes a point twice, those below
 * the diameter. An engine that walks the candidates elsewhere tries them in
 * the same order and takes the same first one.
 */

#ifndef QFTBX_EPSILON_CLIMB_H
#define QFTBX_EPSILON_CLIMB_H

#include <optional>
#include <vector>

namespace qftbx {

double roundedUpToThreeFigures(double value);

class EpsilonClimb
{
public:
    EpsilonClimb(double longest, double diameter);

    std::optional<double> next();

private:
    double m_raw;
    double m_diameter;
    double m_previous = 0.0;
    bool m_done = false;
};

std::vector<double> rungCandidates(double closing, double diameter);

}

#endif
