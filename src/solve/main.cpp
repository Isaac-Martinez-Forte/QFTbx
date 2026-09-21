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

#include <algorithm>
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
    int points = 0;          //0: derived from the cloud asked for
    int cloud = 625;         //points per template, over however many parameters
    std::string phase;       //"min,max,points"; empty: whatever the settings say
    std::string magnitude;
    bool quiet = false;
    bool loop = true;        //false: stop once the boundaries are there
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

//Every uncertain parameter of the plant swept at the same number of points
//over its own range, which is what the templates dialog offers before
//anybody touches it. A name that appears twice - the same 'a' in the
//numerator and the denominator - is one grid, as the engine reads it.
//How many points each parameter is swept at for the cloud to come out at
//about the size asked for. What a sweep costs is the PRODUCT over the
//uncertain parameters, so the same count per parameter is 625 points on a
//plant with two and nine million on a plant with five - which is not a
//slower answer, it is no answer at all.
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

//"min,max,points", or what the settings say when nothing was given. An axis
//is a choice per problem: a plant that needs a hundred decibels of gain is
//not designed on a grid that stops at sixty, and the search has nothing to
//read where the grid does not reach.
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

        //Written here and not only at the end: what the search costs is not
        //what the templates and the boundaries cost, and a problem whose
        //controller nobody finds is still a problem worth having on disc
        //with everything that was computed for it.
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
                project.templates(), qftbx::toSpecificationSet(*project.specifications())));
        }
        const qftbx::SpecificationCheck & check = *project.loopShapingResult()->check();

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
