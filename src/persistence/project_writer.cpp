#include "project_writer.h"

#include <cstdio>
#include <string>

#include <QMap>
#include <QPointF>

#include <pugixml.hpp>

#include "Modelo/Herramientas/exception.h"
#include "qft_dialect.h"
#include "src/core/system/parameter.h"

namespace qftbx {

namespace {

const Tags & t = kV2;

//17 significant digits: enough for an exact double round trip.
std::string number(qreal value)
{
    char buffer[32];
    std::snprintf(buffer, sizeof buffer, "%.17g", value);
    return buffer;
}

std::string realVectorText(const QVector <qreal> & values)
{
    std::string text;
    foreach (qreal value, values) {
        text += number(value) + " ";
    }
    return text;
}

std::string pointVectorText(const QVector <QPointF> & points)
{
    std::string text;
    foreach (const QPointF & point, points) {
        text += number(point.x()) + " " + number(point.y()) + " ";
    }
    return text;
}

std::string boolVectorText(const QVector <bool> & values)
{
    std::string text;
    foreach (bool value, values) {
        text += value ? "1 " : "0 ";
    }
    return text;
}

void addText(pugi::xml_node parent, const char * name, const std::string & text)
{
    parent.append_child(name).text().set(text.c_str());
}

void addReal(pugi::xml_node parent, const char * name, qreal value)
{
    addText(parent, name, number(value));
}

void addBool(pugi::xml_node parent, const char * name, bool value)
{
    parent.append_child(name).text().set(value ? "true" : "false");
}

void writeParameter(pugi::xml_node parent, Parameter * parameter)
{
    pugi::xml_node node = parent.append_child("parameter");
    addReal(node, t.nominal, parameter->nominal());
    addBool(node, t.uncertain, parameter->isUncertain());

    if (parameter->isUncertain()) {
        addText(node, t.parameterName, parameter->name().toStdString());
        addText(node, t.parameterExpression, parameter->expression().toStdString());
        pugi::xml_node range = node.append_child(t.range);
        addReal(range, t.rangeMin, parameter->range().x());
        addReal(range, t.rangeMax, parameter->range().y());
    }
}

void writeSystem(pugi::xml_node parent, const char * sectionName, LtiSystem * system)
{
    pugi::xml_node node = parent.append_child(sectionName);
    node.append_attribute(t.nameAttribute) = system->name().toStdString().c_str();

    pugi::xml_node typeNode = node.append_child(t.type);
    typeNode.append_attribute(t.typeAttribute) = static_cast<qint32>(system->type());

    pugi::xml_node expression = typeNode.append_child(t.expression);
    if (system->type() == LtiSystem::SystemType::FreeForm) {
        expression.append_attribute("size") = 2;
        addText(expression, t.numerator, system->numeratorString().toStdString());
        addText(expression, t.denominator, system->denominatorString().toStdString());
    } else {
        expression.append_attribute("size") = 0;
    }

    pugi::xml_node numerator = typeNode.append_child(t.numerator);
    numerator.append_attribute("size") = system->numerator()->size();
    foreach (Parameter * parameter, *system->numerator()) {
        writeParameter(numerator, parameter);
    }

    pugi::xml_node denominator = typeNode.append_child(t.denominator);
    denominator.append_attribute("size") = system->denominator()->size();
    foreach (Parameter * parameter, *system->denominator()) {
        writeParameter(denominator, parameter);
    }

    writeParameter(typeNode, system->gain());
    writeParameter(typeNode, system->delay());
}

void writeSpecifications(pugi::xml_node root, QVector <tools::dBND *> * specifications)
{
    pugi::xml_node section = root.append_child(t.specifications);
    section.append_attribute("count") = specifications->size();

    foreach (tools::dBND * record, *specifications) {
        pugi::xml_node node = section.append_child(t.specification);
        node.append_attribute(t.nameAttribute) = record->nombre.toStdString().c_str();
        addBool(node, t.used, record->utilizado);

        if (!record->utilizado) {
            continue;
        }

        addReal(node, t.minFrequency, record->frecinicio);
        addReal(node, t.maxFrequency, record->frecfinal);
        addBool(node, t.constant, record->constante);

        if (record->constante) {
            addReal(node, t.magnitude, record->altura);
        } else {
            writeSystem(node, "system", record->sistema);
        }
    }
}

void writeOmega(pugi::xml_node root, Omega * omega)
{
    pugi::xml_node section = root.append_child(t.omega);
    addReal(section, t.omegaMin, omega->getInicio());
    addReal(section, t.omegaMax, omega->getFinal());
    addText(section, t.pointCount, std::to_string(omega->getNPuntos()));
    addText(section, t.omegaType, std::to_string(static_cast<qint32>(omega->getTipo())));
    addText(section, t.values, realVectorText(*omega->getValores()));
}

void writeComplexVectors(pugi::xml_node section,
                         QVector <QVector <std::complex<qreal>> * > * vectors)
{
    foreach (QVector <std::complex<qreal>> * vector, *vectors) {
        std::string reals;
        std::string imaginaries;
        foreach (const std::complex<qreal> & value, *vector) {
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
    full.append_attribute("size") = content.templates->size();
    writeComplexVectors(full, content.templates);

    if (content.contour != nullptr) {
        pugi::xml_node contour = section.append_child(t.templateContour);
        contour.append_attribute("size") = content.contour->size();
        writeComplexVectors(contour, content.contour);
    }
}

void writeTraces(pugi::xml_node parent, QVector <QVector <QPointF> * > * traces)
{
    foreach (QVector <QPointF> * trace, *traces) {
        addText(parent, "trace", pointVectorText(*trace));
    }
}

void writeBoundaries(pugi::xml_node root, BoundaryData * boundaries)
{
    pugi::xml_node section = root.append_child(t.boundaries);
    pugi::xml_node data = section.append_child(t.boundariesData);

    pugi::xml_node phases = data.append_child(t.phases);
    phases.append_attribute(t.phaseCountAttribute) = boundaries->phaseCount();
    addReal(phases, t.axisMin, boundaries->phaseRange().x());
    addReal(phases, t.axisMax, boundaries->phaseRange().y());

    pugi::xml_node magnitudes = data.append_child(t.magnitudes);
    magnitudes.append_attribute(t.magnitudeCountAttribute) = boundaries->magnitudeCount();
    addReal(magnitudes, t.axisMin, boundaries->magnitudeRange().x());
    addReal(magnitudes, t.axisMax, boundaries->magnitudeRange().y());

    pugi::xml_node metadata = data.append_child(t.metadata);
    addText(metadata, t.openFlags, boolVectorText(*boundaries->openFlags()));
    addText(metadata, t.upperFlags, boolVectorText(*boundaries->upperFlags()));

    pugi::xml_node perFrequency = data.append_child(t.perFrequency);
    perFrequency.append_attribute("size") = boundaries->boundaries()->size();
    foreach (auto * map, *boundaries->boundaries()) {
        pugi::xml_node frequency = perFrequency.append_child("frequency");
        frequency.append_attribute("size") = map->size();
        foreach (const QString & key, map->keys()) {
            pugi::xml_node keyNode = frequency.append_child(key.toStdString().c_str());
            keyNode.append_attribute("size") = map->value(key)->size();
            writeTraces(keyNode, map->value(key));
        }
    }

    pugi::xml_node unionNode = data.append_child(t.boundaryUnion);
    unionNode.append_attribute("size") = boundaries->unionBoundaries()->size();
    writeTraces(unionNode, boundaries->unionBoundaries());

    pugi::xml_node buckets = data.append_child(t.unionBuckets);
    buckets.append_attribute("size") = boundaries->unionBuckets()->size();
    foreach (auto * perFrequencyBuckets, *boundaries->unionBuckets()) {
        pugi::xml_node frequency = buckets.append_child("frequency");
        frequency.append_attribute("size") = perFrequencyBuckets->size();
        writeTraces(frequency, perFrequencyBuckets);
    }
}

void writeLoopShaping(pugi::xml_node root, DatosLoopShaping * loopShaping)
{
    pugi::xml_node section = root.append_child(t.loopShaping);

    pugi::xml_node data = section.append_child(t.boundariesData);
    data.append_attribute(t.loopShapingPointCountAttribute) = loopShaping->getNPuntos();
    addReal(data, t.axisMin, loopShaping->range().x());
    addReal(data, t.axisMax, loopShaping->range().y());

    writeSystem(section, t.controller, loopShaping->getControlador());
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
    if (content.templates != nullptr) {
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
