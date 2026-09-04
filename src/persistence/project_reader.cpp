#include <limits>
#include <cmath>
#include <fstream>
#include <algorithm>
#include <cstdlib>
#include <optional>
#include <string>
#include <vector>
#include <cstdint>
#include "project_reader.h"

#include "src/core/text_tokens.h"
#include "src/core/point.h"

#include <pugixml.hpp>
#include <iterator>

#include "src/core/exception.h"
#include "src/core/specifications/specification.h"
#include "src/core/system/free_form.h"
#include "src/core/system/parameter.h"
#include "src/core/system/polynomial_form.h"
#include "src/core/system/time_constant_gain.h"
#include "src/core/system/zero_pole_gain.h"
#include "qft_dialect.h"

namespace qftbx {


//All the parsing state of one load() call, so the reader class itself only
//keeps the results.
class ProjectFileParser
{
public:
    ProjectFileParser(const std::string & filePath, const std::string & raw, const Tags & tags)
        : m_filePath(filePath), m_raw(raw), t(tags) {}

    [[noreturn]] void fail(const pugi::xml_node & node, const std::string & what) const
    {
        std::int64_t line = 0;
        const ptrdiff_t offset = node.offset_debug();
        if (offset >= 0 && offset <= m_raw.size()) {
            line = 1 + std::count(m_raw.begin(),
                                  m_raw.begin() + static_cast<std::ptrdiff_t>(offset), '\n');
        }
        throw ParseError(m_filePath + ": " + what, line);
    }

