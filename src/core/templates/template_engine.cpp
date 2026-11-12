/**
 * @file
 * @brief The template sweep, the epsilon-hull walk and the epsilon proposal.
 *
 * The sweep runs one design frequency per thread and every frequency writes
 * at its own index, so nothing needs a critical section; failures inside the
 * parallel region are recorded per frequency and reported after it, because
 * an exception leaving an OpenMP region terminates the process. Before the
 * sweep the number of right half-plane poles is checked over every distinct
 * denominator of the family, and a family whose count is not one number is
 * noted with its first two plants that differ. A design frequency on a pole
 * of the nominal plant is refused,
 * since the nominal loop is infinite there, and a frequency where some other
 * plant has an infinite value is reported rather than turned into a contour
 * of noise. The poles each distinct denominator is asked for give the bands
 * that poles on the imaginary axis sweep, and a design frequency inside one
 * is marked in its report. The walk finds its candidates through a grid of
 * cells a quarter of epsilon wide (walk_cloud.h); a walk that does not close
 * is replaced by the whole cloud or fails, never handed on partial.
 */

#include <chrono>
#include "src/core/math/constants.h"
#include <string>
#include <vector>
#include <optional>
#include <map>
#include <cstdint>
#include "src/core/common/record.h"
#include "src/core/templates/template_engine.h"
#include "src/core/templates/sweep_plan.h"

#include "src/core/math/polynomial.h"
#include "src/core/templates/alpha_shape.h"

#include "src/core/common/text_tokens.h"
#include "src/core/common/exception.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstdio>
#include <stdexcept>
#include <cmath>
#include <limits>
#include <iostream>

using namespace std;

