/**
 * @file
 * @brief The parser behind the project reader.
 *
 * A project may be saved at any point of a design, so a missing part is not
 * an error: a section or element that is not there is simply not read and
 * its step stays undone, while content that is there and broken refuses the
 * file with its line number; the two are separate exceptions. Numeric lists
 * are space-separated reals rejected whole on a bad token. The columns of a
 * specification are stored per phase column as an interval count and the
 * ends of each interval, which may be infinite, and must cover the phase
 * grid exactly. The seven specification slots are positional, and one that
 * cannot be read whole stays unused without taking the others down. Only the
 * worst excess of the verifier's check is stored, so it comes back unitemised.
 */

#include <limits>
#include <cmath>
#include <fstream>
#include <algorithm>
#include <cstdlib>
#include <optional>
#include <string>
#include <vector>
#include <cstdint>
#include "src/persistence/project_reader.h"

#include "src/core/common/text_tokens.h"
#include "src/core/math/point.h"

#include <pugixml.hpp>
#include <iterator>

#include "src/core/common/exception.h"
#include "src/core/specifications/specification.h"
#include "src/core/system/free_form.h"
#include "src/core/system/parameter.h"
#include "src/core/system/polynomial_form.h"
#include "src/core/system/time_constant_gain.h"
#include "src/core/system/zero_pole_gain.h"
#include "src/persistence/qft_dialect.h"

namespace qftbx {

class ProjectFileParser
{
public:
    ProjectFileParser(const std::string & filePath, const std::string & raw, const Tags & tags)
        : m_filePath(filePath), m_raw(raw), t(tags) {}

    std::int64_t lineOf(const pugi::xml_node & node) const
    {
        const std::ptrdiff_t offset = node.offset_debug();
        if (offset < 0 || static_cast<std::size_t>(offset) > m_raw.size()) {
            return 0;
        }

        return 1 + std::count(m_raw.begin(),
                              m_raw.begin() + static_cast<std::ptrdiff_t>(offset), '\n');
    }

    [[noreturn]] void fail(const pugi::xml_node & node, const Message & what) const
    {
        throw ParseError(what, lineOf(node), m_filePath);
    }

    [[noreturn]] void absent(const pugi::xml_node & node, const Message & what) const
    {
        throw MissingPart(what, lineOf(node), m_filePath);
    }

    pugi::xml_node require(const pugi::xml_node & parent, const char * name) const
    {
        const pugi::xml_node node = parent.child(name);
        if (!node) {
            absent(parent, QFTBX_TR("Core", "missing <%1> element").arg(name));
        }
        return node;
    }

    double realText(const pugi::xml_node & node) const
    {
        char * end = nullptr;
        const char * raw = node.text().get();
        const double value = std::strtod(raw, &end);
        const bool ok = end != nullptr && end != raw && *end == '\0';
        if (!ok) {
            fail(node, QFTBX_TR("Core", "<%1> is not a number").arg(node.name()));
        }
        return value;
    }

    double realChild(const pugi::xml_node & parent, const char * name) const
    {
        return realText(require(parent, name));
    }

    bool boolChild(const pugi::xml_node & parent, const char * name) const
    {
        const pugi::xml_node node = require(parent, name);
        const std::string text = node.text().get();
        if (text == ("true")) {
            return true;
        }
        if (text == ("false")) {
            return false;
        }
        fail(node, QFTBX_TR("Core", "<%1> is not a boolean").arg(name));
    }

    double realAttribute(const pugi::xml_node & node, const char * name) const
    {
        const pugi::xml_attribute attribute = node.attribute(name);
        if (!attribute) {
            absent(node, QFTBX_TR("Core", "missing attribute '%1'").arg(name));
        }
        char * end = nullptr;
        const char * raw = attribute.value();
        const double value = std::strtod(raw, &end);
        if (end == nullptr || end == raw || *end != '\0') {
            fail(node, QFTBX_TR("Core", "attribute '%1' is not a number").arg(name));
        }
        return value;
    }

