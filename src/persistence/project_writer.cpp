#include <cstdint>
#include "project_writer.h"

#include <cstdio>
#include <string>

#include <QMap>
#include "src/core/point.h"

#include <pugixml.hpp>

#include "src/core/exception.h"
#include "qft_dialect.h"
#include "src/core/system/parameter.h"
#include "src/core/text_tokens.h"

namespace qftbx {

namespace {

//The one place a real becomes text: see qftbx::text::number. This used to
//be a local "%.17g", which wrote noise digits by the thousand
//(0.00010155800000000001 for a value whose exact short form is
//0.000101558) - 17669 of them in the shipped fixtures alone.
using qftbx::text::number;

const Tags & t = kV2;

//17 significant digits: enough for an exact double round trip.

std::string realVectorText(const QVector <double> & values)
{
    std::string text;
    foreach (double value, values) {
        text += number(value) + " ";
    }
    return text;
}

std::string pointVectorText(const std::vector<qftbx::NicholsPoint> & points)
{
    std::string text;
    for (const qftbx::NicholsPoint & point : points) {
        text += number(point.phase) + " " + number(point.magnitude) + " ";
    }
    return text;
}

std::string boolVectorText(const std::vector<bool> & values)
{
    std::string text;
    for (const bool value : values) {
        text += value ? "1 " : "0 ";
    }
    return text;
}

void addText(pugi::xml_node parent, const char * name, const std::string & text)
{
    parent.append_child(name).text().set(text.c_str());
}

void addReal(pugi::xml_node parent, const char * name, double value)
{
    addText(parent, name, number(value));
}

void addBool(pugi::xml_node parent, const char * name, bool value)
{
    parent.append_child(name).text().set(value ? "true" : "false");
}

void writeParameter(pugi::xml_node parent, Parameter & parameter)
{
    pugi::xml_node node = parent.append_child("parameter");
    addReal(node, t.nominal, parameter.nominal());
    addBool(node, t.uncertain, parameter.isUncertain());

    if (parameter.isUncertain()) {
        addText(node, t.parameterName, parameter.name().toStdString());
        addText(node, t.parameterExpression, parameter.expression().toStdString());
        pugi::xml_node range = node.append_child(t.range);
        addReal(range, t.rangeMin, parameter.range().min);
        addReal(range, t.rangeMax, parameter.range().max);
    }
}

void writeSystem(pugi::xml_node parent, const char * sectionName, LtiSystem * system)
{
    pugi::xml_node node = parent.append_child(sectionName);
    node.append_attribute(t.nameAttribute) = system->name().toStdString().c_str();

    pugi::xml_node typeNode = node.append_child(t.type);
    typeNode.append_attribute(t.typeAttribute) = static_cast<std::int32_t>(system->type());

    pugi::xml_node expression = typeNode.append_child(t.expression);
    if (system->type() == LtiSystem::SystemType::FreeForm) {
        expression.append_attribute("size") = 2;
        addText(expression, t.numerator, system->numeratorString().toStdString());
        addText(expression, t.denominator, system->denominatorString().toStdString());
    } else {
        expression.append_attribute("size") = 0;
    }

    pugi::xml_node numerator = typeNode.append_child(t.numerator);
    numerator.append_attribute("size") = system->numerator().size();
    for (Parameter & parameter : system->numerator()) {
        writeParameter(numerator, parameter);
    }

    pugi::xml_node denominator = typeNode.append_child(t.denominator);
    denominator.append_attribute("size") = system->denominator().size();
    for (Parameter & parameter : system->denominator()) {
        writeParameter(denominator, parameter);
    }

    writeParameter(typeNode, system->gain());
    writeParameter(typeNode, system->delay());
}

void writeSpecifications(pugi::xml_node root, const qftbx::SpecificationRecords * specifications)
{
    pugi::xml_node section = root.append_child(t.specifications);
    section.append_attribute("count") = static_cast<int>(specifications->size());

    for (const qftbx::SpecificationRecord & record : *specifications) {
        pugi::xml_node node = section.append_child(t.specification);
        node.append_attribute(t.nameAttribute) = record.name.toStdString().c_str();
        addBool(node, t.used, record.used);

        if (!record.used) {
            continue;
        }

        addReal(node, t.minFrequency, record.omegaStart);
        addReal(node, t.maxFrequency, record.omegaEnd);
        addBool(node, t.constant, record.constant);

        if (record.constant) {
            addReal(node, t.magnitude, record.height);
        } else {
            writeSystem(node, "system", record.system.get());
        }
    }
}

void writeOmega(pugi::xml_node root, Omega * omega)
{
    pugi::xml_node section = root.append_child(t.omega);
    addReal(section, t.omegaMin, omega->start());
    addReal(section, t.omegaMax, omega->end());
    addText(section, t.pointCount, std::to_string(omega->pointCount()));
    addText(section, t.omegaType, std::to_string(static_cast<std::int32_t>(omega->type())));
    addText(section, t.values, realVectorText(*omega->values()));
}

void writeComplexVectors(pugi::xml_node section, const qftbx::CloudSet & vectors)
{
    for (const qftbx::ComplexCloud & vector : vectors) {
        std::string reals;
        std::string imaginaries;
        for (const std::complex<double> & value : vector) {
            reals += number(value.real()) + " ";
            imaginaries += number(value.imag()) + " ";
        }
        addText(section, "re", reals);
        addText(section, "im", imaginaries);
    }
}

void writeTemplates(pugi::xml_node root, const ProjectContent & content)
{
    pugi::xml_node section = root.append_child(t.templates);

    pugi::xml_node metadata = section.append_child(t.metadata);
    addText(metadata, t.epsilon,
            content.epsilon != nullptr ? realVectorText(*content.epsilon) : std::string());

    pugi::xml_node full = section.append_child(t.fullTemplates);
    full.append_attribute("size") = static_cast<std::int64_t>(content.templates.size());
    writeComplexVectors(full, content.templates);

    if (!content.contour.empty()) {
        pugi::xml_node contour = section.append_child(t.templateContour);
        contour.append_attribute("size") = static_cast<std::int64_t>(content.contour.size());
        writeComplexVectors(contour, content.contour);
    }
}

void writeTraces(pugi::xml_node parent, const qftbx::TraceSet & traces)
{
    for (const qftbx::Trace & trace : traces) {
        addText(parent, "trace", pointVectorText(trace));
    }
}

void writeBoundaries(pugi::xml_node root, BoundaryData * boundaries)
{
    pugi::xml_node section = root.append_child(t.boundaries);
    pugi::xml_node data = section.append_child(t.boundariesData);

    pugi::xml_node phases = data.append_child(t.phases);
    phases.append_attribute(t.phaseCountAttribute) = boundaries->phaseCount();
    addReal(phases, t.axisMin, boundaries->phaseRange().min);
    addReal(phases, t.axisMax, boundaries->phaseRange().max);

    pugi::xml_node magnitudes = data.append_child(t.magnitudes);
    magnitudes.append_attribute(t.magnitudeCountAttribute) = boundaries->magnitudeCount();
    addReal(magnitudes, t.axisMin, boundaries->magnitudeRange().min);
    addReal(magnitudes, t.axisMax, boundaries->magnitudeRange().max);

    pugi::xml_node metadata = data.append_child(t.metadata);
    addText(metadata, t.openFlags, boolVectorText(boundaries->openFlags()));
    addText(metadata, t.upperFlags, boolVectorText(boundaries->upperFlags()));

    pugi::xml_node perFrequency = data.append_child(t.perFrequency);
    perFrequency.append_attribute("size") = static_cast<std::int64_t>(boundaries->boundaries().size());
    for (const auto & map : boundaries->boundaries()) {
        pugi::xml_node frequency = perFrequency.append_child("frequency");
        frequency.append_attribute("size") = static_cast<std::int64_t>(map.size());

        //std::map iterates in key order, as QMap::keys() did.
        for (const auto & entry : map) {
            pugi::xml_node keyNode = frequency.append_child(entry.first.toStdString().c_str());
            keyNode.append_attribute("size") = static_cast<std::int64_t>(entry.second.size());
            writeTraces(keyNode, entry.second);
        }
    }

    pugi::xml_node unionNode = data.append_child(t.boundaryUnion);
    unionNode.append_attribute("size") = static_cast<std::int64_t>(boundaries->unionBoundaries().size());
    writeTraces(unionNode, boundaries->unionBoundaries());

    pugi::xml_node buckets = data.append_child(t.unionBuckets);
    buckets.append_attribute("size") = static_cast<std::int64_t>(boundaries->unionBuckets().size());
    for (const qftbx::TraceSet & perFrequencyBuckets : boundaries->unionBuckets()) {
        pugi::xml_node frequency = buckets.append_child("frequency");
        frequency.append_attribute("size") = static_cast<std::int64_t>(perFrequencyBuckets.size());
        writeTraces(frequency, perFrequencyBuckets);
    }
}

void writeLoopShaping(pugi::xml_node root, LoopShapingResult * loopShaping)
{
    pugi::xml_node section = root.append_child(t.loopShaping);

    pugi::xml_node data = section.append_child(t.boundariesData);
    data.append_attribute(t.loopShapingPointCountAttribute) = loopShaping->pointCount();
    addReal(data, t.axisMin, loopShaping->range().min);
    addReal(data, t.axisMax, loopShaping->range().max);

    writeSystem(section, t.controller, loopShaping->controller());
}

} // namespace

void ProjectWriter::save(const QString & filePath, const ProjectContent & content)
{
    pugi::xml_document document;
    pugi::xml_node declaration = document.append_child(pugi::node_declaration);
    declaration.append_attribute("version") = "1.0";
    declaration.append_attribute("encoding") = "UTF-8";

    pugi::xml_node root = document.append_child("QFT");
    root.append_attribute("version") = 2;

    if (content.plant != nullptr) {
        writeSystem(root, t.plant, content.plant);
    }
    if (content.specifications != nullptr) {
        writeSpecifications(root, content.specifications);
    }
    if (content.omega != nullptr) {
        writeOmega(root, content.omega);
    }
    if (!content.templates.empty()) {
        writeTemplates(root, content);
    }
    if (content.boundaries != nullptr) {
        writeBoundaries(root, content.boundaries);
    }
    if (content.controller != nullptr) {
        writeSystem(root, t.controller, content.controller);
    }
    if (content.loopShaping != nullptr) {
        writeLoopShaping(root, content.loopShaping);
    }

    if (!document.save_file(filePath.toUtf8().constData(), "    ",
                            pugi::format_default, pugi::encoding_utf8)) {
        throw FileError("Cannot write project file: " + filePath.toStdString());
    }
}

} // namespace qftbx