    pugi::xml_node require(const pugi::xml_node & parent, const char * name) const
    {
        const pugi::xml_node node = parent.child(name);
        if (!node) {
            fail(parent, std::string("missing <") + name + "> element");
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
            fail(node, std::string("<") + node.name() + "> is not a number");
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
        fail(node, std::string("<") + name + "> is not a boolean");
    }

    std::int32_t intAttribute(const pugi::xml_node & node, const char * name) const
    {
        const pugi::xml_attribute attribute = node.attribute(name);
        if (!attribute) {
            fail(node, std::string("missing attribute '") + name + "'");
        }
        char * end = nullptr;
        const char * raw = attribute.value();
        const long parsed = std::strtol(raw, &end, 10);
        const std::int32_t value = static_cast<std::int32_t>(parsed);
        const bool ok = end != nullptr && end != raw && *end == '\0'
                && parsed >= INT32_MIN && parsed <= INT32_MAX;
        if (!ok) {
            fail(node, std::string("attribute '") + name + "' is not an integer");
        }
        return value;
    }

    //Space-separated real vector, the encoding of every numeric list in the
    //format. The historical loader silently kept the prefix before the
    //first garbage token; this one rejects it.
    std::vector <double> realVector(const pugi::xml_node & node) const
    {
        //text::reals answers exactly this, whole-token validation and all:
        //this loop was a second implementation of the same rule.
        const std::optional<std::vector<double>> values =
                qftbx::text::reals(node.text().get());

        if (!values.has_value()) {
            fail(node, std::string("<") + node.name() + "> holds a non-numeric token");
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

    //"x y x y ..." pairs; an unpaired trailing token is rejected.
    qftbx::Trace pointVector(const pugi::xml_node & node) const
    {
        const std::vector <double> reals = realVector(node);
        if (reals.size() % 2 != 0) {
            fail(node, std::string("<") + node.name() + "> holds an odd point list");
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
            Parameter parameter(nominal);
            //Historical quirk kept for compatibility: some old controller
            //records carry a range on a non-uncertain parameter.
            const pugi::xml_node range = node.child(t.range);
            if (range) {
                parameter.setRange(Range(realChild(range, t.rangeMin),
                                           realChild(range, t.rangeMax)));
            }
            return parameter;
        }

        //The parameter name shares the tag with the section-name field in
        //the legacy dialect ("nombre"): resolve it positionally after
        //'uncertain', which every dialect writes before it.
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

        //Gain and delay: the two parameter elements that are direct children
        //of <type> (the legacy dialect names them variable-k/variable-ret).
        std::vector <Parameter> scalars;
        for (const pugi::xml_node & child : typeNode.children()) {
            if (child.child(t.nominal)) {
                scalars.push_back(readParameter(child));
            }
        }
        if (scalars.size() != 2) {
            fail(typeNode, "expected exactly a gain and a delay parameter");
        }
        Parameter gain = scalars.at(0);
        Parameter delay = scalars.at(1);

        switch (type) {
        case LtiSystem::SystemType::PolynomialForm:
            return std::make_unique<PolynomialForm>(name, std::move(numerator),
                    std::move(denominator), std::move(gain), std::move(delay));
        case LtiSystem::SystemType::ZeroPoleGain:
            return std::make_unique<ZeroPoleGain>(name, std::move(numerator),
                    std::move(denominator), std::move(gain), std::move(delay));
        case LtiSystem::SystemType::TimeConstantGain:
            return std::make_unique<TimeConstantGain>(name, std::move(numerator),
                    std::move(denominator), std::move(gain), std::move(delay));
        case LtiSystem::SystemType::FreeForm:
            return std::make_unique<FreeForm>(name, std::move(numerator),
                    std::move(denominator), std::move(gain), std::move(delay),
                    numeratorExpression, denominatorExpression);
        default:
            break;
        }
        fail(typeNode, "unknown system type");
    }

    qftbx::SpecificationRecords readSpecifications(const pugi::xml_node & section) const
    {
        //The set is positional with 7 fixed slots: consumers index blindly,
        //and the type now carries that count.
        const auto slotRange = section.children(t.specification);
        const std::int32_t count = static_cast<std::int32_t>(std::distance(slotRange.begin(), slotRange.end()));
        if (count != kSpecificationCount) {
            fail(section, "a project needs exactly 7 specification slots");
        }

        qftbx::SpecificationRecords specifications;
        std::size_t slot = 0;

        for (const pugi::xml_node & node : section.children(t.specification)) {
            qftbx::SpecificationRecord & record = specifications.at(slot++);
            //Canonical as stored: version 2 writes the English names, and
            //the translation from the Spanish ones went with the dialect.
            record.name = std::string(node.attribute(t.nameAttribute).value());
            record.used = boolChild(node, t.used);

            if (record.used) {
                record.omegaStart = realChild(node, t.minFrequency);
                record.omegaEnd = realChild(node, t.maxFrequency);
                record.constant = boolChild(node, t.constant);

                if (record.constant) {
                    record.height = realChild(node, t.magnitude);
                } else {
                    //The embedded plant is the child that carries a <type>
                    //element (its tag is the plant name in the legacy
                    //dialect).
                    pugi::xml_node systemNode;
                    for (const pugi::xml_node & child : node.children()) {
                        if (child.child(t.type)) {
                            systemNode = child;
                            break;
                        }
                    }
                    if (!systemNode) {
                        fail(node, "a non-constant specification needs its plant");
                    }
                    record.system = readSystem(systemNode);
                }
            }
        }

        return specifications;
    }

    std::unique_ptr<Omega> readOmega(const pugi::xml_node & section) const
    {
        const double min = realChild(section, t.omegaMin);
        const double max = realChild(section, t.omegaMax);
        //The stored count is read and passed on, but Omega ignores it and
        //recomputes the count from the values (old files carry a
        //desynchronised one). It is still clamped before the conversion:
        //turning a file's "1e300" into an std::int32_t is undefined
        //behaviour, whether or not anybody reads the result.
        const double storedCount = realChild(section, t.pointCount);
        const std::int32_t pointCount = std::isfinite(storedCount)
                ? static_cast<std::int32_t>(std::clamp(storedCount, 0.0,
                          static_cast<double>(std::numeric_limits<std::int32_t>::max())))
                : 0;

        //An unknown generation type used to travel in as-is and then behave
        //as linear wherever it was compared, which hid a corrupt file
        //instead of reporting it.
        const double storedType = realChild(section, t.omegaType);
        if (storedType != static_cast<double>(Omega::LinSpace) &&
                storedType != static_cast<double>(Omega::LogSpace) &&
                storedType != static_cast<double>(Omega::Manual) &&
                storedType != static_cast<double>(Omega::File)) {
            fail(section, "the frequency set has an unknown generation type");
        }
        const auto type = static_cast<Omega::GenerationType>(
            static_cast<std::int32_t>(storedType));

        return std::make_unique<Omega>(min, max, pointCount,
                                      realVector(require(section, t.values)), type);
    }

    //Template sets are stored as (real-vector, imaginary-vector) element
    //pairs, one pair per design frequency.
    qftbx::CloudSet readComplexVectors(const pugi::xml_node & section) const
    {
        qftbx::CloudSet vectors;

        pugi::xml_node child = section.first_child();
        while (child) {
            const pugi::xml_node imaginaryNode = child.next_sibling();
            if (!imaginaryNode) {
                fail(child, "a complex vector needs real and imaginary parts");
            }

            const std::vector <double> reals = realVector(child);
            const std::vector <double> imaginaries = realVector(imaginaryNode);
            if (reals.size() != imaginaries.size()) {
                fail(child, "real and imaginary parts differ in length");
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

    //One trace per child element, each a flat "x y x y ..." list.
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
                //Legacy files store the historical Spanish keys.
                map[std::string(keyNode.name())] = readTraces(keyNode);
            }
            boundaries.push_back(std::move(map));
        }

        qftbx::UnionTraces unionBoundaries = readTraces(require(data, t.boundaryUnion));

        qftbx::UnionBuckets unionBuckets;
        for (const pugi::xml_node & frequencyNode : require(data, t.unionBuckets).children()) {
            unionBuckets.push_back(readTraces(frequencyNode));
        }

        //No takeOwnership() any more: BoundaryData holds its containers by
        //value, so there is one owner and it is the object itself.
        return BoundaryData(std::move(boundaries), std::move(openFlags), std::move(upperFlags),
                            phaseCount, phaseRange, std::move(unionBoundaries),
                            std::move(unionBuckets), magnitudeCount, magnitudeRange);
    }

    std::unique_ptr<LoopShapingResult> readLoopShaping(const pugi::xml_node & section) const
    {
        const pugi::xml_node data = require(section, t.boundariesData);
        const std::int32_t pointCount = intAttribute(data, t.loopShapingPointCountAttribute);
        const qftbx::Range range(realChild(data, t.axisMin), realChild(data, t.axisMax));

        //The embedded controller is the child that carries a <type> element.
        pugi::xml_node systemNode;
        for (const pugi::xml_node & child : section.children()) {
            if (child.child(t.type)) {
                systemNode = child;
                break;
            }
        }
        if (!systemNode) {
            fail(section, "the loop-shaping section needs its controller");
        }

        return std::make_unique<LoopShapingResult>(readSystem(systemNode), range, pointCount);
    }

    const std::string & m_filePath;
    const std::string & m_raw;
    const Tags & t;
};

ProjectReader::ProjectReader() = default;

namespace {

} // namespace

//Whatever no caller claimed through take*() dies with the reader, which no
//longer needs to be told: every member owns what it holds.
ProjectReader::~ProjectReader() = default;

ProjectReader::Loaded ProjectReader::load(const std::string & filePath)
{
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        throw FileError("Cannot open project file: " + filePath);
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
        throw ParseError(filePath + ": " + result.description(), line);
    }

    const pugi::xml_node root = document.document_element();
    if (std::string(root.name()) != ("QFT")) {
        throw ParseError(filePath + ": not a QFT project file (root <"
                         + root.name() + ">)", 1);
    }

    //Version 2 is the only format. A file without the attribute used to be
    //read as the historical Spanish dialect; that path is gone, and guessing
    //is worse than refusing - the two dialects share tag names with
    //DIFFERENT meanings (<inicio> is a range start and also an omega start,
    //<tipo> is an element in one place and an attribute in another), so a
    //wrong guess does not fail, it reads the wrong numbers.
    const int version = root.attribute("version").as_int(0);
    if (version != 2) {
        throw ParseError(filePath + ": unsupported .qft version (found " +
                         (version == 0 ? std::string("no version attribute")
                                       : std::to_string(version)) +
                         ", this build reads version 2)", 1);
    }

    ProjectFileParser parser(filePath, raw, kV2);
    const Tags & t = kV2;

    bool hasContour = false;

    if (const pugi::xml_node section = root.child(t.plant)) {
        m_plant = parser.readSystem(section);
    }
    if (const pugi::xml_node section = root.child(t.specifications)) {
        m_specifications = parser.readSpecifications(section);
    }
    if (const pugi::xml_node section = root.child(t.omega)) {
        m_omega = parser.readOmega(section);
    }
    if (const pugi::xml_node section = root.child(t.templates)) {
        m_epsilon = parser.realVector(parser.require(parser.require(section, t.metadata), t.epsilon));
        m_templates = parser.readComplexVectors(parser.require(section, t.fullTemplates));
        if (const pugi::xml_node contourNode = section.child(t.templateContour)) {
            m_contour = parser.readComplexVectors(contourNode);
            hasContour = true;
        }
    }
    if (const pugi::xml_node section = root.child(t.boundaries)) {
        m_boundaries = parser.readBoundaries(section);
    }
    if (const pugi::xml_node section = root.child(t.controller)) {
        //The historical loader stored a free-form CONTROLLER in the plant
        //slot; here every type lands in the controller.
        m_controller = parser.readSystem(section);
    }
    if (const pugi::xml_node section = root.child(t.loopShaping)) {
        m_loopShaping = parser.readLoopShaping(section);
    }

    //Section flags in the historical order (always all 8: the old error
    //paths returned 7 and consumers indexed out of range).
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

} // namespace qftbx