    std::int32_t intAttribute(const pugi::xml_node & node, const char * name) const
    {
        const pugi::xml_attribute attribute = node.attribute(name);
        if (!attribute) {
            absent(node, QFTBX_TR("Core", "missing attribute '%1'").arg(name));
        }
        char * end = nullptr;
        const char * raw = attribute.value();
        const long parsed = std::strtol(raw, &end, 10);
        const std::int32_t value = static_cast<std::int32_t>(parsed);
        const bool ok = end != nullptr && end != raw && *end == '\0'
                && parsed >= INT32_MIN && parsed <= INT32_MAX;
        if (!ok) {
            fail(node, QFTBX_TR("Core", "attribute '%1' is not an integer").arg(name));
        }
        return value;
    }

    std::vector <double> realVector(const pugi::xml_node & node) const
    {
        const std::optional<std::vector<double>> values =
                qftbx::text::reals(node.text().get());

        if (!values.has_value()) {
            fail(node, QFTBX_TR("Core", "<%1> holds a non-numeric token").arg(node.name()));
        }

        return values.value();
    }

    std::vector<bool> boolVector(const pugi::xml_node & node) const
    {
        const std::vector <double> reals = realVector(node);
        std::vector<bool> bools;
        bools.reserve(static_cast<std::size_t>(reals.size()));
        for (const double value : reals) {
            bools.push_back(value != 0.0);
        }
        return bools;
    }

    qftbx::BoundaryColumns columnsOf(const pugi::xml_node & node, std::int32_t phaseCount, qftbx::Range phaseRange) const
    {
        const std::vector<double> reals = realVector(node);
        std::vector<std::vector<qftbx::BoundaryColumns::Span>> columns;
        std::size_t i = 0;
        while (i < reals.size()) {
            const double count = reals.at(i++);
            if (!(count >= 0.0) || count != std::floor(count) || i + 2 * static_cast<std::size_t>(count) > reals.size()) {
                fail(node, QFTBX_TR("Core", "<%1> holds a malformed column list").arg(node.name()));
            }
            std::vector<qftbx::BoundaryColumns::Span> spans;
            for (std::size_t k = 0; k < static_cast<std::size_t>(count); ++k, i += 2) {
                if (std::isnan(reals.at(i)) || std::isnan(reals.at(i + 1)) || reals.at(i) > reals.at(i + 1)) {
                    fail(node, QFTBX_TR("Core", "<%1> holds a malformed column list").arg(node.name()));
                }
                spans.push_back({reals.at(i), reals.at(i + 1)});
            }
            columns.push_back(std::move(spans));
        }
        if (columns.size() != static_cast<std::size_t>(phaseCount)) {
            fail(node, QFTBX_TR("Core", "<%1> does not cover the phase grid").arg(node.name()));
        }
        return qftbx::BoundaryColumns(std::move(columns), phaseCount, phaseRange);
    }

    qftbx::Trace pointVector(const pugi::xml_node & node) const
    {
        const std::vector <double> reals = realVector(node);
        if (reals.size() % 2 != 0) {
            fail(node, QFTBX_TR("Core", "<%1> holds an odd point list").arg(node.name()));
        }
        qftbx::Trace points;
        points.reserve(static_cast<std::size_t>(reals.size() / 2));
        for (std::size_t i = 0; i + 1 < reals.size(); i += 2) {
            points.push_back(qftbx::NicholsPoint(reals.at(i), reals.at(i + 1)));
        }
        return points;
    }

    Parameter readParameter(const pugi::xml_node & node) const
    {
        const double nominal = realChild(node, t.nominal);
        const bool uncertain = boolChild(node, t.uncertain);

        if (!uncertain) {
            return Parameter(nominal);
        }

        const std::string name = std::string(require(node, t.parameterName).text().get());
        const std::string expression = std::string(require(node, t.parameterExpression).text().get());
        const pugi::xml_node range = require(node, t.range);

        return Parameter(name, Range(realChild(range, t.rangeMin),
                                       realChild(range, t.rangeMax)),
                         nominal, expression);
    }

