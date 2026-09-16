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


//All the parsing state of one load() call, so the reader class itself only
//keeps the results.
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

    /// The file says something this reader cannot make sense of: broken,
    /// not unfinished.
    [[noreturn]] void fail(const pugi::xml_node & node, const Message & what) const
    {
        throw ParseError(what, lineOf(node), m_filePath);
    }

    /// Something is not in the file. Its own exception, because a project
    /// may be saved half way through a design and what is missing is then
    /// simply not read: see load().
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

    //Space-separated real vector, the encoding of every numeric list in the
    //format. A list with a garbage token is rejected whole, not truncated
    //at the token.
    std::vector <double> realVector(const pugi::xml_node & node) const
    {
        //text::reals answers exactly this, whole-token validation and all:
        //this loop was a second implementation of the same rule.
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

    //The columns of one specification: per column, the interval count and
    //then the ends of each interval ("inf" and "-inf" are ends too). The
    //list must cover exactly the grid's columns.
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

    //"x y x y ..." pairs; an unpaired trailing token is rejected.
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
            //A constant is its value: no <range> is read for one, and the
            //writer emits none.
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

        //Gain and delay: the two parameter elements that are direct children
        //of <type>.
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
        fail(typeNode, QFTBX_TR("Core", "unknown system type"));
    }

    qftbx::SpecificationRecords readSpecifications(const pugi::xml_node & section) const
    {
        //The set is positional with 7 fixed slots: consumers index blindly,
        //and the type carries that count. A file with fewer fills the ones
        //it has and leaves the rest unused - a specification nobody entered
        //is exactly an unused slot - and one with more is read up to seven.
        qftbx::SpecificationRecords specifications;
        std::size_t slot = 0;

        for (const pugi::xml_node & node : section.children(t.specification)) {
            if (slot >= kSpecificationCount) {
                break;
            }

            qftbx::SpecificationRecord & record = specifications.at(slot++);

            //And a slot that cannot be read whole stays unused, instead of
            //taking the other six down with it.
            try {
                //Canonical as stored: the English names, since the
                //translation from the Spanish ones went with the dialect.
                record.name = std::string(node.attribute(t.nameAttribute).value());
                record.used = boolChild(node, t.used);

                if (record.used) {
                    record.omegaStart = realChild(node, t.minFrequency);
                    record.omegaEnd = realChild(node, t.maxFrequency);
                    record.constant = boolChild(node, t.constant);

                    if (record.constant) {
                        record.height = realChild(node, t.magnitude);
                    } else {
                        //The embedded plant is the child that carries a
                        //<type> element.
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

        //An unknown generation type is refused: travelling in as-is, it
        //behaves as linear wherever it is compared and hides a corrupt
        //file instead of reporting it.
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

    //Template sets are stored as (real-vector, imaginary-vector) element
    //pairs, one pair per design frequency.
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
                map[std::string(keyNode.name())] = readTraces(keyNode);
            }
            boundaries.push_back(std::move(map));
        }

        //The columns are optional: files written before they were stored
        //have none, and BoundaryData rebuilds them from the traces.
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

        //No takeOwnership() any more: BoundaryData holds its containers by
        //value, so there is one owner and it is the object itself.
        return BoundaryData(std::move(boundaries), std::move(openFlags), std::move(upperFlags),
                            phaseCount, phaseRange, std::move(unionBoundaries),
                            std::move(unionBuckets), magnitudeCount, magnitudeRange,
                            std::move(columns));
    }

    std::unique_ptr<LoopShapingResult> readLoopShaping(const pugi::xml_node & section) const
    {
        const pugi::xml_node data = require(section, t.boundariesData);
        //A real, as LoopShapingResult holds it and the writer writes it: read
        //as an integer, a count with a fractional part was refused.
        const double pointCount = realAttribute(data, t.loopShapingPointCountAttribute);
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
            absent(section, QFTBX_TR("Core", "the loop-shaping section needs its controller"));
        }

        auto result = std::make_unique<LoopShapingResult>(readSystem(systemNode), range, pointCount);

        //The verdict, when the file carries one. Only the worst excess is
        //stored, so the check that comes back has no itemised entries: it
        //says whether the design satisfied its specifications and by how
        //much it missed, which is what the file was asked to remember.
        if (const pugi::xml_node checkNode = section.child(t.check)) {
            SpecificationCheck check;
            //Absent when nothing was active to exceed; the default is the
            //minus infinity the checker itself starts from.
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

//Whatever no caller claimed through take*() dies with the reader, which no
//longer needs to be told: every member owns what it holds.
ProjectReader::~ProjectReader() = default;

ProjectReader::Loaded ProjectReader::load(const std::string & filePath)
{
    //An earlier load() must not show through this one: a section absent
    //from this file leaves nothing of the previous file's behind.
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

    //A file without the attribute is refused, not guessed at: the older
    //Spanish dialect shares tag names with DIFFERENT meanings (<inicio> is
    //a range start and also an omega start, <tipo> is an element in one
    //place and an attribute in another), so a wrong guess does not fail,
    //it reads the wrong numbers.
    const int version = root.attribute("version").as_int(0);
    if (version != kVersion) {
        throw ParseError(version == 0
                         ? QFTBX_TR("Core", "unsupported .qft version (no version attribute; this build reads version %1)")
                         : QFTBX_TR("Core", "unsupported .qft version (found %1, this build reads version %2)"),
                         version, filePath);
    }

    ProjectFileParser parser(filePath, raw, kV4);
    const Tags & t = kV4;

    //The three parts of the file, any of which may be missing: a project is
    //saved at whatever point of the design it has reached.
    const pugi::xml_node inputs = root.child(t.inputs);
    const pugi::xml_node settings = root.child(t.settings);
    const pugi::xml_node results = root.child(t.results);

    //And so may any part of any of them. A plant whose uncertainty was
    //never entered, a boundary section written by a run that was stopped -
    //what is not there is simply not read, and the rest of the file comes
    //in: the user finishes what he left unfinished, which is what he would
    //have to do anyway. Only content that IS there and is broken - a number
    //that is not a number - refuses the file.
    const auto part = [](auto && read) {
        try {
            read();
        } catch (const MissingPart &) {
            //Not there. The step stays undone.
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
    //The epsilon of the templates. It lives in the settings, which is what
    //it is - the tolerance the hull walk was asked for - but the first
    //files of version 4 carried it among the results, under the templates
    //themselves. Read from there when the settings do not have it, rather
    //than leaving a project with clouds and no tolerance behind them.
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
            //Every type lands in the controller, free-form included.
            m_controller = parser.readSystem(section);
        }
    });
    part([&] {
        if (const pugi::xml_node section = results.child(t.loopShaping)) {
            m_loopShaping = parser.readLoopShaping(section);

            //What the search was run with. A file that does not say keeps
            //the defaults of the Run, and the interface shows a design whose
            //settings it cannot vouch for as such rather than inventing
            //them.
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

    //Section flags, ALWAYS all 8: consumers index into them.
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
