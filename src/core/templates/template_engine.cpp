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
 * refused. A design frequency on a pole of the nominal plant is refused,
 * since the nominal loop is infinite there, and a frequency where some other
 * plant has an infinite value is reported rather than turned into a contour
 * of noise. The poles each distinct denominator is asked for give the bands
 * that poles on the imaginary axis sweep, and a design frequency inside one
 * is marked in its report. The walk finds its candidates through a grid of
 * cells a quarter of epsilon wide; a walk that does not close is replaced by
 * the whole cloud or fails, never handed on partial.
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

    if (m_epsilon.empty()){
        const auto proposing = std::chrono::steady_clock::now();
        for (const EpsilonProposal & proposal : proposeEpsilon()){
            m_epsilon.push_back(proposal.epsilon);
        }
        qftbx::record::write("templates", "proposal",
                          qftbx::record::milliseconds(std::chrono::duration<double, std::milli>(
                                                       std::chrono::steady_clock::now() - proposing).count()));
    }

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

bool TemplateEngine::computeContours(std::vector<double> epsilon){

    if (m_clouds.empty()){
        throw qftbx::InvalidInput(QFTBX_TR("Core", "There are no templates to compute contours from."));
    }
    if (epsilon.size() < m_clouds.size()){
        throw qftbx::InvalidInput(QFTBX_TR("Core", "Missing epsilon values for the template contours."));
    }

    m_epsilon = std::move(epsilon);
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
    const SweepPlan plan = planSweep(*plant, *omega, m_grids, m_borderSweep);
    adoptPlan(plan);

    return sweepOnProcessor(*plant, *omega, plan, m_cancellation);
}