    std::unique_ptr<LtiSystem> readSystem(const pugi::xml_node & systemNode) const
    {
        const std::string name = std::string(systemNode.attribute(t.nameAttribute).value());

        const pugi::xml_node typeNode = require(systemNode, t.type);
        const auto type = static_cast<LtiSystem::SystemType>(intAttribute(typeNode, t.typeAttribute));

        const pugi::xml_node expressionNode = require(typeNode, t.expression);
        std::string numeratorExpression;
        std::string denominatorExpression;
        if (intAttribute(expressionNode, "size") == 2) {
            numeratorExpression = std::string(require(expressionNode, t.numerator).text().get());
            denominatorExpression = std::string(require(expressionNode, t.denominator).text().get());
        }

        std::vector <Parameter> numerator;
        for (const pugi::xml_node & child : require(typeNode, t.numerator).children()) {
            numerator.push_back(readParameter(child));
        }

        std::vector <Parameter> denominator;
        for (const pugi::xml_node & child : require(typeNode, t.denominator).children()) {
            denominator.push_back(readParameter(child));
        }

        std::vector <Parameter> scalars;
        for (const pugi::xml_node & child : typeNode.children()) {
            if (child.child(t.nominal)) {
                scalars.push_back(readParameter(child));
            }
        }
        if (scalars.size() != 2) {
            absent(typeNode, QFTBX_TR("Core", "expected exactly a gain and a delay parameter"));
        }
        Parameter gain = scalars.at(0);
        Parameter delay = scalars.at(1);

        std::unique_ptr<LtiSystem> system;

        switch (type) {
        case LtiSystem::SystemType::PolynomialForm:
            system = std::make_unique<PolynomialForm>(name, std::move(numerator),
                    std::move(denominator), std::move(gain), std::move(delay));
            break;
        case LtiSystem::SystemType::ZeroPoleGain:
            system = std::make_unique<ZeroPoleGain>(name, std::move(numerator),
                    std::move(denominator), std::move(gain), std::move(delay));
            break;
        case LtiSystem::SystemType::TimeConstantGain:
            system = std::make_unique<TimeConstantGain>(name, std::move(numerator),
                    std::move(denominator), std::move(gain), std::move(delay));
            break;
        case LtiSystem::SystemType::FreeForm:
            system = std::make_unique<FreeForm>(name, std::move(numerator),
                    std::move(denominator), std::move(gain), std::move(delay),
                    numeratorExpression, denominatorExpression);
            break;
        default:
            fail(typeNode, QFTBX_TR("Core", "unknown system type"));
        }

        system->setDescription(std::string(systemNode.attribute(t.descriptionAttribute).value()));

        return system;
    }

    qftbx::SpecificationRecords readSpecifications(const pugi::xml_node & section) const
    {
        qftbx::SpecificationRecords specifications;
        std::size_t slot = 0;

        for (const pugi::xml_node & node : section.children(t.specification)) {
            if (slot >= kSpecificationCount) {
                break;
            }

            qftbx::SpecificationRecord & record = specifications.at(slot++);

            try {
                record.name = std::string(node.attribute(t.nameAttribute).value());
                record.used = boolChild(node, t.used);

                if (record.used) {
                    record.omegaStart = realChild(node, t.minFrequency);
                    record.omegaEnd = realChild(node, t.maxFrequency);

                    if (const pugi::xml_node skipped = node.child(t.skipped)) {
                        record.skipped = realVector(skipped);
                    }

                    record.constant = boolChild(node, t.constant);

                    if (record.constant) {
                        record.height = realChild(node, t.magnitude);
                    } else {
                        pugi::xml_node systemNode;
                        for (const pugi::xml_node & child : node.children()) {
                            if (child.child(t.type)) {
                                systemNode = child;
                                break;
                            }
                        }
                        if (!systemNode) {
                            absent(node, QFTBX_TR("Core", "a non-constant specification needs its plant"));
                        }
                        record.system = readSystem(systemNode);
                    }
                }
            } catch (const MissingPart &) {
                record = qftbx::SpecificationRecord{};
            }
        }

        return specifications;
    }

