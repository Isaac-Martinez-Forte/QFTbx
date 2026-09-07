#include "src/bench/plan.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <sstream>

#include <pugixml.hpp>

#include "src/core/common/exception.h"
#include "src/core/system/parameter.h"

namespace qftbx::bench {

namespace {

const char * kRoot = "QFTbench";
const int kVersion = 1;

double realAttribute(pugi::xml_node node, const char * name, double fallback, bool required = false)
{
    const pugi::xml_attribute attribute = node.attribute(name);
    if (!attribute) {
        if (required) {
            throw InvalidInput(std::string("benchmark plan: <") + node.name() + "> needs the attribute '" + name + "'");
        }
        return fallback;
    }
    char * end = nullptr;
    const double value = std::strtod(attribute.value(), &end);
    if (end == attribute.value() || *end != '\0' || !std::isfinite(value)) {
        throw InvalidInput(std::string("benchmark plan: the attribute '") + name + "' of <" + node.name()
                           + "> is not a number: '" + attribute.value() + "'");
    }
    return value;
}

bool flagAttribute(pugi::xml_node node, const char * name, bool fallback)
{
    const double value = realAttribute(node, name, fallback ? 1.0 : 0.0);
    if (value != 0.0 && value != 1.0) {
        throw InvalidInput(std::string("benchmark plan: the attribute '") + name + "' of <" + node.name() + "> must be 0 or 1");
    }
    return value != 0.0;
}

std::string text(double value)
{
    std::ostringstream out;
    out.precision(17);
    out << value;
    return out.str();
}

} // namespace

const char * algorithmName(LoopShapingAlgorithm algorithm)
{
    switch (algorithm) {
    case nt: return "nt";
    case nk: return "nk";
    case mr: return "mr";
    case mc1: return "mc1";
    case mc_thesis: return "mc_thesis";
    }
    return "unknown";
}

std::optional<LoopShapingAlgorithm> algorithmFromName(const std::string & name)
{
    for (const LoopShapingAlgorithm algorithm : {nt, nk, mr, mc1, mc_thesis}) {
        if (name == algorithmName(algorithm)) {
            return algorithm;
        }
    }
    return std::nullopt;
}

Plan readPlan(const std::string & path)
{
    pugi::xml_document document;
    const pugi::xml_parse_result parsed = document.load_file(path.c_str());
    if (parsed.status == pugi::status_file_not_found || parsed.status == pugi::status_io_error) {
        throw FileError(path + ": cannot read the benchmark plan");
    }
    if (!parsed) {
        throw InvalidInput(path + ": malformed benchmark plan: " + parsed.description());
    }

    const pugi::xml_node root = document.child(kRoot);
    if (!root) {
        throw InvalidInput(path + ": not a benchmark plan (no <" + std::string(kRoot) + "> element)");
    }
    if (static_cast<int>(realAttribute(root, "version", 0.0)) != kVersion) {
        throw InvalidInput(path + ": unsupported benchmark plan version");
    }

    Plan plan;
    plan.name = root.attribute("name").as_string(plan.name.c_str());

    plan.projectFile = root.child("project").attribute("file").as_string("");
    if (plan.projectFile.empty()) {
        throw InvalidInput(path + ": the plan names no project file (<project file=...>)");
    }
    plan.outputDirectory = root.child("output").attribute("directory").as_string(plan.outputDirectory.c_str());

    const pugi::xml_node execution = root.child("execution");
    if (execution) {
        plan.jobs = static_cast<int>(realAttribute(execution, "jobs", 0.0));
        plan.timeoutSeconds = realAttribute(execution, "timeout-seconds", 0.0);
        plan.memoryLimitMegabytes = static_cast<int>(realAttribute(execution, "memory-limit-megabytes", 0.0));
    }

    const pugi::xml_node measure = root.child("measure");
    if (measure) {
        plan.measures.time = flagAttribute(measure, "time", true);
        plan.measures.cpu = flagAttribute(measure, "cpu", true);
        plan.measures.memory = flagAttribute(measure, "memory", true);
        plan.measures.memoryTrace = flagAttribute(measure, "memory-trace", false);
        plan.measures.counters = flagAttribute(measure, "counters", true);
    }

    const pugi::xml_node repetitions = root.child("repetitions");
    if (repetitions) {
        plan.repetitions = static_cast<int>(realAttribute(repetitions, "count", 1.0));
        plan.warmUp = flagAttribute(repetitions, "warm-up", false);
    }
    if (plan.repetitions < 1) {
        throw InvalidInput(path + ": the repetitions count must be at least 1");
    }

    for (pugi::xml_node node : root.child("algorithms").children("algorithm")) {
        const std::string name = node.attribute("name").as_string("");
        const std::optional<LoopShapingAlgorithm> algorithm = algorithmFromName(name);
        if (!algorithm) {
            throw InvalidInput(path + ": unknown algorithm '" + name + "' (nt, nk, mr, mc1, mc_thesis)");
        }
        plan.algorithms.push_back(*algorithm);
    }
    if (plan.algorithms.empty()) {
        throw InvalidInput(path + ": the plan names no algorithm");
    }

    for (pugi::xml_node node : root.child("epsilons").children("epsilon")) {
        const double epsilon = realAttribute(node, "value", 0.0, true);
        if (epsilon <= 0.0) {
            throw InvalidInput(path + ": an epsilon must be positive");
        }
        plan.epsilons.push_back(epsilon);
    }
    if (plan.epsilons.empty()) {
        throw InvalidInput(path + ": the plan names no epsilon");
    }

    const pugi::xml_node structures = root.child("structures");
    if (structures) {
        plan.runBase = flagAttribute(structures, "run-base", true);
        const pugi::xml_node gain = structures.child("gain");
        if (gain) {
            plan.gainRange = Range(realAttribute(gain, "min", 0.0, true), realAttribute(gain, "max", 0.0, true));
        }
        for (pugi::xml_node node : structures.children("step")) {
            StructureStep step;
            const std::string add = node.attribute("add").as_string("");
            if (add == "zero") {
                step.kind = StructureStep::Kind::Zero;
            } else if (add == "pole") {
                step.kind = StructureStep::Kind::Pole;
            } else {
                throw InvalidInput(path + ": a <step> adds 'zero' or 'pole', not '" + add + "'");
            }
            step.min = realAttribute(node, "min", 0.0, true);
            step.max = realAttribute(node, "max", 0.0, true);
            if (step.min > step.max) {
                std::swap(step.min, step.max);
            }
            step.run = flagAttribute(node, "run", true);
            plan.steps.push_back(step);
        }
    }
    if (measuredStructures(plan).empty()) {
        throw InvalidInput(path + ": the plan runs no structure (run-base is 0 and no step runs)");
    }

    for (pugi::xml_node node : root.child("settings").children("setting")) {
        const std::string key = node.attribute("key").as_string("");
        const std::string value = node.attribute("value").as_string("");
        if (key.empty() || value.empty()) {
            throw InvalidInput(path + ": a <setting> needs a key and a value");
        }
        plan.settings.emplace_back(key, value);
    }

    return plan;
}

void writePlan(const Plan & plan, const std::string & path)
{
    pugi::xml_document document;
    pugi::xml_node declaration = document.prepend_child(pugi::node_declaration);
    declaration.append_attribute("version") = "1.0";
    declaration.append_attribute("encoding") = "UTF-8";

    pugi::xml_node root = document.append_child(kRoot);
    root.append_attribute("version") = kVersion;
    root.append_attribute("name") = plan.name.c_str();

    root.append_child("project").append_attribute("file") = plan.projectFile.c_str();
    root.append_child("output").append_attribute("directory") = plan.outputDirectory.c_str();

    pugi::xml_node execution = root.append_child("execution");
    execution.append_attribute("jobs") = plan.jobs;
    execution.append_attribute("timeout-seconds") = text(plan.timeoutSeconds).c_str();
    execution.append_attribute("memory-limit-megabytes") = plan.memoryLimitMegabytes;

    pugi::xml_node measure = root.append_child("measure");
    measure.append_attribute("time") = plan.measures.time ? 1 : 0;
    measure.append_attribute("cpu") = plan.measures.cpu ? 1 : 0;
    measure.append_attribute("memory") = plan.measures.memory ? 1 : 0;
    measure.append_attribute("memory-trace") = plan.measures.memoryTrace ? 1 : 0;
    measure.append_attribute("counters") = plan.measures.counters ? 1 : 0;

    pugi::xml_node repetitions = root.append_child("repetitions");
    repetitions.append_attribute("count") = plan.repetitions;
    repetitions.append_attribute("warm-up") = plan.warmUp ? 1 : 0;

    pugi::xml_node algorithms = root.append_child("algorithms");
    for (const LoopShapingAlgorithm algorithm : plan.algorithms) {
        algorithms.append_child("algorithm").append_attribute("name") = algorithmName(algorithm);
    }

    pugi::xml_node epsilons = root.append_child("epsilons");
    for (const double epsilon : plan.epsilons) {
        epsilons.append_child("epsilon").append_attribute("value") = text(epsilon).c_str();
    }

    pugi::xml_node structures = root.append_child("structures");
    structures.append_attribute("run-base") = plan.runBase ? 1 : 0;
    if (plan.gainRange) {
        pugi::xml_node gain = structures.append_child("gain");
        gain.append_attribute("min") = text(plan.gainRange->min).c_str();
        gain.append_attribute("max") = text(plan.gainRange->max).c_str();
    }
    for (const StructureStep & step : plan.steps) {
        pugi::xml_node node = structures.append_child("step");
        node.append_attribute("add") = step.kind == StructureStep::Kind::Zero ? "zero" : "pole";
        node.append_attribute("min") = text(step.min).c_str();
        node.append_attribute("max") = text(step.max).c_str();
        node.append_attribute("run") = step.run ? 1 : 0;
    }

    pugi::xml_node settings = root.append_child("settings");
    for (const auto & [key, value] : plan.settings) {
        pugi::xml_node node = settings.append_child("setting");
        node.append_attribute("key") = key.c_str();
        node.append_attribute("value") = value.c_str();
    }

    if (!document.save_file(path.c_str(), "    ")) {
        throw FileError(path + ": cannot write the benchmark plan");
    }
}

Plan examplePlan()
{
    Plan plan;
    plan.name = "example";
    plan.projectFile = "qft_toolbox_ex2.qft";
    plan.outputDirectory = "results";
    plan.jobs = 0;
    plan.timeoutSeconds = 3600.0;
    plan.repetitions = 5;
    plan.warmUp = true;
    plan.algorithms = {nt, nk, mc1, mc_thesis};
    plan.epsilons = {2.0};
    plan.runBase = true;
    plan.steps = {
        {StructureStep::Kind::Zero, 0.01, 1000.0, true},
        {StructureStep::Kind::Pole, 0.01, 1000.0, true},
    };
    plan.settings = {{"stability.base-grid-points", "3000"}};
    return plan;
}

std::vector<std::size_t> measuredStructures(const Plan & plan)
{
    std::vector<std::size_t> structures;
    if (plan.runBase) {
        structures.push_back(0);
    }
    for (std::size_t i = 0; i < plan.steps.size(); ++i) {
        if (plan.steps[i].run) {
            structures.push_back(i + 1);
        }
    }
    return structures;
}

std::vector<Case> expandCases(const Plan & plan)
{
    std::vector<Case> cases;
    //Structures outermost, so that a plan cut short has whole rows of the
    //thesis table rather than a column of every row.
    for (const std::size_t stepsApplied : measuredStructures(plan)) {
        for (const LoopShapingAlgorithm algorithm : plan.algorithms) {
            for (const double epsilon : plan.epsilons) {
                const int first = plan.warmUp ? 0 : 1;
                for (int repetition = first; repetition <= plan.repetitions; ++repetition) {
                    Case c;
                    c.index = cases.size();
                    c.stepsApplied = stepsApplied;
                    c.algorithm = algorithm;
                    c.epsilon = epsilon;
                    c.repetition = repetition;
                    c.warmUp = repetition == 0;
                    cases.push_back(c);
                }
            }
        }
    }
    return cases;
}

std::string caseId(const Case & c)
{
    std::ostringstream out;
    out << "s" << c.stepsApplied << "-" << algorithmName(c.algorithm) << "-e" << c.epsilon
        << "-r" << c.repetition;
    std::string id = out.str();
    std::replace(id.begin(), id.end(), '.', '_');
    std::replace(id.begin(), id.end(), '+', 'p');
    return id;
}

std::string structureLabel(const Plan & plan, std::size_t stepsApplied)
{
    std::string label = "base";
    for (std::size_t i = 0; i < stepsApplied && i < plan.steps.size(); ++i) {
        label += plan.steps[i].kind == StructureStep::Kind::Zero ? "+z" : "+p";
    }
    return label;
}

std::unique_ptr<LtiSystem> structureAfter(const Plan & plan, LtiSystem & base, std::size_t stepsApplied)
{
    if (base.type() != LtiSystem::SystemType::ZeroPoleGain) {
        throw InvalidInput("benchmark: the structure sequence needs a zero-pole-gain base controller");
    }
    if (stepsApplied > plan.steps.size()) {
        throw InvalidInput("benchmark: the plan has fewer steps than asked for");
    }

    std::vector<Parameter> numerator = base.numerator();
    std::vector<Parameter> denominator = base.denominator();

    const auto freshName = [&](const char * prefix, std::size_t count) {
        for (std::size_t n = count + 1;; ++n) {
            const std::string candidate = prefix + std::to_string(n);
            const auto named = [&](const Parameter & p) { return p.name() == candidate; };
            if (std::none_of(numerator.begin(), numerator.end(), named)
                    && std::none_of(denominator.begin(), denominator.end(), named)
                    && base.gain().name() != candidate) {
                return candidate;
            }
        }
    };

    for (std::size_t i = 0; i < stepsApplied; ++i) {
        const StructureStep & step = plan.steps[i];
        const Range range(step.min, step.max);
        if (step.kind == StructureStep::Kind::Zero) {
            numerator.emplace_back(freshName("z", numerator.size()), range, range.min);
        } else {
            denominator.emplace_back(freshName("p", denominator.size()), range, range.min);
        }
    }

    Parameter gain = base.gain();
    if (plan.gainRange) {
        gain = Parameter(base.gain().name(), *plan.gainRange, plan.gainRange->min);
    }

    return base.create(base.name(), std::move(numerator), std::move(denominator), gain, base.delay());
}

} // namespace qftbx::bench
