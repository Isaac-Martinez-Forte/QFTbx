/**
 * @file
 * @brief The clouds the tests of the contour on the card are run on.
 *
 * The examples' own clouds, in the complex plane at the project's epsilon
 * and on the Nichols plane at the one the engine proposes there, each also
 * at half and at twice it; small clouds with repeats, a single point,
 * parts that reach equally far, a cut of the phase and an epsilon too
 * small for the grid; and four lattices whose walk does not close, found
 * by search: two fall back to the relaxed walk and two run out of steps in
 * it, each once as one part and once as several. walkPlaneGap() is the
 * Hausdorff distance between two contours of one cloud on the plane of
 * their walk.
 */

#ifndef QFTBX_TESTS_GPU_WALK_CASES_H
#define QFTBX_TESTS_GPU_WALK_CASES_H

#include <algorithm>
#include <cmath>
#include <complex>
#include <filesystem>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "src/app/project_controller.h"
#include "src/core/templates/template_engine.h"
#include "src/core/templates/walk_cloud.h"

namespace qftbx::gputest {

struct WalkCase
{
    std::string name;
    CloudSet clouds;
    std::vector<double> epsilon;
    HullMetric metric;
};

inline std::vector<std::filesystem::path> exampleFiles()
{
    std::vector<std::filesystem::path> files;
    for (const auto & entry : std::filesystem::directory_iterator(QFTBX_EXAMPLES_DIR)) {
        if (entry.path().extension() == ".qft") {
            files.push_back(entry.path());
        }
    }
    std::sort(files.begin(), files.end());
    return files;
}

inline std::vector<WalkCase> exampleCases(std::vector<double> scales)
{
    std::vector<WalkCase> cases;
    for (const std::filesystem::path & file : exampleFiles()) {
        ProjectController project;
        project.load(file.string());
        const CloudSet clouds = project.templates();
        const std::vector<double> projectEpsilon = *project.epsilon();

        TemplateEngine proposing;
        proposing.setHullMetric(HullMetric::Nichols, 1.0);
        proposing.setClouds(clouds);
        std::vector<double> nicholsEpsilon;
        for (const EpsilonProposal & proposal : proposing.proposeEpsilon()) {
            nicholsEpsilon.push_back(proposal.epsilon);
        }

        for (const double scale : scales) {
            std::vector<double> complexAt = projectEpsilon;
            std::vector<double> nicholsAt = nicholsEpsilon;
            for (double & e : complexAt) {
                e *= scale;
            }
            for (double & e : nicholsAt) {
                e *= scale;
            }
            const std::string suffix = " x" + std::to_string(scale);
            cases.push_back({file.stem().string() + " complex" + suffix, clouds, complexAt, HullMetric::ComplexPlane});
            cases.push_back({file.stem().string() + " Nichols" + suffix, clouds, nicholsAt, HullMetric::Nichols});
        }
    }
    return cases;
}

inline void addBlock(ComplexCloud & cloud, std::complex<double> corner, int side)
{
    for (int i = 0; i < side; ++i) {
        for (int j = 0; j < side; ++j) {
            cloud.push_back(corner + std::complex<double>(i, j));
        }
    }
}

inline std::vector<WalkCase> smallCases()
{
    ComplexCloud repeats{{3.0, 0.0}, {0.0, 1.0}, {-1.0, 0.0}, {1.0, 0.0}, {0.0, 1.0}, {2.0, 0.0}, {3.0, 0.0}};
    ComplexCloud blocks;
    addBlock(blocks, {0.0, 10.0}, 3);
    addBlock(blocks, {5.0, 0.0}, 2);
    addBlock(blocks, {0.0, 0.0}, 3);
    ComplexCloud acrossTheCut;
    for (int k = -6; k <= 6; ++k) {
        acrossTheCut.push_back(std::polar(1.0 + 0.1 * (k + 6), 3.1 + 0.01 * k));
        acrossTheCut.push_back(std::polar(2.0 + 0.1 * (k + 6), -3.1 + 0.01 * k));
    }
    ComplexCloud spread{{0.0, 0.0}, {1.0, 0.0}, {1.0, 1.0}, {1e-12, 0.0}};

    return {
        {"repeats", {repeats}, {1.5}, HullMetric::ComplexPlane},
        {"repeats on Nichols", {repeats}, {40.0}, HullMetric::Nichols},
        {"one point", {{{2.0, -1.0}}}, {1.0}, HullMetric::ComplexPlane},
        {"parts that reach equally far", {blocks}, {1.5}, HullMetric::ComplexPlane},
        {"two frequencies", {blocks, repeats}, {1.5, 0.5}, HullMetric::ComplexPlane},
        {"across the cut", {acrossTheCut}, {3.0}, HullMetric::Nichols},
        {"an epsilon too small for the grid", {spread}, {1e-12}, HullMetric::ComplexPlane},
    };
}

inline std::vector<WalkCase> fallbackCases()
{
    const ComplexCloud relaxedOne{
        {2, 0.8660254037844386}, {1.5, 1.7320508075688772}, {0, 1.7320508075688772}, {0.5, 1.7320508075688772},
        {1, 1.7320508075688772}, {1, 1.7320508075688772}, {0, 1.7320508075688772}, {0.5, 0},
        {0.5, 2.598076211353316}, {0, 0.8660254037844386}, {3.5, 0.8660254037844386}, {3, 2.598076211353316},
        {2, 0}, {2.5, 0.8660254037844386}, {3, 1.7320508075688772}, {1, 0.8660254037844386},
        {0, 2.598076211353316}, {1, 2.598076211353316}, {2.5, 0}, {0, 0}, {3, 0}};
    const ComplexCloud relaxedParts{
        {0, 0}, {2.5, 0}, {0.5, 0}, {1, 1.7320508075688772}, {1.5, 0.8660254037844386}, {1, 0.8660254037844386},
        {3, 0}, {0, 2.598076211353316}, {0.5, 1.7320508075688772}, {2.5, 2.598076211353316}, {3.5, 0},
        {0, 1.7320508075688772}, {0.5, 2.598076211353316}};
    const ComplexCloud truncatedOne{
        {1.5, 0.8660254037844386}, {2.5, 0}, {1.5, 0.8660254037844386}, {0.5, 2.598076211353316},
        {2.5, 1.7320508075688772}, {1, 1.7320508075688772}, {0.5, 2.598076211353316}, {1.5, 2.598076211353316},
        {3.5, 0.8660254037844386}, {3, 0.8660254037844386}, {1, 0.8660254037844386}, {3, 0},
        {0, 1.7320508075688772}, {3, 0.8660254037844386}, {3.5, 0}, {3.5, 0.8660254037844386},
        {0.5, 1.7320508075688772}, {2, 2.598076211353316}, {0, 0}, {2, 1.7320508075688772},
        {0.5, 1.7320508075688772}, {1.5, 0}, {3, 1.7320508075688772}, {0, 0}, {1.5, 0}, {3.5, 0},
        {2.5, 2.598076211353316}, {2.5, 2.598076211353316}, {3.5, 0}, {2, 0}, {0, 0}, {0.5, 0}, {0.5, 0},
        {1.5, 1.7320508075688772}, {0.5, 1.7320508075688772}, {0.5, 0.8660254037844386}};
    const ComplexCloud truncatedParts{
        {2.1213203435596419, -2.1213203435596428}, {-0.70710678118654768, -0.70710678118654746}, {3, 0}, {2, 0},
        {-0.70710678118654768, -0.70710678118654746}, {-1.8369701987210297e-16, -1},
        {2.1213203435596419, -2.1213203435596428}, {1.4142135623730947, -1.4142135623730954},
        {-1.8369701987210297e-16, -1}, {-5.5109105961630896e-16, -3}, {1, 0},
        {1.4142135623730951, 1.4142135623730949}, {-1, 1.2246467991473532e-16}, {1, 0},
        {-1.4142135623730954, -1.4142135623730949}, {0.70710678118654735, -0.70710678118654768},
        {-0.70710678118654768, -0.70710678118654746}, {1.8369701987210297e-16, 3},
        {2.1213203435596428, 2.1213203435596424}, {1, 0}, {-2.1213203435596424, 2.1213203435596428},
        {-5.5109105961630896e-16, -3}};

    return {
        {"relaxed, one part", {relaxedOne}, {1.0}, HullMetric::ComplexPlane},
        {"relaxed, several parts", {relaxedParts}, {1.0}, HullMetric::ComplexPlane},
        {"out of steps, one part", {truncatedOne}, {1.0}, HullMetric::ComplexPlane},
        {"out of steps, several parts", {truncatedParts}, {2.0}, HullMetric::ComplexPlane},
    };
}

inline std::vector<WalkedContour> walksOnTheProcessor(const WalkCase & c)
{
    TemplateEngine engine;
    engine.setHullMetric(c.metric, 1.0);
    std::vector<WalkedContour> walks(c.clouds.size());
    for (std::size_t i = 0; i < c.clouds.size(); ++i) {
        walks[i].contour = engine.epsilonHull(c.clouds[i], c.epsilon[i], &walks[i].fellBack, &walks[i].truncated,
                                              &walks[i].starts);
    }
    return walks;
}

inline double walkPlaneGap(const ComplexCloud & cloud, HullMetric metric, const ComplexCloud & a,
                           const ComplexCloud & b)
{
    const SortedCloud sorted = sortedCloud(cloud, metric, 1.0);
    const auto key = [](const std::complex<double> & z) { return std::make_pair(z.real(), z.imag()); };
    std::map<std::pair<double, double>, std::complex<double>> onWalk;
    for (std::size_t i = 0; i < sorted.points.size(); ++i) {
        onWalk[key(sorted.points[i])] = sorted.walk[i];
    }
    const auto farthest = [&](const ComplexCloud & from, const ComplexCloud & to) {
        double gap = 0.0;
        for (const std::complex<double> & p : from) {
            double nearest = INFINITY;
            for (const std::complex<double> & q : to) {
                nearest = std::min(nearest, std::abs(onWalk.at(key(p)) - onWalk.at(key(q))));
            }
            gap = std::max(gap, nearest);
        }
        return gap;
    };
    if (a.empty() || b.empty()) {
        return a.empty() && b.empty() ? 0.0 : INFINITY;
    }
    return std::max(farthest(a, b), farthest(b, a));
}

}

#endif