void TemplateEngine::adoptPlan(const SweepPlan & plan){
    m_combinationCount = plan.combinationCount;
    m_borderSweepApplied = plan.borderSweepApplied();
    m_familyRhpPoles = plan.familyRhpPoles;
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

bool TemplateEngine::computeContourSet(){

    if (m_epsilon.size() != m_clouds.size()) {
        throw qftbx::InvalidInput(QFTBX_TR("Core", "The contours need one epsilon per design frequency: %1 given for %2 frequencies.").arg(m_epsilon.size()).arg(m_clouds.size()));
    }

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
    std::vector<char> relaxedFrequencies (digitCount, 0);
    std::vector<char> truncatedFrequencies (digitCount, 0);

#ifdef OpenMP_AVAILABLE
#pragma omp parallel for
#endif
    for (std::size_t i = 0; i < digitCount; i++){

        bool fellBack = false;
        bool truncated = false;
        std::vector<std::size_t> starts;
        ComplexCloud cont = m_borderSweepApplied
                ? alphaShapeContour(m_clouds[i], roundedUpToThreeFigures(connectingEpsilon(m_clouds[i])), &starts)
                : m_alphaShape
                ? alphaShapeContour(m_clouds[i], m_epsilon.at(i), &starts)
                : epsilonHull(m_clouds[i], m_epsilon.at(i), &fellBack, &truncated, &starts);

        if (fellBack){
            relaxedFrequencies[i] = true;
        }
        if (truncated){
            truncatedFrequencies[i] = true;
        }

        bool wholeCloud = false;
        if ((truncated || cont.empty()) && m_wholeCloudStandsIn){
            cont = m_clouds[i];
            starts.assign(1, 0);
            wholeCloud = true;
        }

        m_reports[i].contourPoints = cont.size();
        m_reports[i].relaxed = fellBack;
        m_reports[i].truncated = truncated;
        m_reports[i].wholeCloud = wholeCloud;
        m_reports[i].components = std::max<std::size_t>(starts.size(), 1);
        m_reports[i].componentStarts = std::move(starts);

        if (cont.empty()){
            failed[i] = true;

#ifdef OpenMP_AVAILABLE
#pragma omp critical
#endif
            {
                succeeded = false;
            }
        }

        m_contours[i] = std::move(cont);
    }

    std::vector<std::string> relaxed;
    for (std::size_t i = 0; i < digitCount; i++){
        if (relaxedFrequencies.at(i) && i < m_frequencies.size()){
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

class TemplateEngine::NeighbourGrid
{
public:
    NeighbourGrid(const ComplexCloud & points, double epsilon)
        : m_pointCount(points.size())
    {
        double minRe = numeric_limits<double>::infinity();
        double minIm = numeric_limits<double>::infinity();
        double maxRe = -numeric_limits<double>::infinity();
        double maxIm = -numeric_limits<double>::infinity();

        for (const complex<double> & point : points){
            minRe = std::min(minRe, point.real());
            maxRe = std::max(maxRe, point.real());
            minIm = std::min(minIm, point.imag());
            maxIm = std::max(maxIm, point.imag());
        }

        const double side = epsilon / kCellsPerEpsilon;
        const bool finite = points.empty() ||
                (std::isfinite(minRe) && std::isfinite(maxRe) && std::isfinite(minIm) && std::isfinite(maxIm));
        const bool indexable = finite && side > 0 && std::isfinite(side) &&
                (maxRe - minRe) / side < kMaxCells && (maxIm - minIm) / side < kMaxCells;

        if (!indexable){
            return;
        }

        m_indexed = true;
        m_side = side;
        m_minRe = minRe;
        m_minIm = minIm;

        m_cells.reserve(points.size());
        for (std::size_t i = 0; i < points.size(); ++i){
            m_cells.push_back({cellOf(points[i]), static_cast<std::int32_t>(i)});
        }

        std::sort(m_cells.begin(), m_cells.end());
    }

    void candidates(const complex<double> & centre, std::vector<std::int32_t> & out) const
    {
        out.clear();

        if (!m_indexed){
            for (std::size_t i = 0; i < m_pointCount; ++i){
                out.push_back(static_cast<std::int32_t>(i));
            }
            return;
        }

        const Cell cell = cellOf(centre);

        for (std::int64_t x = cell.x - kReach; x <= cell.x + kReach; ++x){
            const Cell first{x, cell.y - kReach};
            const Cell last{x, cell.y + kReach};

            auto entry = std::lower_bound(m_cells.begin(), m_cells.end(), Entry{first, 0});

            for (; entry != m_cells.end() && !(last < entry->cell); ++entry){
                out.push_back(entry->index);
            }
        }
    }

private:
    struct Cell {
        std::int64_t x;
        std::int64_t y;

        bool operator<(const Cell & other) const { return x != other.x ? x < other.x : y < other.y; }
    };

    struct Entry {
        Cell cell;
        std::int32_t index;

        bool operator<(const Entry & other) const
        {
            if (cell < other.cell) return true;
            if (other.cell < cell) return false;
            return index < other.index;
        }
    };

    static constexpr double kCellsPerEpsilon = 4.0;
    static constexpr std::int64_t kReach = 5;
    static constexpr double kMaxCells = 1e9;

    Cell cellOf(const complex<double> & point) const
    {
        return {static_cast<std::int64_t>(std::floor((point.real() - m_minRe) / m_side)),
                static_cast<std::int64_t>(std::floor((point.imag() - m_minIm) / m_side))};
    }

    std::size_t m_pointCount = 0;
    bool m_indexed = false;
    double m_side = 0.0;
    double m_minRe = 0.0;
    double m_minIm = 0.0;
    std::vector<Entry> m_cells;
};

namespace {

inline bool withinReach(const complex<double> & candidate, const complex<double> & centre,
                        double epsilon)
{
    const double dx = candidate.real() - centre.real();
    const double dy = candidate.imag() - centre.imag();

    return dx * dx + dy * dy <= epsilon * epsilon * (1.0 + 1e-9);
}

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

    if (m_metric == HullMetric::ComplexPlane || points.empty()){
        return points;
    }

    std::vector<double> phases;
    phases.reserve(points.size());
    for (const complex<double> & z : points){
        phases.push_back(std::arg(z) * 180.0 / qftbx::math::kPi);
    }

    std::vector<double> sorted = phases;
    std::sort(sorted.begin(), sorted.end());
    double widest = sorted.front() + 360.0 - sorted.back();
    bool shiftBelow = false;
    double cut = 0.0;
    for (std::size_t i = 1; i < sorted.size(); ++i){
        const double gap = sorted[i] - sorted[i - 1];
        if (gap > widest){
            widest = gap;
            cut = sorted[i - 1];
            shiftBelow = true;
        }
    }

    ComplexCloud out;
    out.reserve(points.size());
    for (std::size_t i = 0; i < points.size(); ++i){
        double phase = phases[i];
        if (shiftBelow && phase <= cut){
            phase += 360.0;
        }
        const double magnitude = std::abs(points[i]);
        const double db = magnitude > 0.0 ? 20.0 * std::log10(magnitude) : -400.0;
        out.emplace_back(phase, db / m_dbPerDegree);
    }
    return out;
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
        const ComplexCloud walked = epsilonHull(cloud, candidate, &fellBack, &truncated, nullptr);

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

            double previous = 0.0;
            for (double raw = longest; ; raw *= 1.01){
                const bool last = raw >= diameter;
                const double candidate = roundedUpToThreeFigures(last ? diameter : raw);
                if (candidate > previous){
                    previous = candidate;
                    bool truncated = false;
                    bool fellBack = false;
                    const ComplexCloud walked = epsilonHull(cloud, candidate, &fellBack, &truncated, nullptr);
                    if (!walked.empty() && !truncated && !fellBack){
                        proposal.epsilon = retraces(walked)
                                               ? withoutRetracing(cloud, candidate, diameter)
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

    ComplexCloud cv = cloud;
    std::sort(cv.begin(), cv.end(),
              [](const complex<double> & a, const complex<double> & b){
                  const double absA = abs(a);
                  const double absB = abs(b);
                  return absA != absB ? absA < absB : arg(a) < arg(b);
              });
    cv.erase(std::unique(cv.begin(), cv.end()), cv.end());

    const AlphaShape shape = alphaShape(projected(cv), epsilon);

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

    ComplexCloud cv = temp;
    std::sort(cv.begin(), cv.end(),
              [](const complex<double> & a, const complex<double> & b){
                  const double absA = abs(a);
                  const double absB = abs(b);
                  if (absA != absB){
                      return absA < absB;
                  }
                  return arg(a) < arg(b);
              });
    cv.erase(std::unique(cv.begin(), cv.end()), cv.end());

    const ComplexCloud cvWalk = projected(cv);
    const ComplexCloud tempWalk = projected(temp);

    const NeighbourGrid neighbours(cvWalk, epsilon);
    const std::vector<std::vector<std::int32_t>> parts = components(cvWalk, epsilon, neighbours);

    if (parts.size() == 1){
        if (componentStarts != nullptr){
            componentStarts->push_back(0);
        }
        return walkComponent(cv, cvWalk, temp, tempWalk, epsilon, neighbours, fellBack, truncated);
    }

    ComplexCloud result;
    bool anyWalked = false;

    for (const std::vector<std::int32_t> & part : parts){
        ComplexCloud points, walkPoints;
        points.reserve(part.size());
        walkPoints.reserve(part.size());
        for (const std::int32_t index : part){
            points.push_back(cv.at(static_cast<std::size_t>(index)));
            walkPoints.push_back(cvWalk.at(static_cast<std::size_t>(index)));
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

std::vector<std::vector<std::int32_t>> TemplateEngine::components(const ComplexCloud & cv, double epsilon,
                                                                  const NeighbourGrid & neighbours){
    const std::size_t n = cv.size();

    std::vector<std::int32_t> parent(n);
    for (std::size_t i = 0; i < n; ++i){
        parent[i] = static_cast<std::int32_t>(i);
    }

    const auto find = [&parent](std::int32_t x){
        while (parent[static_cast<std::size_t>(x)] != x){
            parent[static_cast<std::size_t>(x)] = parent[static_cast<std::size_t>(parent[static_cast<std::size_t>(x)])];
            x = parent[static_cast<std::size_t>(x)];
        }
        return x;
    };

    std::vector<std::int32_t> nearby;
    for (std::size_t i = 0; i < n; ++i){
        neighbours.candidates(cv[i], nearby);
        const std::int32_t ri = find(static_cast<std::int32_t>(i));
        for (const std::int32_t j : nearby){
            if (static_cast<std::size_t>(j) <= i || !withinReach(cv[static_cast<std::size_t>(j)], cv[i], epsilon)){
                continue;
            }
            const std::int32_t rj = find(j);
            if (rj != ri){
                parent[static_cast<std::size_t>(rj)] = ri;
            }
        }
    }

    std::vector<std::vector<std::int32_t>> byRoot(n);
    for (std::size_t i = 0; i < n; ++i){
        byRoot[static_cast<std::size_t>(find(static_cast<std::int32_t>(i)))].push_back(static_cast<std::int32_t>(i));
    }

    std::vector<std::vector<std::int32_t>> parts;
    for (std::vector<std::int32_t> & members : byRoot){
        if (!members.empty()){
            parts.push_back(std::move(members));
        }
    }

    const auto rightmost = [&cv](const std::vector<std::int32_t> & part){
        double best = -numeric_limits<double>::infinity();
        for (const std::int32_t index : part){
            best = std::max(best, cv[static_cast<std::size_t>(index)].real());
        }
        return best;
    };
    std::stable_sort(parts.begin(), parts.end(),
                     [&rightmost](const std::vector<std::int32_t> & a, const std::vector<std::int32_t> & b){
                         return rightmost(a) > rightmost(b);
                     });

    return parts;
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
