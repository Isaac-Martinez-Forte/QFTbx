#include "project_reader.h"

#include <QByteArray>
#include <QFile>
#include <QPointF>
#include <QMap>

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
    ProjectFileParser(const QString & filePath, const QByteArray & raw, const Tags & tags)
        : m_filePath(filePath), m_raw(raw), t(tags) {}

    [[noreturn]] void fail(const pugi::xml_node & node, const std::string & what) const
    {
        qint64 line = 0;
        const ptrdiff_t offset = node.offset_debug();
        if (offset >= 0 && offset <= m_raw.size()) {
            line = 1 + QByteArray(m_raw.constData(), offset).count('\n');
        }
        throw ParseError(m_filePath.toStdString() + ": " + what, line);
    }

    pugi::xml_node require(const pugi::xml_node & parent, const char * name) const
    {
        const pugi::xml_node node = parent.child(name);
        if (!node) {
            fail(parent, std::string("missing <") + name + "> element");
        }
        return node;
    }

    qreal realText(const pugi::xml_node & node) const
    {
        bool ok = false;
        const qreal value = QString(node.text().get()).toDouble(&ok);
        if (!ok) {
            fail(node, std::string("<") + node.name() + "> is not a number");
        }
        return value;
    }

    qreal realChild(const pugi::xml_node & parent, const char * name) const
    {
        return realText(require(parent, name));
    }

    bool boolChild(const pugi::xml_node & parent, const char * name) const
    {
        const pugi::xml_node node = require(parent, name);
        const QString text = node.text().get();
        if (text == QStringLiteral("true")) {
            return true;
        }
        if (text == QStringLiteral("false")) {
            return false;
        }
        fail(node, std::string("<") + name + "> is not a boolean");
    }

    qint32 intAttribute(const pugi::xml_node & node, const char * name) const
    {
        const pugi::xml_attribute attribute = node.attribute(name);
        if (!attribute) {
            fail(node, std::string("missing attribute '") + name + "'");
        }
        bool ok = false;
        const qint32 value = QString(attribute.value()).toInt(&ok);
        if (!ok) {
            fail(node, std::string("attribute '") + name + "' is not an integer");
        }
        return value;
    }

    //Space-separated real vector, the encoding of every numeric list in the
    //format. The historical loader silently kept the prefix before the
    //first garbage token; this one rejects it.
    QVector <qreal> realVector(const pugi::xml_node & node) const
    {
        QVector <qreal> values;
        const QString text = QString(node.text().get());
        const QStringList tokens = text.split(' ', Qt::SkipEmptyParts);
        values.reserve(tokens.size());
        foreach (const QString & token, tokens) {
            bool ok = false;
            values.append(token.toDouble(&ok));
            if (!ok) {
                fail(node, std::string("<") + node.name() + "> holds a non-numeric token");
            }
        }
        return values;
    }

    std::vector<bool> boolVector(const pugi::xml_node & node) const
    {
        const QVector <qreal> reals = realVector(node);
        std::vector<bool> bools;
        bools.reserve(static_cast<std::size_t>(reals.size()));
        for (const qreal value : reals) {
            bools.push_back(value != 0.0);
        }
        return bools;
    }

    //"x y x y ..." pairs; an unpaired trailing token is rejected.
    qftbx::Trace pointVector(const pugi::xml_node & node) const
    {
        const QVector <qreal> reals = realVector(node);
        if (reals.size() % 2 != 0) {
            fail(node, std::string("<") + node.name() + "> holds an odd point list");
        }
        qftbx::Trace points;
        points.reserve(static_cast<std::size_t>(reals.size() / 2));
        for (qint32 i = 0; i + 1 < reals.size(); i += 2) {
            points.push_back(QPointF(reals.at(i), reals.at(i + 1)));
        }
        return points;
    }

    Parameter readParameter(const pugi::xml_node & node) const
    {
        const qreal nominal = realChild(node, t.nominal);
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
        const QString name = QString(require(node, t.parameterName).text().get());
        const QString expression = QString(require(node, t.parameterExpression).text().get());
        const pugi::xml_node range = require(node, t.range);

        return Parameter(name, Range(realChild(range, t.rangeMin),
                                       realChild(range, t.rangeMax)),
                         nominal, expression);
    }

    std::unique_ptr<LtiSystem> readSystem(const pugi::xml_node & systemNode) const
    {
        const QString name = QString(systemNode.attribute(t.nameAttribute).value());

        const pugi::xml_node typeNode = require(systemNode, t.type);
        const auto type = static_cast<LtiSystem::SystemType>(intAttribute(typeNode, t.typeAttribute));

        const pugi::xml_node expressionNode = require(typeNode, t.expression);
        QString numeratorExpression;
        QString denominatorExpression;
        if (intAttribute(expressionNode, "size") == 2) {
            numeratorExpression = QString(require(expressionNode, t.numerator).text().get());
            denominatorExpression = QString(require(expressionNode, t.denominator).text().get());
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
        const qint32 count = static_cast<qint32>(std::distance(slotRange.begin(), slotRange.end()));
        if (count != kSpecificationCount) {
            fail(section, "a project needs exactly 7 specification slots");
        }

        qftbx::SpecificationRecords specifications;
        std::size_t slot = 0;

        for (const pugi::xml_node & node : section.children(t.specification)) {
            qftbx::SpecificationRecord & record = specifications.at(slot++);
            record.name = modernSpecificationName(QString(node.attribute(t.nameAttribute).value()));
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
        const qreal min = realChild(section, t.omegaMin);
        const qreal max = realChild(section, t.omegaMax);
        const qint32 pointCount = static_cast<qint32>(realChild(section, t.pointCount));
        const auto type = static_cast<Omega::GenerationType>(
            static_cast<qint32>(realChild(section, t.omegaType)));
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

            const QVector <qreal> reals = realVector(child);
            const QVector <qreal> imaginaries = realVector(imaginaryNode);
            if (reals.size() != imaginaries.size()) {
                fail(child, "real and imaginary parts differ in length");
            }

            qftbx::ComplexCloud vector;
            vector.reserve(static_cast<std::size_t>(reals.size()));
            for (qint32 i = 0; i < reals.size(); ++i) {
                vector.push_back(std::complex<qreal>(reals.at(i), imaginaries.at(i)));
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
        const qint32 phaseCount = intAttribute(phases, t.phaseCountAttribute);
        const QPointF phaseRange(realChild(phases, t.axisMin), realChild(phases, t.axisMax));

        const pugi::xml_node magnitudes = require(data, t.magnitudes);
        const qint32 magnitudeCount = intAttribute(magnitudes, t.magnitudeCountAttribute);
        const QPointF magnitudeRange(realChild(magnitudes, t.axisMin), realChild(magnitudes, t.axisMax));

        const pugi::xml_node metadata = require(data, t.metadata);
        std::vector<bool> openFlags = boolVector(require(metadata, t.openFlags));
        std::vector<bool> upperFlags = boolVector(require(metadata, t.upperFlags));

        qftbx::BoundarySet boundaries;
        for (const pugi::xml_node & frequencyNode : require(data, t.perFrequency).children()) {
            std::map<QString, qftbx::TraceSet> map;
            for (const pugi::xml_node & keyNode : frequencyNode.children()) {
                //Legacy files store the historical Spanish keys.
                map[modernSpecificationName(QString(keyNode.name()))] = readTraces(keyNode);
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
        const qint32 pointCount = intAttribute(data, t.loopShapingPointCountAttribute);
        const QPointF range(realChild(data, t.axisMin), realChild(data, t.axisMax));

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

    const QString & m_filePath;
    const QByteArray & m_raw;
    const Tags & t;
};

ProjectReader::ProjectReader() = default;

namespace {

} // namespace

//Whatever no caller claimed through take*() dies with the reader, which no
//longer needs to be told: every member owns what it holds.
ProjectReader::~ProjectReader() = default;

std::vector<bool> ProjectReader::load(const QString & filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        throw FileError("Cannot open project file: " + filePath.toStdString());
    }
    const QByteArray raw = file.readAll();
    file.close();

    pugi::xml_document document;
    const pugi::xml_parse_result result = document.load_buffer(raw.constData(), raw.size(),
                                                               pugi::parse_default, pugi::encoding_utf8);
    if (!result) {
        const qint64 line = 1 + QByteArray(raw.constData(),
                                           qMin<qint64>(result.offset, raw.size())).count('\n');
        throw ParseError(filePath.toStdString() + ": " + result.description(), line);
    }

    const pugi::xml_node root = document.document_element();
    if (QString(root.name()) != QStringLiteral("QFT")) {
        throw ParseError(filePath.toStdString() + ": not a QFT project file (root <"
                         + root.name() + ">)", 1);
    }

    const bool isV2 = root.attribute("version").as_int(1) >= 2;
    ProjectFileParser parser(filePath, raw, isV2 ? kV2 : kLegacy);
    const Tags & t = isV2 ? kV2 : kLegacy;

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
    std::vector<bool> sections;
    sections.push_back(m_plant != nullptr);
    sections.push_back(m_specifications.has_value());
    sections.push_back(m_omega != nullptr);
    sections.push_back(!m_templates.empty());
    sections.push_back(m_boundaries.has_value());
    sections.push_back(m_controller != nullptr);
    sections.push_back(m_loopShaping != nullptr);
    sections.push_back(hasContour);

    return sections;
}

} // namespace qftbx
