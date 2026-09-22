/**
 * @file
 * @brief `qftbx-solve`, the seven phases of a QFT design from a command line.
 *
 * It takes a project that carries its inputs, the plant and its uncertainty,
 * the design frequencies, the specifications and the structure the
 * controller is looked for in, and writes the same project with every result
 * in it: templates, contours, boundaries, the controller the search returned
 * and the verifier's verdict on it. It drives the same facade the windows
 * drive and links no Qt. The Nichols grid comes from the options or from the
 * settings file the application reads; the points per uncertain parameter
 * default to a count that brings the whole cloud near the size asked for,
 * since a sweep costs the product over the parameters. The project is saved
 * once the boundaries exist and again after the search.
 */

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "src/app/project_controller.h"
#include "src/core/common/exception.h"
#include "src/core/loopshaping/algorithm_name.h"
#include "src/core/loopshaping/common/specification_checker.h"
#include "src/core/math/range.h"
#include "src/core/math/sequences.h"
#include "src/core/project/settings.h"
#include "src/core/specifications/specification_record.h"

namespace {

struct Options {
    std::string input;
    std::string output;
    std::string configuration;
    qftbx::LoopShapingAlgorithm algorithm = qftbx::nt;
    double tolerance = 0.5;
    int points = 0;
    int cloud = 625;
    std::string phase;
    std::string magnitude;
    bool quiet = false;
    bool loop = true;
};

void usage()
{
    std::cout <<
        "qftbx-solve - compute every phase of a QFT design\n\n"
        "  qftbx-solve <project.qft> [-o <solved.qft>] [options]\n\n"
        "  -o, --output FILE     where the solved project goes (default: the input)\n"
        "  -a, --algorithm NAME  the loop-shaping search: ";
    for (const qftbx::LoopShapingAlgorithm one : qftbx::everyAlgorithm()) {
        std::cout << qftbx::algorithmName(one) << " ";
    }
    std::cout <<
        "(default: nt)\n"
        "  -e, --epsilon X       the tolerance that search stops at (default: 0.5)\n"
        "  -n, --cloud N         points per template, over however many uncertain\n"
        "                        parameters there are (default: 625)\n"
        "  -p, --points N        points per uncertain parameter, instead of --cloud\n"
        "  -P, --phase A,B,N     the phase axis of the Nichols grid, in degrees\n"
        "  -M, --magnitude A,B,N the magnitude axis, in decibels\n"
        "  -c, --config FILE     the settings file to read instead of the usual ones\n"
        "  -q, --quiet           say nothing but the errors\n"
        "      --no-loop         stop once the boundaries are computed\n"
        "\n"
        "  The project is written as each phase is reached, so a search that finds\n"
        "  no controller still leaves the templates and the boundaries behind.\n"
        "  Exit: 0 every specification met, 1 one exceeded, 3 no controller, 2 error.\n";
}

int pointsForCloud(int cloud, std::size_t parameters)
{
    if (parameters == 0) {
        return 1;
    }

    const double each = std::pow(double(cloud), 1.0 / double(parameters));
    return std::max(2, int(std::lround(each)));
}

std::size_t uncertainCount(qftbx::LtiSystem & plant)
{
    std::vector<std::string> names;
    const auto count = [&names](const qftbx::Parameter & parameter) {
        if (parameter.isUncertain()
                && std::find(names.begin(), names.end(), parameter.name()) == names.end()) {
            names.push_back(parameter.name());
        }
    };

    for (const qftbx::Parameter & parameter : plant.numerator()) count(parameter);
    for (const qftbx::Parameter & parameter : plant.denominator()) count(parameter);
    count(plant.gain());
    count(plant.delay());

    return names.size();
}

std::string familyText(const qftbx::FamilyStability & family)
{
    if (!family.checked) {
        switch (family.notChecked) {
        case qftbx::FamilyStability::NotChecked::NoSweepRecord: return "closed-loop stability not checked: the project has no record of the sweep";
        case qftbx::FamilyStability::NotChecked::Delay:         return "closed-loop stability not checked: the loop has a delay";
        case qftbx::FamilyStability::NotChecked::NotRational:   return "closed-loop stability not checked: the loop is not a rational function";
        case qftbx::FamilyStability::NotChecked::No:            break;
        }
        return "closed-loop stability not checked";
    }
    std::ostringstream text;
    if (family.unstableMembers == 0) {
        text << "every one of the " << family.members << " plants is closed-loop stable, worst real part "
             << family.worstRealPart;
    } else {
        text << family.unstableMembers << " of the " << family.members
             << " plants are CLOSED-LOOP UNSTABLE, worst real part " << family.worstRealPart;
    }
    return text.str();
}

qftbx::ParameterGrids gridsOf(qftbx::LtiSystem & plant, int points)
{
    qftbx::ParameterGrids grids;

    const auto take = [&grids, points](const qftbx::Parameter & parameter) {
        if (!parameter.isUncertain() || grids.count(parameter.name()) != 0) {
            return;
        }
        grids[parameter.name()] = qftbx::math::linspace(parameter.range().min,
                                                       parameter.range().max,
                                                       std::size_t(points));
    };

    for (const qftbx::Parameter & parameter : plant.numerator()) take(parameter);
    for (const qftbx::Parameter & parameter : plant.denominator()) take(parameter);

    for (const qftbx::Parameter * parameter : {&plant.gain(), &plant.delay()}) {
        if (parameter->isUncertain()) {
            take(*parameter);
        } else if (!parameter->name().empty()) {
            grids[parameter->name()] = std::vector<double>(1, parameter->nominal());
        }
    }

    return grids;
}

void axisFrom(const std::string & text, const char * what,
              double & from, double & to, std::int32_t & points)
{
    if (text.empty()) {
        return;
    }

    const std::string::size_type first = text.find(',');
    const std::string::size_type second = text.find(',', first + 1);
    if (first == std::string::npos || second == std::string::npos) {
        throw qftbx::InvalidInput(std::string(what) + " takes min,max,points: '" + text + "'");
    }

    from = std::stod(text.substr(0, first));
    to = std::stod(text.substr(first + 1, second - first - 1));
    points = std::stoi(text.substr(second + 1));

    if (points < 2 || !(to > from)) {
        throw qftbx::InvalidInput(std::string(what) + " needs max above min and two points or more");
    }
}

bool readOptions(int argc, char ** argv, Options & into)
{
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        const auto value = [&](const char * what) -> std::string {
            if (i + 1 >= argc) {
                throw qftbx::InvalidInput(std::string(what) + " needs a value");
            }
            return argv[++i];
        };

        if (argument == "-h" || argument == "--help") {
            usage();
            return false;
        } else if (argument == "-o" || argument == "--output") {
            into.output = value("--output");
        } else if (argument == "-a" || argument == "--algorithm") {
            const std::string name = value("--algorithm");
            const std::optional<qftbx::LoopShapingAlgorithm> found = qftbx::algorithmFromName(name);
            if (!found.has_value()) {
                throw qftbx::InvalidInput(name + ": no algorithm of that name");
            }
            into.algorithm = *found;
        } else if (argument == "-e" || argument == "--epsilon") {
            into.tolerance = std::stod(value("--epsilon"));
        } else if (argument == "-p" || argument == "--points") {
            into.points = std::stoi(value("--points"));
        } else if (argument == "-n" || argument == "--cloud") {
            into.cloud = std::stoi(value("--cloud"));
        } else if (argument == "-P" || argument == "--phase") {
            into.phase = value("--phase");
        } else if (argument == "-M" || argument == "--magnitude") {
            into.magnitude = value("--magnitude");
        } else if (argument == "-c" || argument == "--config") {
            into.configuration = value("--config");
        } else if (argument == "-q" || argument == "--quiet") {
            into.quiet = true;
        } else if (argument == "--no-loop") {
            into.loop = false;
        } else if (!argument.empty() && argument[0] == '-') {
            throw qftbx::InvalidInput(argument + ": no such option");
        } else if (into.input.empty()) {
            into.input = argument;
        } else {
            throw qftbx::InvalidInput(argument + ": one project at a time");
        }
    }

