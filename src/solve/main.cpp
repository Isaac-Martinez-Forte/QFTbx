// qftbx-solve: the seven phases of a design from a command line.
//
// It takes a project that carries its inputs - the plant and its
// uncertainty, the design frequencies, the specifications and the structure
// the controller is looked for in - and writes the same project with every
// result in it: the templates, their contours, the boundaries, the
// controller the search returned and what the verifier says about it.
//
// It is what the interface does, without the interface: the facade it drives
// is the one the windows drive, and this tool links no Qt at all. That is
// the point of it as much as the batteries of examples it exists to build -
// a design that can be run from a script can be run from anywhere else.
//
// What it does not take from the project it takes from the settings file,
// the same one the application reads: how many points a parameter is swept
// at, and the Nichols grid the boundaries are computed over. The epsilon of
// each contour it works out itself.

#include <cmath>
#include <cstring>
#include <iostream>
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
    int points = 0;          //0: whatever the settings say
    bool quiet = false;
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
        "  -p, --points N        points per uncertain parameter (default: the settings')\n"
        "  -c, --config FILE     the settings file to read instead of the usual ones\n"
        "  -q, --quiet           say nothing but the errors\n";
}

//Every uncertain parameter of the plant swept at the same number of points
//over its own range, which is what the templates dialog offers before
//anybody touches it. A name that appears twice - the same 'a' in the
//numerator and the denominator - is one grid, as the engine reads it.
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
        } else if (argument == "-c" || argument == "--config") {
            into.configuration = value("--config");
        } else if (argument == "-q" || argument == "--quiet") {
            into.quiet = true;
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

} // namespace

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
        const int points = options.points > 0 ? options.points
                                              : int(settings.defaults.templatePointCount);

        qftbx::openRecord(settings);

        qftbx::ProjectController project;
        project.applySettings(settings);
        project.load(options.input);

        if (project.plant() == nullptr || project.omega() == nullptr) {
            throw qftbx::InvalidInput(options.input + ": the project has no plant or no frequencies");
        }

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
                      << cloud << " points, contours " << contour << "\n";
        }

        const qftbx::Range phase(settings.defaults.phaseStart, settings.defaults.phaseEnd);
        const qftbx::Range magnitude(settings.defaults.magnitudeStart, settings.defaults.magnitudeEnd);

        if (!project.computeBoundaries(phase, settings.defaults.phasePoints,
                                       magnitude, settings.defaults.magnitudePoints,
                                       -1.0, true, false)) {
            throw qftbx::InvalidInput("the boundaries came out empty");
        }

        if (!options.quiet) {
            std::cout << "boundaries: over " << settings.defaults.phasePoints << " x "
                      << settings.defaults.magnitudePoints << " points of the Nichols plane\n";
        }

        if (!project.computeLoopShaping(options.tolerance, options.algorithm,
                                        qftbx::Range(1e-9, 10.0), 100)) {
            throw qftbx::InvalidInput("the loop shaping found no controller");
        }

        qftbx::LtiSystem * controller = project.loopShapingResult()->controller();
        if (controller == nullptr) {
            throw qftbx::InvalidInput("the loop shaping returned no controller");
        }

        const qftbx::SpecificationCheck check = qftbx::checkAgainstSpecifications(
            *controller, *project.plant(), *project.omega()->values(),
            project.templates(), qftbx::toSpecificationSet(*project.specifications()));

        project.save(options.output);

        if (!options.quiet) {
            std::cout << "controller (" << qftbx::algorithmName(options.algorithm) << "): gain "
                      << controller->gain().range().min << "\n"
                      << "verifier: worst excess " << check.worstExcessDb << " dB over "
                      << check.entries.size() << " checks, "
                      << (check.satisfied() ? "every specification met" : "A SPECIFICATION IS EXCEEDED")
                      << "\n"
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