    std::unique_ptr<Omega> readOmega(const pugi::xml_node & section) const
    {
        const double min = realChild(section, t.omegaMin);
        const double max = realChild(section, t.omegaMax);
        const double storedCount = realChild(section, t.pointCount);
        const std::int32_t pointCount = std::isfinite(storedCount)
                ? static_cast<std::int32_t>(std::clamp(storedCount, 0.0,
                          static_cast<double>(std::numeric_limits<std::int32_t>::max())))
                : 0;

        const double storedType = realChild(section, t.omegaType);
        if (storedType != static_cast<double>(Omega::LinSpace) &&
                storedType != static_cast<double>(Omega::LogSpace) &&
                storedType != static_cast<double>(Omega::Manual) &&
                storedType != static_cast<double>(Omega::File)) {
            fail(section, QFTBX_TR("Core", "the frequency set has an unknown generation type"));
        }
        const auto type = static_cast<Omega::GenerationType>(
            static_cast<std::int32_t>(storedType));

        return std::make_unique<Omega>(min, max, pointCount,
                                      realVector(require(section, t.values)), type);
    }

    qftbx::CloudSet readComplexVectors(const pugi::xml_node & section) const
    {
        qftbx::CloudSet vectors;

        pugi::xml_node child = section.first_child();
        while (child) {
            const pugi::xml_node imaginaryNode = child.next_sibling();
            if (!imaginaryNode) {
                fail(child, QFTBX_TR("Core", "a complex vector needs real and imaginary parts"));
            }

            const std::vector <double> reals = realVector(child);
            const std::vector <double> imaginaries = realVector(imaginaryNode);
            if (reals.size() != imaginaries.size()) {
                fail(child, QFTBX_TR("Core", "real and imaginary parts differ in length"));
            }

            qftbx::ComplexCloud vector;
            vector.reserve(static_cast<std::size_t>(reals.size()));
            for (std::size_t i = 0; i < reals.size(); ++i) {
                vector.push_back(std::complex<double>(reals.at(i), imaginaries.at(i)));
            }

            vectors.push_back(std::move(vector));
            child = imaginaryNode.next_sibling();
        }

        return vectors;
    }

    qftbx::TraceSet readTraces(const pugi::xml_node & section) const
    {
        qftbx::TraceSet traces;
        for (const pugi::xml_node & child : section.children()) {
            traces.push_back(pointVector(child));
        }
        return traces;
    }

    BoundaryData readBoundaries(const pugi::xml_node & section)
    {
        const pugi::xml_node data = require(section, t.boundariesData);

        const pugi::xml_node phases = require(data, t.phases);
        const std::int32_t phaseCount = intAttribute(phases, t.phaseCountAttribute);
        const qftbx::Range phaseRange(realChild(phases, t.axisMin), realChild(phases, t.axisMax));

        const pugi::xml_node magnitudes = require(data, t.magnitudes);
        const std::int32_t magnitudeCount = intAttribute(magnitudes, t.magnitudeCountAttribute);
        const qftbx::Range magnitudeRange(realChild(magnitudes, t.axisMin), realChild(magnitudes, t.axisMax));

        const pugi::xml_node metadata = require(data, t.metadata);
        std::vector<bool> openFlags = boolVector(require(metadata, t.openFlags));
        std::vector<bool> upperFlags = boolVector(require(metadata, t.upperFlags));

        qftbx::BoundarySet boundaries;
        for (const pugi::xml_node & frequencyNode : require(data, t.perFrequency).children()) {
            std::map<std::string, qftbx::TraceSet> map;
            for (const pugi::xml_node & keyNode : frequencyNode.children()) {
                map[std::string(keyNode.name())] = readTraces(keyNode);
            }
            boundaries.push_back(std::move(map));
        }

        qftbx::ColumnSet columns;
        for (const pugi::xml_node & frequencyNode : metadata.child(t.boundaryColumns).children()) {
            std::map<std::string, qftbx::BoundaryColumns> map;
            for (const pugi::xml_node & keyNode : frequencyNode.children()) {
                map[std::string(keyNode.name())] = columnsOf(keyNode, phaseCount, phaseRange);
            }
            columns.push_back(std::move(map));
        }

        qftbx::UnionTraces unionBoundaries = readTraces(require(data, t.boundaryUnion));

        qftbx::UnionBuckets unionBuckets;
        for (const pugi::xml_node & frequencyNode : require(data, t.unionBuckets).children()) {
            unionBuckets.push_back(readTraces(frequencyNode));
        }

        return BoundaryData(std::move(boundaries), std::move(openFlags), std::move(upperFlags),
                            phaseCount, phaseRange, std::move(unionBoundaries),
                            std::move(unionBuckets), magnitudeCount, magnitudeRange,
                            std::move(columns));
    }

