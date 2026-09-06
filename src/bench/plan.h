#ifndef QFTBX_BENCH_PLAN_H
#define QFTBX_BENCH_PLAN_H

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "src/core/loopshaping/loop_shaping_types.h"
#include "src/core/math/range.h"
#include "src/core/system/lti_system.h"

/**
 * @brief A benchmark plan: what to measure, on which project, how often.
 *
 * A plan names a project file whose templates and boundaries are already
 * computed, a controller structure sequence, the algorithms and epsilons
 * to run, how many repetitions, and what to measure. It expands into
 * cases, one per (structure, algorithm, epsilon, repetition), and every
 * case runs in a process of its own (see Runner).
 *
 * The structure sequence is the thesis' way of measuring: the project's
 * controller structure is the base, and each step adds a zero or a pole
 * with its search range; the plan says after which steps to run. The plan
 * lives in an XML file the planner writes and this reads.
 */
namespace qftbx::bench {

/// What a run records. Wall time, CPU time and peak memory are read once,
/// at the end, and cost nothing; the memory trace samples the process every
/// few milliseconds from another thread and is off unless asked for, so it
/// cannot disturb a timing; the counters are the algorithm's own.
struct Measures
{
    bool time = true;
    bool cpu = true;
    bool memory = true;
    bool memoryTrace = false;
    bool counters = true;
};

/// One addition to the controller structure.
struct StructureStep
{
    enum class Kind { Zero, Pole };
    Kind kind = Kind::Zero;
    double min = 0.0;
    double max = 0.0;
    /// Whether the cases are run after this step.
    bool run = true;
};

struct Plan
{
    std::string name = "benchmark";
    std::string projectFile;
    std::string outputDirectory = ".";

    /// Processes at once; 0 means one less than the machine has cores.
    int jobs = 0;
    /// A case killed after this long; 0 means never.
    double timeoutSeconds = 0.0;
    /// Address-space limit of a case, in megabytes; 0 means none.
    int memoryLimitMegabytes = 0;

    Measures measures;

    int repetitions = 1;
    /// One extra run per case that is recorded but excluded from the
    /// statistics: the first run pays for what the others find cached.
    bool warmUp = false;

    std::vector<LoopShapingAlgorithm> algorithms;
    std::vector<double> epsilons;

    /// Whether the project's own structure, before any step, is run.
    bool runBase = true;
    /// A replacement for the base structure's gain range.
    std::optional<Range> gainRange;
    std::vector<StructureStep> steps;

    /// Settings overrides as (dotted key, value), e.g.
    /// ("stability.base-grid-points", "3000").
    std::vector<std::pair<std::string, std::string>> settings;
};

/// Reads a plan file; throws qftbx::FileError when it cannot be read and
/// qftbx::InvalidInput when it is malformed.
Plan readPlan(const std::string & path);
void writePlan(const Plan & plan, const std::string & path);

/// A plan with every element present, commented by its element names, to
/// start from.
Plan examplePlan();

/// One measured run.
struct Case
{
    std::size_t index = 0;
    /// How many steps of the structure sequence are applied.
    std::size_t stepsApplied = 0;
    LoopShapingAlgorithm algorithm = nt;
    double epsilon = 0.0;
    int repetition = 0;
    bool warmUp = false;
};

/// The structures the plan runs, as numbers of steps applied, in order.
std::vector<std::size_t> measuredStructures(const Plan & plan);

/// Every case of the plan: structures × algorithms × epsilons ×
/// repetitions (plus the warm-up), in the order they are queued.
std::vector<Case> expandCases(const Plan & plan);

/// A file-name-safe identifier of a case, e.g. "s2-nt-e2-r1".
std::string caseId(const Case & c);

/// "base", "base+z", "base+z+p", ...
std::string structureLabel(const Plan & plan, std::size_t stepsApplied);

/// The controller structure after the given number of steps: the base
/// with the added zeros and poles (named z1, z2, ... and p1, p2, ... after
/// the ones it has) and the gain range replaced when the plan says so.
/// Zero-pole-gain structures only.
std::unique_ptr<LtiSystem> structureAfter(const Plan & plan, LtiSystem & base, std::size_t stepsApplied);

const char * algorithmName(LoopShapingAlgorithm algorithm);
std::optional<LoopShapingAlgorithm> algorithmFromName(const std::string & name);

} // namespace qftbx::bench

#endif // QFTBX_BENCH_PLAN_H
