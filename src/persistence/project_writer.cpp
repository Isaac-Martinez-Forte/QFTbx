#include <vector>
#include <cstdint>
#include "src/persistence/project_writer.h"

#include <cmath>
#include <cstdio>
#include <string>

#include "src/core/math/point.h"

#include <pugixml.hpp>

#include "src/core/common/exception.h"
#include "src/persistence/qft_dialect.h"
#include "src/core/system/parameter.h"
#include "src/core/common/text_tokens.h"

namespace qftbx {

namespace {

//The one place a real becomes text: see qftbx::text::number. Not "%.17g",
//which writes noise digits (0.00010155800000000001 for a value whose exact
//short form is 0.000101558).
using qftbx::text::number;

const Tags & t = kV4;

//A NaN or an infinity never leaves for the file: written as text they read
//back through strtod, and a project that had gone wrong in memory would
//come back looking like one that had not.
void requireFinite(double value, const char * what)
{
    if (!std::isfinite(value)) {
        throw InvalidInput(QFTBX_TR("Core", "The project cannot be written: <%1> holds a value that is not a finite number.").arg(what));
    }
}

std::string realVectorText(const std::vector <double> & values, const char * what)
{
    std::string text;
    for (double value : values) {
        requireFinite(value, what);
        text += number(value) + " ";
    }
    return text;
}

std::string pointVectorText(const std::vector<qftbx::NicholsPoint> & points, const char * what)
{
    std::string text;
    for (const qftbx::NicholsPoint & point : points) {
        requireFinite(point.phase, what);
        requireFinite(point.magnitude, what);
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

//An interval end: a real, or an infinity, which is legitimate here (the
//side of an open boundary) and which strtod reads back.
std::string endText(double value)
{
    if (std::isinf(value)) {
        return value > 0 ? "inf" : "-inf";
    }
    return number(value);
}

std::string columnsText(const BoundaryColumns & columns)
{
    std::string text;
    for (std::int32_t c = 0; c < columns.columnCount(); ++c) {
        const std::vector<BoundaryColumns::Span> spans = columns.spans(c);
        text += std::to_string(spans.size()) + " ";
        for (const BoundaryColumns::Span & span : spans) {
            text += endText(span.lo) + " " + endText(span.hi) + " ";
        }
    }
    return text;
}

void addText(pugi::xml_node parent, const char * name, const std::string & text)
{
    parent.append_child(name).text().set(text.c_str());
}

void addReal(pugi::xml_node parent, const char * name, double value)
{
    requireFinite(value, name);
    addText(parent, name, number(value));
}

void addBool(pugi::xml_node parent, const char * name, bool value)
{
    parent.append_child(name).text().set(value ? "true" : "false");
}

//The RAW values, because they are the state and the reader takes what it
//finds as the state. range() and nominal() are those values with the
//reparametrisation already applied, so writing them applies the expression
//once more on every save and load: "a*10" over [1, 2] comes back raw as
//[10, 20] and reports [100, 200].
void writeParameter(pugi::xml_node parent, const Parameter & parameter)
{
    pugi::xml_node node = parent.append_child("parameter");
    addReal(node, t.nominal, parameter.rawNominal());
    addBool(node, t.uncertain, parameter.isUncertain());

    if (parameter.isUncertain()) {
        addText(node, t.parameterName, parameter.name());
        addText(node, t.parameterExpression, parameter.expression());
        pugi::xml_node range = node.append_child(t.range);
        addReal(range, t.rangeMin, parameter.rawRange().min);
        addReal(range, t.rangeMax, parameter.rawRange().max);
    }
}

void writeSystem(pugi::xml_node parent, const char * sectionName, LtiSystem * system)
{
    pugi::xml_node node = parent.append_child(sectionName);
    node.append_attribute(t.nameAttribute) = system->name().c_str();

    pugi::xml_node typeNode = node.append_child(t.type);
    typeNode.append_attribute(t.typeAttribute) = static_cast<std::int32_t>(system->type());

    pugi::xml_node expression = typeNode.append_child(t.expression);
    if (system->type() == LtiSystem::SystemType::FreeForm) {
        expression.append_attribute("size") = 2;
        addText(expression, t.numerator, system->numeratorString());
        addText(expression, t.denominator, system->denominatorString());
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
        node.append_attribute(t.nameAttribute) = record.name.c_str();
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

void writeOmega(pugi::xml_node root, const Omega * omega)
{
    pugi::xml_node section = root.append_child(t.omega);
    addReal(section, t.omegaMin, omega->start());
    addReal(section, t.omegaMax, omega->end());
    addText(section, t.pointCount, std::to_string(omega->pointCount()));
    addText(section, t.omegaType, std::to_string(static_cast<std::int32_t>(omega->type())));
    addText(section, t.values, realVectorText(*omega->values(), t.values));
}

void writeComplexVectors(pugi::xml_node section, const qftbx::CloudSet & vectors, const char * what)
{
    for (const qftbx::ComplexCloud & vector : vectors) {
        std::string reals;
        std::string imaginaries;
        for (const std::complex<double> & value : vector) {
            requireFinite(value.real(), what);
            requireFinite(value.imag(), what);
            reals += number(value.real()) + " ";
            imaginaries += number(value.imag()) + " ";
        }
        addText(section, "re", reals);
        addText(section, "im", imaginaries);
    }
}

/// What the templates were computed with: the tolerance of the hull walk,
/// per frequency, and the plane it is measured in.
void writeTemplateSettings(pugi::xml_node settings, const ProjectContent & content)
{
    pugi::xml_node section = settings.append_child(t.templates);
    pugi::xml_node epsilonNode = section.append_child(t.epsilon);
    epsilonNode.text().set((content.epsilon != nullptr ? realVectorText(*content.epsilon, t.epsilon) : std::string()).c_str());
    epsilonNode.append_attribute("metric") = hullMetricName(content.epsilonMetric.metric);
    epsilonNode.append_attribute("db-per-degree") = number(content.epsilonMetric.dbPerDegree).c_str();
}

/// What the search was asked for. The same problem answers a different gain
/// under a different reading of the phase grid, so a design without these
/// three numbers cannot be compared with another.
void writeLoopShapingSettings(pugi::xml_node settings, const LoopShapingResult::Run & run)
{
    pugi::xml_node section = settings.append_child(t.loopShaping);
    section.append_attribute("algorithm") = algorithmName(run.algorithm);
    section.append_attribute("tolerance") = number(run.epsilon).c_str();
    section.append_attribute("columns") = run.conservativeColumns ? "conservative" : "nearest";
}

void writeTemplates(pugi::xml_node root, const ProjectContent & content)
{
    pugi::xml_node section = root.append_child(t.templates);

    pugi::xml_node full = section.append_child(t.fullTemplates);
    full.append_attribute("size") = static_cast<std::int64_t>(content.templates->size());
    writeComplexVectors(full, *content.templates, t.fullTemplates);

    if (content.contour != nullptr && !content.contour->empty()) {
        pugi::xml_node contour = section.append_child(t.templateContour);
        contour.append_attribute("size") = static_cast<std::int64_t>(content.contour->size());
        writeComplexVectors(contour, *content.contour, t.templateContour);
    }
}

void writeTraces(pugi::xml_node parent, const qftbx::TraceSet & traces, const char * what)
{
    for (const qftbx::Trace & trace : traces) {
        addText(parent, "trace", pointVectorText(trace, what));
    }
}

void writeBoundaries(pugi::xml_node root, const BoundaryData * boundaries)
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

    //The allowed magnitude intervals per phase column of every
    //specification, per frequency: for each column its interval count and
    //then the ends of each interval, which may be infinite. A reader
    //without them rebuilds them from the traces, a cell coarser.
    pugi::xml_node columns = metadata.append_child(t.boundaryColumns);
    for (const auto & map : boundaries->specificationColumns()) {
        pugi::xml_node frequency = columns.append_child("frequency");
        for (const auto & entry : map) {
            addText(frequency, entry.first.c_str(), columnsText(entry.second));
        }
    }

    pugi::xml_node perFrequency = data.append_child(t.perFrequency);
    perFrequency.append_attribute("size") = static_cast<std::int64_t>(boundaries->boundaries().size());
    for (const auto & map : boundaries->boundaries()) {
        pugi::xml_node frequency = perFrequency.append_child("frequency");
        frequency.append_attribute("size") = static_cast<std::int64_t>(map.size());

        //std::map iterates in key order, as QMap::keys() did.
        for (const auto & entry : map) {
            pugi::xml_node keyNode = frequency.append_child(entry.first.c_str());
            keyNode.append_attribute("size") = static_cast<std::int64_t>(entry.second.size());
            writeTraces(keyNode, entry.second, t.perFrequency);
        }
    }

    pugi::xml_node unionNode = data.append_child(t.boundaryUnion);
    unionNode.append_attribute("size") = static_cast<std::int64_t>(boundaries->unionBoundaries().size());
    writeTraces(unionNode, boundaries->unionBoundaries(), t.boundaryUnion);

    pugi::xml_node buckets = data.append_child(t.unionBuckets);
    buckets.append_attribute("size") = static_cast<std::int64_t>(boundaries->unionBuckets().size());
    for (const qftbx::TraceSet & perFrequencyBuckets : boundaries->unionBuckets()) {
        pugi::xml_node frequency = buckets.append_child("frequency");
        frequency.append_attribute("size") = static_cast<std::int64_t>(perFrequencyBuckets.size());
        writeTraces(frequency, perFrequencyBuckets, t.unionBuckets);
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

    //The verifier's verdict: the worst excess over any active specification,
    //in decibels, over the full templates. Negative means satisfied, and by
    //how much. The itemised list behind it is not written - it is derived
    //from data the file already has - but the verdict is not derivable
    //without recomputing it, and it is what the design is worth.
    if (loopShaping->check().has_value()) {
        pugi::xml_node check = section.append_child(t.check);
        check.append_attribute("satisfied") = loopShaping->check()->satisfied();
        //Minus infinity when no specification was active at any frequency:
        //there is nothing to have exceeded, and the file carries no
        //infinities. The verdict stands on its own.
        if (std::isfinite(loopShaping->check()->worstExcessDb)) {
            check.append_attribute("worst-excess-db") = number(loopShaping->check()->worstExcessDb).c_str();
        }
    }
}

} // namespace

void ProjectWriter::save(const std::string & filePath, const ProjectContent & content)
{
    pugi::xml_document document;
    pugi::xml_node declaration = document.append_child(pugi::node_declaration);
    declaration.append_attribute("version") = "1.0";
    declaration.append_attribute("encoding") = "UTF-8";

    pugi::xml_node root = document.append_child("QFT");
    root.append_attribute("version") = kVersion;

    //Three parts, in the order a reader meets them: what the user described,
    //then what each computation was run with, then what came out. The
    //problem is legible in the first page of the file and the bulk is at the
    //bottom, which is the point of the arrangement.
    pugi::xml_node inputs = root.append_child(t.inputs);

    if (content.plant != nullptr) {
        writeSystem(inputs, t.plant, content.plant);
    }
    if (content.specifications != nullptr) {
        writeSpecifications(inputs, content.specifications);
    }
    if (content.omega != nullptr) {
        writeOmega(inputs, content.omega);
    }
    //The controller STRUCTURE is an input: it is the box the search is asked
    //to look in, not what the search found.
    if (content.controller != nullptr) {
        writeSystem(inputs, t.controller, content.controller);
    }

    const bool hasTemplates = content.templates != nullptr && !content.templates->empty();

    if (hasTemplates || content.loopShaping != nullptr) {
        pugi::xml_node settings = root.append_child(t.settings);
        if (hasTemplates) {
            writeTemplateSettings(settings, content);
        }
        if (content.loopShaping != nullptr) {
            writeLoopShapingSettings(settings, content.loopShaping->run());
        }
    }

    pugi::xml_node results = root.append_child(t.results);

    if (hasTemplates) {
        writeTemplates(results, content);
    }
    if (content.boundaries != nullptr) {
        writeBoundaries(results, content.boundaries);
    }
    if (content.loopShaping != nullptr) {
        writeLoopShaping(results, content.loopShaping);
    }

    if (!document.save_file(filePath.c_str(), "    ",
                            pugi::format_default, pugi::encoding_utf8)) {
        throw FileError(QFTBX_TR("Core", "Cannot write project file: %1").arg(filePath));
    }
}

} // namespace qftbx