    std::unique_ptr<LoopShapingResult> readLoopShaping(const pugi::xml_node & section) const
    {
        const pugi::xml_node data = require(section, t.boundariesData);
        const double pointCount = realAttribute(data, t.loopShapingPointCountAttribute);
        const qftbx::Range range(realChild(data, t.axisMin), realChild(data, t.axisMax));

        pugi::xml_node systemNode;
        for (const pugi::xml_node & child : section.children()) {
            if (child.child(t.type)) {
                systemNode = child;
                break;
            }
        }
        if (!systemNode) {
            absent(section, QFTBX_TR("Core", "the loop-shaping section needs its controller"));
        }

        auto result = std::make_unique<LoopShapingResult>(readSystem(systemNode), range, pointCount);

        if (const pugi::xml_node checkNode = section.child(t.check)) {
            SpecificationCheck check;
            if (checkNode.attribute("worst-excess-db")) {
                check.worstExcessDb = realAttribute(checkNode, "worst-excess-db");
            }
            result->setCheck(std::move(check));
        }

        return result;
    }

    const std::string & m_filePath;
    const std::string & m_raw;
    const Tags & t;
};

ProjectReader::ProjectReader() = default;

ProjectReader::~ProjectReader() = default;

ProjectReader::Loaded ProjectReader::load(const std::string & filePath)
{
    m_name.clear();
    m_description.clear();
    m_plant.reset();
    m_specifications.reset();
    m_omega.reset();
    m_templates.clear();
    m_contour.clear();
    m_epsilon.reset();
    m_epsilonMetric = EpsilonMetric{};
    m_boundaries.reset();
    m_controller.reset();
    m_loopShaping.reset();

    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        throw FileError(QFTBX_TR("Core", "Cannot open project file: %1").arg(filePath));
    }
    file.seekg(0, std::ios::end);
    std::string raw;
    raw.resize(static_cast<std::size_t>(file.tellg()));
    file.seekg(0, std::ios::beg);
    file.read(&raw[0], static_cast<std::streamsize>(raw.size()));
    raw.resize(static_cast<std::size_t>(file.gcount()));
    file.close();

    pugi::xml_document document;
    const pugi::xml_parse_result result = document.load_buffer(raw.data(), raw.size(),
                                                               pugi::parse_default, pugi::encoding_utf8);
    if (!result) {
        const std::size_t upTo = std::min(static_cast<std::size_t>(result.offset), raw.size());
        const std::int64_t line = 1 + std::count(raw.begin(), raw.begin()
                                                 + static_cast<std::ptrdiff_t>(upTo), '\n');
        throw ParseError(Message::plain(result.description()), line, filePath);
    }

    const pugi::xml_node root = document.document_element();
    if (std::string(root.name()) != ("QFT")) {
        throw ParseError(QFTBX_TR("Core", "not a QFT project file (root <%1>)").arg(root.name()), 1, filePath);
    }

    const int version = root.attribute("version").as_int(0);
    if (version != kVersion) {
        throw ParseError(version == 0
                         ? QFTBX_TR("Core", "unsupported .qft version (no version attribute; this build reads version %1)")
                               .arg(kVersion)
                         : QFTBX_TR("Core", "unsupported .qft version (found %1, this build reads version %2)")
                               .arg(version).arg(kVersion),
                         1, filePath);
    }

    ProjectFileParser parser(filePath, raw, kV4);
    const Tags & t = kV4;

    m_name = std::string(root.attribute(t.nameAttribute).value());
    m_description = std::string(root.attribute(t.descriptionAttribute).value());

    const pugi::xml_node inputs = root.child(t.inputs);
    const pugi::xml_node settings = root.child(t.settings);
    const pugi::xml_node results = root.child(t.results);