namespace qftbx {

void TemplateEngine::setGrids(ParameterGrids grids){
    m_grids = std::move(grids);
}

void TemplateEngine::setEpsilon(std::vector<double> epsilon){
    m_epsilon = std::move(epsilon);
}

void TemplateEngine::setClouds(CloudSet templates){
    m_clouds = std::move(templates);
    m_resonanceBands.clear();
    m_poleCountChange.reset();
    m_proposals.reset();
}

void TemplateEngine::setAlphaShapeContour(bool alphaShape){
    if (alphaShape != m_alphaShape){
        m_proposals.reset();
    }
    m_alphaShape = alphaShape;
}

bool TemplateEngine::compute(LtiSystem *plant, std::vector<double> *omega){
    const auto timer = std::chrono::steady_clock::now();
    m_frequencies = *omega;

    m_clouds = computeClouds(plant, omega);

    if (cancellationAsked(m_cancellation)) {
        m_clouds.clear();
        throw qftbx::Cancelled();
    }

    {
        std::size_t points = 0;
        for (const ComplexCloud & cloud : m_clouds) points += cloud.size();
        qftbx::record::write("templates", "sweep",
                          qftbx::record::milliseconds(std::chrono::duration<double, std::milli>(
                                                       std::chrono::steady_clock::now() - timer).count())
                              + " " + qftbx::record::number("frequencies", m_clouds.size())
                              + " " + qftbx::record::number("points", points));
    }

    return contourClouds();
}

bool TemplateEngine::contourClouds(){

    if (m_clouds.empty()){
        throw qftbx::ComputationError(QFTBX_TR("Core", "Could not compute the templates."));
    }

    proposeMissingEpsilon();

    const auto timer2 = std::chrono::steady_clock::now();

    const bool result = computeContourSet();

    if (cancellationAsked(m_cancellation)) {
        m_clouds.clear();
        m_contours.clear();
        throw qftbx::Cancelled();
    }

    logContours(timer2);

    return result;
}

void TemplateEngine::proposeMissingEpsilon(){
    if (m_epsilon.empty()){
        const auto proposing = std::chrono::steady_clock::now();
        for (const EpsilonProposal & proposal : proposeEpsilon()){
            m_epsilon.push_back(proposal.epsilon);
        }
        qftbx::record::write("templates", "proposal",
                          qftbx::record::milliseconds(std::chrono::duration<double, std::milli>(
                                                       std::chrono::steady_clock::now() - proposing).count()));
    }
}

bool TemplateEngine::contourPrepared(const std::vector<PreparedCloud> & prepared){

    if (prepared.size() != m_clouds.size()){
        throw qftbx::ComputationError("The contours were given the prepared clouds of " + std::to_string(prepared.size())
                                      + " frequencies for " + std::to_string(m_clouds.size()) + ".");
    }

    const auto timer = std::chrono::steady_clock::now();

    const bool result = computeContourSet(&prepared);

    logContours(timer);

    return result;
}

void TemplateEngine::logContours(std::chrono::steady_clock::time_point since) const
{
    if (!qftbx::record::isOpen()){
        return;
    }

    std::size_t points = 0;
    for (const ComplexCloud & contour : m_contours) points += contour.size();

    qftbx::record::write("contours", m_alphaShape ? "alpha-shape" : "walk",
                      qftbx::record::milliseconds(std::chrono::duration<double, std::milli>(
                                                   std::chrono::steady_clock::now() - since).count())
                          + " " + qftbx::record::number("frequencies", m_contours.size())
                          + " " + qftbx::record::number("points", points));
}

void TemplateEngine::takeEpsilon(std::vector<double> epsilon){

    if (m_clouds.empty()){
        throw qftbx::InvalidInput(QFTBX_TR("Core", "There are no templates to compute contours from."));
    }
    if (epsilon.size() < m_clouds.size()){
        throw qftbx::InvalidInput(QFTBX_TR("Core", "Missing epsilon values for the template contours."));
    }

    m_epsilon = std::move(epsilon);
}

bool TemplateEngine::computeContours(std::vector<double> epsilon){

    takeEpsilon(std::move(epsilon));
    const auto timer = std::chrono::steady_clock::now();

    bool result = computeContourSet();

    logContours(timer);

    return result;
}

const CloudSet & TemplateEngine::clouds() const{
    return m_clouds;
}

const CloudSet & TemplateEngine::contours() const{
    return m_contours;
}

CloudSet TemplateEngine::computeClouds(LtiSystem *plant, std::vector<double> *omega){

    m_proposals.reset();
    SweepPlan plan = planCombinations(*plant, *omega, m_grids, m_borderSweep);
    const auto checking = std::chrono::steady_clock::now();
    adoptFamily(plan, checkFamily(*plant, plan), *omega);
    qftbx::record::write("templates", "family",
                      qftbx::record::milliseconds(std::chrono::duration<double, std::milli>(
                                                   std::chrono::steady_clock::now() - checking).count()));
    adoptPlan(plan);

    return sweepOnProcessor(*plant, *omega, plan, m_cancellation);
}

void TemplateEngine::adoptPlan(const SweepPlan & plan){
    m_combinationCount = plan.combinationCount;
    m_borderSweepApplied = plan.borderSweepApplied();
    m_familyRhpPoles = plan.familyRhpPoles;
    m_poleCountChange = plan.poleCountChange;
    m_resonanceBands = plan.resonanceBands;
}

void TemplateEngine::adoptSweep(CloudSet clouds, const SweepPlan & plan, std::vector<double> frequencies){
    m_proposals.reset();
    m_frequencies = std::move(frequencies);
    m_clouds = std::move(clouds);
    adoptPlan(plan);
}

const std::vector <double> & TemplateEngine::omega() const{
    return m_frequencies;
}

const std::vector <double> & TemplateEngine::epsilon() const{
    return m_epsilon;
}

namespace {

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

}

bool TemplateEngine::computeContourSet(const std::vector<PreparedCloud> * prepared){

    if (m_epsilon.size() != m_clouds.size()) {
        throw qftbx::InvalidInput(QFTBX_TR("Core", "The contours need one epsilon per design frequency: %1 given for %2 frequencies.").arg(m_epsilon.size()).arg(m_clouds.size()));
    }

    const std::size_t digitCount = m_clouds.size();
    std::vector<WalkedContour> walked(digitCount);

#ifdef OpenMP_AVAILABLE
#pragma omp parallel for
#endif
    for (std::size_t i = 0; i < digitCount; i++){

        WalkedContour & out = walked[i];
        SortedCloud own;
        if (prepared == nullptr){
            own = sortedCloud(m_clouds[i], m_metric, m_dbPerDegree);
        }
        const SortedCloud & sorted = prepared != nullptr ? (*prepared)[i].sorted : own;
        out.contour = m_borderSweepApplied
                ? alphaShapeOfSorted(sorted, roundedUpToThreeFigures(connectingEpsilon(m_clouds[i])), &out.starts)
                : m_alphaShape
                ? alphaShapeOfSorted(sorted, m_epsilon.at(i), &out.starts)
                : walkSorted(sorted, projected(m_clouds[i]), m_clouds[i], m_epsilon.at(i),
                             prepared != nullptr ? &(*prepared)[i].parts : nullptr, &out.fellBack, &out.truncated,
                             &out.starts);
    }

    return finishContours(std::move(walked));
}

bool TemplateEngine::contourWalked(std::vector<WalkedContour> walked){

    if (walked.size() != m_clouds.size()){
        throw qftbx::ComputationError("The contours were given the walks of " + std::to_string(walked.size())
                                      + " frequencies for " + std::to_string(m_clouds.size()) + ".");
    }

    return finishContours(std::move(walked));
}

bool TemplateEngine::finishContours(std::vector<WalkedContour> walked){

    bool succeeded = true;
    const std::size_t digitCount = m_clouds.size();

    m_reports.assign(digitCount, ContourReport{});
    for (std::size_t i = 0; i < digitCount; i++){
        m_reports[i].cloudPoints = m_clouds[i].size();
        if (i < m_resonanceBands.size() && m_resonanceBands[i].has_value()){
            m_reports[i].unbounded = true;
            m_reports[i].resonanceFrom = (*m_resonanceBands[i])[0];
            m_reports[i].resonanceTo = (*m_resonanceBands[i])[1];
        }
    }

    m_contours = CloudSet(digitCount);

    std::vector<char> failed (digitCount, 0);

    for (std::size_t i = 0; i < digitCount; i++){

        WalkedContour & out = walked[i];
        ComplexCloud cont = std::move(out.contour);
        std::vector<std::size_t> starts = std::move(out.starts);

        bool wholeCloud = false;
        if ((out.truncated || cont.empty()) && m_wholeCloudStandsIn){
            cont = m_clouds[i];
            starts.assign(1, 0);
            wholeCloud = true;
        }

        m_reports[i].contourPoints = cont.size();
        m_reports[i].relaxed = out.fellBack;
        m_reports[i].truncated = out.truncated;
        m_reports[i].wholeCloud = wholeCloud;
        m_reports[i].components = std::max<std::size_t>(starts.size(), 1);
        m_reports[i].componentStarts = std::move(starts);

        if (cont.empty()){
            failed[i] = true;
            succeeded = false;
        }

        m_contours[i] = std::move(cont);
    }

    std::vector<std::string> relaxed;
    for (std::size_t i = 0; i < digitCount; i++){
        if (m_reports[i].relaxed && i < m_frequencies.size()){
            relaxed.push_back(qftbx::text::number(m_frequencies.at(i)));
        }
    }

    const auto label = [this](std::size_t i){
        return i < m_frequencies.size()
                ? "w = " + qftbx::text::number(m_frequencies.at(i)) + " rad/s"
                : "frequency index " + std::to_string(i);
    };

    std::vector<std::string> truncatedAt;
    for (std::size_t i = 0; i < digitCount; i++){
        if (m_reports[i].wholeCloud){
            truncatedAt.push_back(label(i));
        }
    }

    if (!relaxed.empty()){
        qftbx::record::write("contours", "relaxed", qftbx::text::join(relaxed, ", ") + " rad/s");
    }
    std::vector<std::string> split;
    for (std::size_t i = 0; i < digitCount; i++){
        if (m_reports[i].components > 1 && i < m_frequencies.size()){
            split.push_back(qftbx::text::number(m_frequencies.at(i)) + " (" +
                            std::to_string(m_reports[i].components) + ")");
        }
    }
    if (!split.empty()){
        qftbx::record::write("contours", "split", qftbx::text::join(split, ", ") + " rad/s");
    }

    if (!truncatedAt.empty()){
        qftbx::record::write("contours", "whole cloud", qftbx::text::join(truncatedAt, ", "));
    }

    std::vector<std::string> unboundedAt;
    for (std::size_t i = 0; i < digitCount; i++){
        if (m_reports[i].unbounded){
            unboundedAt.push_back(label(i));
        }
    }
    if (!unboundedAt.empty()){
        qftbx::record::write("contours", "unbounded", qftbx::text::join(unboundedAt, ", "));
    }

    if (!succeeded){
        std::vector<std::string> detail;

        for (std::size_t i = 0; i < digitCount; i++){
            if (!failed.at(i)){
                continue;
            }

            double largest = 0;
            for (const complex<double> & value : m_clouds[i]){
                largest = std::max(largest, std::abs(value));
            }

            const std::string where = i < m_frequencies.size()
                    ? qftbx::text::number(m_frequencies.at(i))
                    : std::to_string(i);

            detail.push_back(where + " rad/s (largest |P| = "
                             + qftbx::text::number(largest) + ")");
        }

        throw qftbx::ComputationError(QFTBX_TR("Core", "The contour did not close at %1 with the epsilon given. A larger epsilon or a denser template closes it; or let the whole template stand in for the contour (templates dialog, or the setting algorithms.whole-template-if-no-contour).")
                .arg(qftbx::text::join(detail, "; ")));
    }

    return true;
}

void TemplateEngine::setHullMetric(HullMetric metric, double dbPerDegree){
    if (!(dbPerDegree > 0.0) || !std::isfinite(dbPerDegree)){
        throw qftbx::InvalidInput(QFTBX_TR("Core", "The decibels per degree of the Nichols metric must be a finite positive number."));
    }
    if (metric != m_metric || dbPerDegree != m_dbPerDegree){
        m_proposals.reset();
    }
    m_metric = metric;
    m_dbPerDegree = dbPerDegree;
}

ComplexCloud TemplateEngine::projected(const ComplexCloud & points) const{
    return projectedCloud(points, m_metric, m_dbPerDegree);
}

namespace {

void spanningExtremes(const ComplexCloud & pts, double & longest, double & diameter)
{
    const std::size_t n = pts.size();
    longest = 0.0;
    diameter = 0.0;
    if (n < 2){
        return;
    }
    std::vector<double> best(n, std::numeric_limits<double>::infinity());
    std::vector<char> inTree(n, 0);
    inTree[0] = 1;
    for (std::size_t j = 1; j < n; ++j){
        best[j] = std::abs(pts[j] - pts[0]);
    }
    for (std::size_t added = 1; added < n; ++added){
        std::size_t next = n;
        for (std::size_t j = 0; j < n; ++j){
            if (!inTree[j] && (next == n || best[j] < best[next])){
                next = j;
            }
        }
        longest = std::max(longest, best[next]);
        inTree[next] = 1;
        for (std::size_t j = 0; j < n; ++j){
            if (!inTree[j]){
                best[j] = std::min(best[j], std::abs(pts[j] - pts[next]));
            }
        }
    }
    for (std::size_t i = 0; i < n; ++i){
        for (std::size_t j = i + 1; j < n; ++j){
            diameter = std::max(diameter, std::abs(pts[i] - pts[j]));
        }
    }
}

}

double TemplateEngine::connectingEpsilon(const ComplexCloud & cloud) const{
    ComplexCloud pts = projected(cloud);
    std::sort(pts.begin(), pts.end(), [](const complex<double> & a, const complex<double> & b){
        return a.real() != b.real() ? a.real() < b.real() : a.imag() < b.imag();
    });
    pts.erase(std::unique(pts.begin(), pts.end()), pts.end());
    double longest = 0.0, diameter = 0.0;
    spanningExtremes(pts, longest, diameter);
    return longest;
}

bool TemplateEngine::retraces(const ComplexCloud & walked)
{
    std::vector<complex<double>> seen (walked.begin(), walked.end());
    std::sort(seen.begin(), seen.end(),
              [](const complex<double> & a, const complex<double> & b){
                  return a.real() != b.real() ? a.real() < b.real() : a.imag() < b.imag();
              });
    seen.erase(std::unique(seen.begin(), seen.end()), seen.end());

    return walked.size() > seen.size() + 1;
}

double TemplateEngine::withoutRetracing(const ComplexCloud & cloud, double closing,
                                        double diameter)
{
    return withoutRetracing(sortedCloud(cloud, m_metric, m_dbPerDegree), projected(cloud), cloud, closing, diameter);
}

double TemplateEngine::withoutRetracing(const SortedCloud & sorted, const ComplexCloud & cloudWalk,
                                        const ComplexCloud & cloud, double closing, double diameter)
{
    constexpr int kRungs = 4;
    constexpr double kStep = 1.5;

    double raw = closing;
    for (int rung = 0; rung < kRungs; ++rung){
        raw *= kStep;
        if (raw > diameter){
            break;
        }

        const double candidate = roundedUpToThreeFigures(raw);

        bool truncated = false;
        bool fellBack = false;
        const ComplexCloud walked = walkSorted(sorted, cloudWalk, cloud, candidate, nullptr, &fellBack, &truncated,
                                               nullptr);

        if (!walked.empty() && !truncated && !fellBack && !retraces(walked)){
            return candidate;
        }
    }

    return closing;
}

std::vector<EpsilonProposal> TemplateEngine::proposeEpsilon(){

    if (m_proposals.has_value()){
        return *m_proposals;
    }

    std::vector<EpsilonProposal> proposals;
    proposals.reserve(m_clouds.size());

    for (const ComplexCloud & cloud : m_clouds){
        EpsilonProposal proposal;

        ComplexCloud pts = projected(cloud);
        std::sort(pts.begin(), pts.end(), [](const complex<double> & a, const complex<double> & b){
            return a.real() != b.real() ? a.real() < b.real() : a.imag() < b.imag();
        });
        pts.erase(std::unique(pts.begin(), pts.end()), pts.end());
        const std::size_t n = pts.size();

        if (n >= 2){
            double longest = 0.0;
            double diameter = 0.0;
            spanningExtremes(pts, longest, diameter);
            proposal.connected = longest;
            proposal.diameter = diameter;
            proposal.epsilon = longest;

            if (m_alphaShape || m_borderSweepApplied){
                proposal.epsilon = roundedUpToThreeFigures(longest);
                proposal.closes = true;
                proposals.push_back(proposal);
                continue;
            }

            const SortedCloud sorted = sortedCloud(cloud, m_metric, m_dbPerDegree);
            const ComplexCloud cloudWalk = projected(cloud);
            double previous = 0.0;
            for (double raw = longest; ; raw *= 1.01){
                const bool last = raw >= diameter;
                const double candidate = roundedUpToThreeFigures(last ? diameter : raw);
                if (candidate > previous){
                    previous = candidate;
                    bool truncated = false;
                    bool fellBack = false;
                    const ComplexCloud walked = walkSorted(sorted, cloudWalk, cloud, candidate, nullptr, &fellBack,
                                                           &truncated, nullptr);
                    if (!walked.empty() && !truncated && !fellBack){
                        proposal.epsilon = retraces(walked)
                                               ? withoutRetracing(sorted, cloudWalk, cloud, candidate, diameter)
                                               : candidate;
                        proposal.closes = true;
                        break;
                    }
                }
                if (last){
                    break;
                }
            }
        }

        proposals.push_back(proposal);
    }

    m_proposals = proposals;
    return proposals;
}

ComplexCloud TemplateEngine::alphaShapeContour(const ComplexCloud & cloud, double epsilon,
                                               std::vector<std::size_t> * componentStarts) const{
    if (componentStarts != nullptr){
        componentStarts->clear();
    }
    if (cloud.empty()){
        return {};
    }

    return alphaShapeOfSorted(sortedCloud(cloud, m_metric, m_dbPerDegree), epsilon, componentStarts);
}

ComplexCloud TemplateEngine::alphaShapeOfSorted(const SortedCloud & sorted, double epsilon,
                                                std::vector<std::size_t> * componentStarts) const{
    if (componentStarts != nullptr){
        componentStarts->clear();
    }
    if (sorted.points.empty()){
        return {};
    }

    const ComplexCloud & cv = sorted.points;
    const AlphaShape shape = alphaShape(sorted.walk, epsilon);

    ComplexCloud result;
    std::vector<char> emitted(cv.size(), 0);
    for (const std::vector<std::int32_t> & loop : shape.loops){
        if (componentStarts != nullptr){
            componentStarts->push_back(result.size());
        }
        std::fill(emitted.begin(), emitted.end(), 0);
        for (const std::int32_t idx : loop){
            if (!emitted[static_cast<std::size_t>(idx)]){
                emitted[static_cast<std::size_t>(idx)] = 1;
                result.push_back(cv[static_cast<std::size_t>(idx)]);
            }
        }
        if (loop.size() > 1){
            result.push_back(cv[static_cast<std::size_t>(loop.front())]);
        }
    }
    return result;
}

ComplexCloud TemplateEngine::epsilonHull(const ComplexCloud & temp, double epsilon,
                                         bool * fellBack, bool * truncated,
                                         std::vector<std::size_t> * componentStarts){

    if (componentStarts != nullptr){
        componentStarts->clear();
    }

    if (temp.empty()){
        return {};
    }

    return walkSorted(sortedCloud(temp, m_metric, m_dbPerDegree), projected(temp), temp, epsilon, nullptr,
                      fellBack, truncated, componentStarts);
}

ComplexCloud TemplateEngine::walkSorted(const SortedCloud & sorted, const ComplexCloud & tempWalk,
                                        const ComplexCloud & temp, double epsilon, const CloudParts * given,
                                        bool * fellBack, bool * truncated,
                                        std::vector<std::size_t> * componentStarts){

    if (componentStarts != nullptr){
        componentStarts->clear();
    }

    if (sorted.points.empty()){
        return {};
    }

    const ComplexCloud & cv = sorted.points;
    const ComplexCloud & cvWalk = sorted.walk;

    const NeighbourGrid neighbours(cvWalk, epsilon);
    CloudParts found;
    if (given == nullptr){
        found = cloudParts(cvWalk, epsilon, neighbours);
    }
    const CloudParts & parts = given != nullptr ? *given : found;

    if (parts.starts.size() == 1){
        if (componentStarts != nullptr){
            componentStarts->push_back(0);
        }
        return walkComponent(cv, cvWalk, temp, tempWalk, epsilon, neighbours, fellBack, truncated);
    }

    ComplexCloud result;
    bool anyWalked = false;

    for (std::size_t p = 0; p < parts.starts.size(); ++p){
        const std::size_t end = p + 1 < parts.starts.size() ? parts.starts[p + 1] : parts.members.size();
        ComplexCloud points, walkPoints;
        points.reserve(end - parts.starts[p]);
        walkPoints.reserve(end - parts.starts[p]);
        for (std::size_t m = parts.starts[p]; m < end; ++m){
            const std::size_t index = static_cast<std::size_t>(parts.members[m]);
            points.push_back(cv.at(index));
            walkPoints.push_back(cvWalk.at(index));
        }

        const NeighbourGrid own(walkPoints, epsilon);

        bool partFellBack = false;
        bool partTruncated = false;
        ComplexCloud contour = walkComponent(points, walkPoints, points, walkPoints, epsilon, own,
                                             &partFellBack, &partTruncated);

        if (fellBack != nullptr && partFellBack){
            *fellBack = true;
        }
        if (partTruncated){
            if (truncated != nullptr){
                *truncated = true;
            }
            if (componentStarts != nullptr){
                componentStarts->clear();
            }
            return {};
        }

        if (contour.empty()){
            contour = points;
        } else {
            anyWalked = true;
        }
        if (componentStarts != nullptr){
            componentStarts->push_back(result.size());
        }

        result.insert(result.end(), contour.begin(), contour.end());
    }

    if (!anyWalked){
        if (componentStarts != nullptr){
            componentStarts->clear();
        }
        return {};
    }

    return result;
}

ComplexCloud TemplateEngine::walkComponent(const ComplexCloud & source, const ComplexCloud & walk,
                                           const ComplexCloud & fallbackSource, const ComplexCloud & fallbackWalk,
                                           double epsilon, const NeighbourGrid & neighbours,
                                           bool * fellBack, bool * truncated){

    const ComplexCloud & cv = walk;
    const std::size_t pointCount = cv.size();
    const std::size_t MAXP = 3 * pointCount;

    std::int32_t b1 = 0;
    for (std::size_t i = 1; i < pointCount; i++){
        if (real(cv.at(i)) > real(cv.at(static_cast<std::size_t>(b1)))){
            b1 = i;
        }
    }

    std::int32_t b2 = findSecond(b1, cv, epsilon, neighbours);

    if (b2 < 0)
        return {};

    std::vector <std::int32_t> steps;
    steps.push_back(b1);
    steps.push_back(b2);

    std::int32_t previousPoint = b1;
    std::int32_t currentPoint = b2;

    std::int32_t nextPoint = findNext(b1, b2, cv, epsilon, neighbours);
    if (nextPoint < 0)
        return {};

    std::size_t counter = 2;

    while (b1 != currentPoint || b2 != nextPoint){

        steps.push_back(nextPoint);
        counter++;

        if (counter > MAXP){
            if (fellBack != nullptr){
                *fellBack = true;
            }

            return epsilonHullRelaxed(fallbackSource, fallbackWalk, epsilon, truncated);
        }

        previousPoint = currentPoint;
        currentPoint = nextPoint;

        nextPoint = findNext(previousPoint, currentPoint, cv, epsilon, neighbours);

        if (nextPoint < 0){
            return {};
        }
    }

    ComplexCloud result;
    result.reserve(steps.size());

    for (const std::int32_t var : steps) {
        result.push_back(source.at(static_cast<std::size_t>(var)));
    }

    return result;
}

ComplexCloud TemplateEngine::epsilonHullRelaxed(const ComplexCloud & source, const ComplexCloud & walkPoints,
                                                double epsilon, bool * truncated){

    const ComplexCloud & temp = walkPoints;
    std::size_t pointCount = temp.size();
    std::size_t MAXP = 3 * pointCount;

    std::int32_t b1 = 0;
    double largestImag = -numeric_limits<double>::infinity();

    for(std::size_t i = 0;i < pointCount ; i++){
        if (imag(temp.at(i)) > largestImag){
            b1 = i;
            largestImag = imag(temp.at(i));
        }
    }

    const NeighbourGrid neighbours(temp, epsilon);

    std::int32_t b2 = findSecond(b1, temp, epsilon, neighbours);

    if (b2 < 0)
        return {};

    std::vector <std::int32_t> walk;
    walk.push_back(b1);
    walk.push_back(b2);

    std::int32_t previousPoint = b1;
    std::int32_t currentPoint = b2;

    std::int32_t nextPoint = findNext(b1, b2, temp, epsilon, neighbours, true);
    if (nextPoint < 0)
        return {};

    std::size_t counter = 2;

    while (b1 != currentPoint || b2 != nextPoint){

        walk.push_back(nextPoint);
        counter++;

        if (counter > MAXP){
            if (truncated != nullptr){
                *truncated = true;
            }
            return {};
        }

        previousPoint = currentPoint;
        currentPoint = nextPoint;

        nextPoint = findNext(previousPoint, currentPoint, temp, epsilon, neighbours, true);

        if (nextPoint < 0){
            return {};
        }
    }

    std::vector <std::int32_t> uniqueIdx;
    for (std::int32_t idx : walk) {
        if (std::find(uniqueIdx.begin(), uniqueIdx.end(), idx) == uniqueIdx.end()){
            uniqueIdx.push_back(idx);
        }
    }

    ComplexCloud result;
    result.reserve(uniqueIdx.size());

    for (const std::int32_t idx : uniqueIdx) {
        result.push_back(source.at(static_cast<std::size_t>(idx)));
    }

    return result;
}

std::int32_t TemplateEngine::findSecond(std::int32_t b1, const ComplexCloud & cv, double epsilon,
                                        const NeighbourGrid & neighbours){

    double dist = 0;
    complex <double> firstPoint = cv.at(static_cast<std::size_t>(b1));

    double fmin = numeric_limits<double>::infinity();
    std::int32_t pmin = -1;
    double dmax = 0;

    complex <double> candidate;

    double phase = 0;

    std::vector<std::int32_t> nearby;
    neighbours.candidates(firstPoint, nearby);

    for (const std::int32_t i : nearby){

        candidate = cv[static_cast<std::size_t>(i)];

        if (!withinReach(candidate, firstPoint, epsilon)){
            continue;
        }

        dist = abs(firstPoint - candidate);

        if (dist > 0 && dist <= epsilon){

            phase = arg (candidate - firstPoint);

            if (phase < 0)
                phase += 2 * qftbx::math::kPi;

            phase -= std::acos(dist / epsilon);

            if (phase < fmin){
                fmin = phase;
                pmin = i;
                dmax = dist;

            }else if (phase == fmin && (dist > dmax || (dist == dmax && i < pmin))){
                pmin = i;
                dmax = dist;
            }
        }
    }

    return pmin;
}

namespace {

constexpr double kStraightBack = 1e-9;

}

std::int32_t TemplateEngine::findNext(std::int32_t previousPoint, std::int32_t currentPoint,
                                  const ComplexCloud & cv, double epsilon,
                                  const NeighbourGrid & neighbours, bool excludePrevious){

    complex <double> current = cv.at(static_cast<std::size_t>(currentPoint));
    complex <double> previous = cv.at(static_cast<std::size_t>(previousPoint));

    double aco2 = std::acos(abs(previous-current) / epsilon);

    double phase = 0;

    double aco1 = 0;
    double dmax = 0;

    double psi = 0;

    double psiMin = numeric_limits<double>::infinity();
    std::int32_t bestIndex = -1;

    complex <double> candidate;
    double distance;

    std::vector<std::int32_t> nearby;
    neighbours.candidates(current, nearby);

    for (const std::int32_t i : nearby){

        candidate = cv[static_cast<std::size_t>(i)];

        if (!withinReach(candidate, current, epsilon)){
            continue;
        }

        distance = abs(candidate - current);

        if (distance > 0 && distance <= epsilon &&
                !(excludePrevious && (candidate == previous || candidate == current))){

            phase = arg(candidate - current) - arg(previous - current);

            if(phase < 0)
                phase +=  2 * qftbx::math::kPi;

            aco1 = std::acos(distance / epsilon );

            if(phase < kStraightBack || phase > 2 * qftbx::math::kPi - kStraightBack){
                psi =  2 * qftbx::math::kPi - aco1 - aco2;
            }else if (phase > 0 && phase < aco2){
                psi = phase + aco1- aco2;
            }else{
                psi = phase - aco1 - aco2;
            }

            if (psi < 0)
                psi +=  2 * qftbx::math::kPi;

            if (psi < psiMin) {
                psiMin = psi;
                bestIndex = i;
                dmax = distance;
            }else if (psi == psiMin &&
                      (distance > dmax || (distance == dmax && i < bestIndex))){
                bestIndex = i;
                dmax = distance;
            }
        }
    }

    return bestIndex;

}

}