    if (into.input.empty()) {
        usage();
        return false;
    }
    if (into.output.empty()) {
        into.output = into.input;
    }
    return true;
}

}

int main(int argc, char ** argv)
{
    Options options;

    try {
        if (!readOptions(argc, argv, options)) {
            return argc > 1 ? 0 : 2;
        }

        const qftbx::Settings settings = options.configuration.empty()
                                             ? qftbx::loadSettings()
                                             : qftbx::readSettings(options.configuration);
        qftbx::openRecord(settings);

        qftbx::ProjectController project;
        project.applySettings(settings);
        project.load(options.input);

        if (project.plant() == nullptr || project.omega() == nullptr) {
            throw qftbx::InvalidInput(options.input + ": the project has no plant or no frequencies");
        }

        const std::size_t parameters = uncertainCount(*project.plant());
        const int points = options.points > 0
                               ? options.points
                               : pointsForCloud(options.cloud, parameters);

        const qftbx::ParameterGrids grids = gridsOf(*project.plant(), points);

        std::vector<double> epsilon;
        for (const qftbx::TemplateEngine::EpsilonProposal & one
                 : project.proposeEpsilon(grids, project.epsilonMetric())) {
            epsilon.push_back(one.epsilon);
        }

        if (!project.computeTemplates(epsilon, grids, false)) {
            throw qftbx::InvalidInput("the templates came out empty");
        }

        if (!options.quiet) {
            std::size_t cloud = 0, contour = 0;
            for (const qftbx::ComplexCloud & one : project.templates()) cloud += one.size();
            for (const qftbx::ComplexCloud & one : project.contour()) contour += one.size();
            std::cout << "templates: " << project.templates().size() << " frequencies, "
                      << parameters << " uncertain parameters at " << points
                      << " points each, " << cloud << " points, contours " << contour << "\n";
        }

        double phaseFrom = settings.defaults.phaseStart;
        double phaseTo = settings.defaults.phaseEnd;
        std::int32_t phasePoints = settings.defaults.phasePoints;
        axisFrom(options.phase, "--phase", phaseFrom, phaseTo, phasePoints);

        double magnitudeFrom = settings.defaults.magnitudeStart;
        double magnitudeTo = settings.defaults.magnitudeEnd;
        std::int32_t magnitudePoints = settings.defaults.magnitudePoints;
        axisFrom(options.magnitude, "--magnitude", magnitudeFrom, magnitudeTo, magnitudePoints);

        if (!project.computeBoundaries(qftbx::Range(phaseFrom, phaseTo), phasePoints,
                                       qftbx::Range(magnitudeFrom, magnitudeTo), magnitudePoints,
                                       -1.0, true, false)) {
            throw qftbx::InvalidInput("the boundaries came out empty");
        }

        if (!options.quiet) {
            std::cout << "boundaries: " << phasePoints << " x " << magnitudePoints
                      << " over phase [" << phaseFrom << ", " << phaseTo << "] and magnitude ["
                      << magnitudeFrom << ", " << magnitudeTo << "] dB\n";
        }

        project.save(options.output);

        if (!options.loop) {
            if (!options.quiet) {
                std::cout << "written to " << options.output << ", without a controller\n";
            }
            return 3;
        }

        std::string failed;
        bool solved = false;
        try {
            solved = project.computeLoopShaping(options.tolerance, options.algorithm,
                                                qftbx::Range(1e-9, 10.0), 100);
        } catch (const qftbx::Exception & refused) {
            failed = refused.what();
        }

        qftbx::LtiSystem * controller = solved ? project.loopShapingResult()->controller() : nullptr;

        if (controller == nullptr) {
            if (!options.quiet) {
                std::cout << "no controller: " << (failed.empty() ? "the search found none" : failed)
                          << "\n"
                          << "written to " << options.output << ", up to the boundaries\n";
            }
            return 3;
        }

        if (!project.loopShapingResult()->check().has_value()) {
            project.loopShapingResult()->setCheck(qftbx::checkAgainstSpecifications(
                *controller, *project.plant(), *project.omega()->values(),
                project.templates(), qftbx::toSpecificationSet(*project.specifications()),
                &project.sweepGrids()));
        }
        const qftbx::SpecificationCheck & check = *project.loopShapingResult()->check();

        project.save(options.output);

        if (!options.quiet) {
            std::cout << "controller (" << qftbx::algorithmName(options.algorithm) << "): gain "
                      << controller->gain().range().min << "\n"
                      << "verifier: worst excess " << check.worstExcessDb << " dB over "
                      << check.entries.size() << " checks, "
                      << (check.worstExcessDb > 0.0 ? "A SPECIFICATION IS EXCEEDED" : "every specification met")
                      << "\n"
                      << "family: " << familyText(check.family) << "\n"
                      << "written to " << options.output << "\n";
        }

        return check.satisfied() ? 0 : 1;

    } catch (const qftbx::Exception & failure) {
        std::cerr << "qftbx-solve: " << failure.what() << "\n";
        return 2;
    } catch (const std::exception & failure) {
        std::cerr << "qftbx-solve: " << failure.what() << "\n";
        return 2;
    }
}