    const auto part = [](auto && read) {
        try {
            read();
        } catch (const MissingPart &) {
        }
    };

    bool hasContour = false;

    part([&] {
        if (const pugi::xml_node section = inputs.child(t.plant)) {
            m_plant = parser.readSystem(section);
        }
    });
    part([&] {
        if (const pugi::xml_node section = inputs.child(t.specifications)) {
            m_specifications = parser.readSpecifications(section);
        }
    });
    part([&] {
        if (const pugi::xml_node section = inputs.child(t.omega)) {
            m_omega = parser.readOmega(section);
        }
    });
    pugi::xml_node epsilonHolder = settings.child(t.templates);

    if (!epsilonHolder) {
        epsilonHolder = results.child(t.templates).child(t.metadata);
    }

    part([&] {
        if (const pugi::xml_node section = epsilonHolder) {
            const pugi::xml_node epsilonNode = parser.require(section, t.epsilon);
            m_epsilon = parser.realVector(epsilonNode);
            m_epsilonMetric = EpsilonMetric{};

            const std::string metric = epsilonNode.attribute("metric").value();
            if (metric == "nichols") {
                m_epsilonMetric.metric = HullMetric::Nichols;
            } else if (!metric.empty() && metric != "complex") {
                throw ParseError(QFTBX_TR("Core", "unknown epsilon metric '%1' (complex or nichols)").arg(metric), 1, filePath);
            }
            if (const pugi::xml_attribute weight = epsilonNode.attribute("db-per-degree")) {
                const double value = weight.as_double(0.0);
                if (!(value > 0.0) || !std::isfinite(value)) {
                    throw ParseError(QFTBX_TR("Core", "the decibels per degree of the epsilon metric must be a finite positive number"), 1, filePath);
                }
                m_epsilonMetric.dbPerDegree = value;
            }
        }
    });
    part([&] {
        if (const pugi::xml_node section = results.child(t.templates)) {
            m_templates = parser.readComplexVectors(parser.require(section, t.fullTemplates));
            if (const pugi::xml_node contourNode = section.child(t.templateContour)) {
                m_contour = parser.readComplexVectors(contourNode);
                hasContour = true;
            }
        }
    });
    part([&] {
        if (const pugi::xml_node section = results.child(t.boundaries)) {
            m_boundaries = parser.readBoundaries(section);
        }
    });
    part([&] {
        if (const pugi::xml_node section = inputs.child(t.controller)) {
            m_controller = parser.readSystem(section);
        }
    });
    part([&] {
        if (const pugi::xml_node section = results.child(t.loopShaping)) {
            m_loopShaping = parser.readLoopShaping(section);

            if (const pugi::xml_node runNode = settings.child(t.loopShaping)) {
                LoopShapingResult::Run run;
                const std::string name = runNode.attribute("algorithm").value();
                if (const std::optional<LoopShapingAlgorithm> algorithm = algorithmFromName(name)) {
                    run.algorithm = *algorithm;
                } else if (!name.empty()) {
                    throw ParseError(QFTBX_TR("Core", "unknown loop-shaping algorithm '%1'").arg(name), 1, filePath);
                }
                run.epsilon = runNode.attribute("tolerance").as_double(0.0);
                run.conservativeColumns = std::string(runNode.attribute("columns").value()) == "conservative";
                m_loopShaping->setRun(run);
            }
        }
    });

    Loaded loaded;

    if (m_plant != nullptr)                { loaded.steps.add(qftbx::Step::Plant); }
    if (m_specifications.has_value())      { loaded.steps.add(qftbx::Step::Specifications); }
    if (m_omega != nullptr)                { loaded.steps.add(qftbx::Step::Frequencies); }
    if (!m_templates.empty())              { loaded.steps.add(qftbx::Step::Templates); }
    if (m_boundaries.has_value())          { loaded.steps.add(qftbx::Step::Boundaries); }
    if (m_controller != nullptr)           { loaded.steps.add(qftbx::Step::Controller); }
    if (m_loopShaping != nullptr)          { loaded.steps.add(qftbx::Step::LoopShaping); }

    loaded.hasContour = hasContour;

    return loaded;
}

}
