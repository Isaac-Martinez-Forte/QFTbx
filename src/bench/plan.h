#ifndef QFTBX_BENCH_PLAN_H
#define QFTBX_BENCH_PLAN_H

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "src/core/loopshaping/algorithm_name.h"
#include "src/core/loopshaping/loop_shaping_types.h"
#include "src/core/math/range.h"
#include "src/core/pipeline/processor.h"
#include "src/core/system/lti_system.h"
#include "src/core/templates/hull_metric.h"

/**
 * @file
 * @brief A benchmark plan: what to measure, on which project, how often.
 *
 * A plan names a project file whose templates and boundaries are already
 * computed, a controller structure sequence, the algorithms and epsilons
 * to run, how many repetitions, and what to measure. It expands into
 * cases, one per (structure, algorithm, epsilon, repetition), queued in
 * that order, and every case runs in a process of its own (see Runner).
 * The warm-up run is recorded but left out of the statistics: the first
 * run pays for what the others find cached.
 *
 * A plan may also compute the phases again before the search (Phases): the
 * templates, the boundaries or both, from what the project holds or on
 * denser grids, once per processor named, and then the search or not. A
 * comparison case computes them on the processor and on the GPU in the same
 * process and measures how far apart they come out. Without phases a plan
 * measures the search alone, as it always has.
 *
 * The structure sequence is the thesis' way of measuring: the project's
 * controller structure is the base, and each step adds a zero or a pole
 * with its search range; the plan says after which steps to run. Only a
 * zero-pole-gain structure can be extended; the roots added are named
 * z1, z2, ... and p1, p2, ... after the ones the base has.
 *
 * Wall time, CPU time and peak memory are read once, at the end, and cost
 * nothing; the memory trace samples the process from another thread and
 * is off unless asked for, so that it cannot disturb a timing. A zero in
 * jobs means one process less than the machine has cores, and a zero
 * timeout or memory limit (in megabytes) means none. threads is OpenMP's
 * thread count in each case, one unless the plan says otherwise, and zero
 * leaves OpenMP the whole machine, as the program has it. Settings overrides
 * are pairs of a dotted key and a value. The plan lives in an XML file the
 * planner writes and this reads; readPlan throws FileError when the file
 * cannot be read and InvalidInput when it is malformed.
 */
namespace qftbx::bench {

struct Measures
{
    bool time = true;
    bool cpu = true;
    bool memory = true;
    bool memoryTrace = false;
    bool counters = true;
};

struct StructureStep
{
    enum class Kind { Zero, Pole };
    Kind kind = Kind::Zero;
    double min = 0.0;
    double max = 0.0;
    bool run = true;
};

struct Phases
{
    bool templates = false;
    bool boundaries = false;
    bool loopShaping = true;
    std::vector<Processor> processors{Processor::Cpu};
    bool compare = false;

    int sweepPoints = 0;
    int frequencyCount = 0;
    std::optional<EpsilonMetric> metric;
    bool proposeEpsilon = false;

    bool fromContour = true;
    int phasePoints = 0;
    int magnitudePoints = 0;
};

struct Plan
{
    std::string name = "benchmark";
    std::string projectFile;
    std::string outputDirectory = ".";

    int jobs = 0;
    int threads = 1;
    double timeoutSeconds = 0.0;
    int memoryLimitMegabytes = 0;

    Measures measures;

    int repetitions = 1;
    bool warmUp = false;

    std::vector<LoopShapingAlgorithm> algorithms;
    std::vector<double> epsilons;

    bool runBase = true;
    std::optional<Range> gainRange;
    std::vector<StructureStep> steps;

    std::vector<std::pair<std::string, std::string>> settings;

    std::optional<Phases> phases;
};

Plan readPlan(const std::string & path);
void writePlan(const Plan & plan, const std::string & path);

Plan examplePlan();

struct Case
{
    std::size_t index = 0;
    std::size_t stepsApplied = 0;
    LoopShapingAlgorithm algorithm = nt;
    double epsilon = 0.0;
    int repetition = 0;
    bool warmUp = false;

    bool withPhases = false;
    bool loopShaping = true;
    Processor processor = Processor::Cpu;
    bool comparison = false;
};

std::vector<std::size_t> measuredStructures(const Plan & plan);

std::vector<Case> expandCases(const Plan & plan);

std::string caseId(const Case & c);

const char * processorName(Processor processor);

std::string caseKind(const Case & c);

std::string structureLabel(const Plan & plan, std::size_t stepsApplied);

std::unique_ptr<LtiSystem> structureAfter(const Plan & plan, LtiSystem & base, std::size_t stepsApplied);

}

#endif
