/**
 * @file
 * @brief The published problems and the controllers the tests of the exact
 * reading draw in them.
 *
 * The examples shipped with the program, one by its name or all of them in
 * the order of their names. A parameter is drawn log-uniformly over its
 * range when the range is positive and uniformly when it is not, and a
 * certain parameter is its nominal value and draws nothing. A random
 * controller draws its zeros, its poles and its gain in that order, and a
 * controller inside a box draws each of them uniformly over its range. A
 * random box of controllers is centred on a draw of each parameter, zeros,
 * poles and gain, and spans a relative width around it inside the box of the
 * structure. The controller a design stands for is its nominal values. One
 * test binary includes this file, so everything in it is inline.
 */

#ifndef QFTBX_TESTS_PUBLISHED_PROBLEMS_H
#define QFTBX_TESTS_PUBLISHED_PROBLEMS_H

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include "src/core/loopshaping/common/point_controller.h"
#include "src/core/math/range.h"
#include "src/core/system/lti_system.h"
#include "src/core/system/parameter.h"

namespace qftbx_tests {

inline std::string example(const char * name)
{
    return (std::filesystem::path(QFTBX_EXAMPLES_DIR) / name).string();
}

inline std::vector<std::string> allExamples()
{
    std::vector<std::string> files;
    if (!std::filesystem::exists(QFTBX_EXAMPLES_DIR)) {
        return files;
    }
    for (const std::filesystem::directory_entry & entry : std::filesystem::directory_iterator(QFTBX_EXAMPLES_DIR)) {
        if (entry.path().extension() == ".qft") {
            files.push_back(entry.path().string());
        }
    }
    std::sort(files.begin(), files.end());
    return files;
}

inline double drawIn(const qftbx::Parameter & parameter, std::mt19937 & generator)
{
    if (!parameter.isUncertain()) {
        return parameter.nominal();
    }
    std::uniform_real_distribution<double> unit(0.0, 1.0);
    const qftbx::Range range = parameter.range();
    if (range.min > 0.0) {
        return std::exp(std::log(range.min) + unit(generator) * (std::log(range.max) - std::log(range.min)));
    }
    return range.min + unit(generator) * (range.max - range.min);
}

inline qftbx::PointController randomPoint(qftbx::LtiSystem & box, std::mt19937 & generator)
{
    qftbx::PointController point;
    for (const qftbx::Parameter & parameter : box.numerator()) point.zeros.push_back(drawIn(parameter, generator));
    for (const qftbx::Parameter & parameter : box.denominator()) point.poles.push_back(drawIn(parameter, generator));
    point.gain = drawIn(box.gain(), generator);
    return point;
}

inline qftbx::PointController pointInside(qftbx::LtiSystem & box, std::mt19937 & generator)
{
    std::uniform_real_distribution<double> unit(0.0, 1.0);
    const auto draw = [&](const qftbx::Parameter & parameter) {
        const qftbx::Range range = parameter.range();
        return range.min + unit(generator) * (range.max - range.min);
    };
    qftbx::PointController point;
    for (const qftbx::Parameter & parameter : box.numerator()) point.zeros.push_back(draw(parameter));
    for (const qftbx::Parameter & parameter : box.denominator()) point.poles.push_back(draw(parameter));
    point.gain = draw(box.gain());
    return point;
}

inline std::unique_ptr<qftbx::LtiSystem> randomBox(qftbx::LtiSystem & structure, double relativeWidth, std::mt19937 & generator)
{
    const auto around = [&](const qftbx::Parameter & parameter) {
        const double centre = drawIn(parameter, generator);
        const qftbx::Range within = parameter.range();
        return qftbx::Range(std::max(within.min, centre / (1.0 + relativeWidth)),
                            std::min(within.max, centre * (1.0 + relativeWidth)));
    };
    std::vector<qftbx::Parameter> zeros, poles;
    for (const qftbx::Parameter & z : structure.numerator()) {
        zeros.emplace_back(z.name(), around(z), z.range().min);
    }
    for (const qftbx::Parameter & p : structure.denominator()) {
        poles.emplace_back(p.name(), around(p), p.range().min);
    }
    const qftbx::Range gain = around(structure.gain());
    return structure.create("box", zeros, poles, qftbx::Parameter(std::string("k"), gain, gain.min), qftbx::Parameter(0.0));
}

inline qftbx::PointController designOf(qftbx::LtiSystem & design)
{
    qftbx::PointController point;
    for (const qftbx::Parameter & z : design.numerator()) point.zeros.push_back(z.nominal());
    for (const qftbx::Parameter & p : design.denominator()) point.poles.push_back(p.nominal());
    point.gain = design.gain().nominal();
    return point;
}

}

#endif
