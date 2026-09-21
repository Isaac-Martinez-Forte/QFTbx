/**
 * @file
 * @brief Name recognition and evaluation behind the system reader.
 *
 * A parameter is a whole identifier of the grammar, a letter followed by
 * letters, digits or underscores, found in one pass over the text and
 * recorded once in order of appearance. A function of the grammar and the
 * Laplace variable are not parameters; the constants `pi` and `e` cannot be
 * one and are refused with a complaint; the `e` of a number in scientific
 * notation is an exponent and is skipped. A coefficient that does not parse
 * or is not finite is refused rather than read as zero, because the system
 * designed would not be the one the user typed.
 */

#include "src/gui/common/system_description_reader.h"

#include <QObject>
#include <QRegularExpression>

#include "src/core/common/exception.h"
#include "src/core/math/expression_tree.h"
#include "src/gui/common/expression_field.h"
#include "src/core/system/free_form.h"
#include "src/core/system/polynomial_form.h"
#include "src/core/system/time_constant_gain.h"
#include "src/core/system/zero_pole_gain.h"
#include "src/core/common/text_tokens.h"

namespace qftbx {

SystemDescriptionReader::SystemDescriptionReader(QString title)
    : m_title(std::move(title))
{
}

const QString & SystemDescriptionReader::complaint() const
{
    return m_complaint;
}

QStringList SystemDescriptionReader::parameterNames(const QString & text, bool & refused)
{
    refused = false;
    m_complaint.clear();

    static const QRegularExpression identifier("[A-Za-z][A-Za-z0-9_]*");

    QStringList names;

    QRegularExpressionMatchIterator found = identifier.globalMatch(text);
    while (found.hasNext()) {
        const QRegularExpressionMatch match = found.next();
        const QString capture = match.captured(0);

        const int before = match.capturedStart(0) - 1;
        if (before >= 0) {
            const QChar previous = text.at(before);
            if (previous.isDigit() || previous == QLatin1Char('.')) {
                continue;
            }
        }

        const std::string name = capture.toStdString();

        if (name == "pi" || name == "PI" || name == "e" || name == "E") {
            m_complaint = QObject::tr("\"%1\" cannot be used as a parameter name: "
                                      "it is a constant of the expression grammar.").arg(capture);
            refused = true;
            return QStringList();
        }

        if (ExpressionTree::isFunctionName(name) || name == FreeForm::laplaceName()) {
            continue;
        }

        if (!names.contains(capture)) {
            names.push_back(capture);
        }
    }

    return names;
}

QString SystemDescriptionReader::firstParameterName(const QString & text, bool & refused)
{
    const QStringList names = parameterNames(text, refused);

    return names.isEmpty() ? QString() : names.front();
}

bool SystemDescriptionReader::readCoefficients(const QString & text, CoefficientTable & table,
                                               CoefficientTable & expressionTable,
                                               UncertainTable & uncertainTable,
                                               bool emptyIsOne)
{
    CoefficientRow expressions;
    for (const std::string & token : text::tokens(text.toStdString())) {
        expressions.push_back(QString::fromStdString(token));
    }

    CoefficientRow values;
    UncertainRow uncertainFlags;

    if (text.trimmed().isEmpty() && emptyIsOne) {
        expressions.push_back("1");
        values.push_back("1");
        uncertainFlags.push_back(false);
    } else if (!text.trimmed().isEmpty()) {
        for (const QString & expression : expressions) {
            bool refused = false;
            const QString parameter = firstParameterName(expression, refused);
            if (refused) {
                return false;
            }

            const bool uncertain = !parameter.isEmpty();
            uncertainFlags.push_back(uncertain);
            values.push_back(uncertain ? parameter : expression);
        }
    }

    table.push_back(values);
    uncertainTable.push_back(uncertainFlags);
    expressionTable.push_back(expressions);

    return true;
}

bool SystemDescriptionReader::readScalar(const QString & text, CoefficientTable & table,
                                         CoefficientTable & expressionTable,
                                         UncertainTable & uncertainTable)
{
    const QString trimmed = text.trimmed();

    bool refused = false;
    const QString parameter = firstParameterName(trimmed, refused);
    if (refused) {
        return false;
    }

    const bool uncertain = !parameter.isEmpty();

    table.push_back(CoefficientRow(1, uncertain ? parameter : trimmed));
    expressionTable.push_back(CoefficientRow(1, trimmed));
    uncertainTable.push_back(UncertainRow(1, uncertain));

    return true;
}

bool SystemDescriptionReader::readGainRange(const QString & start, const QString & end,
                                            CoefficientTable & table,
                                            CoefficientTable & expressionTable,
                                            UncertainTable & uncertainTable)
{
    const CoefficientRow ends{start.trimmed(), end.trimmed()};

    table.push_back(ends);
    expressionTable.push_back(ends);
    uncertainTable.push_back(UncertainRow(1, true));

    return true;
}

bool SystemDescriptionReader::readFreeForm(const QString & text, CoefficientTable & table,
                                           CoefficientTable & expressionTable,
                                           UncertainTable & uncertainTable)
{
    bool refused = false;
    const QStringList found = parameterNames(text, refused);

    if (refused) {
        return false;
    }

    CoefficientRow names;
    UncertainRow flags;

    for (const QString & name : found) {
        names.push_back(name);
        flags.push_back(true);
    }

    table.push_back(names);
    expressionTable.push_back(names);
    uncertainTable.push_back(flags);

    return true;
}

std::optional<double> SystemDescriptionReader::evaluate(const QString & expression)
{
    return evaluateNumber(expression);
}

std::optional<std::vector<Parameter>> SystemDescriptionReader::buildParameters(const CoefficientRow & numbers)
{
    std::vector<Parameter> parameters;
    parameters.reserve(numbers.size());

    for (const QString & number : numbers) {
        const std::optional<double> value = evaluate(number);
        if (!value.has_value()) {
            return std::nullopt;
        }
        try {
            parameters.push_back(Parameter(*value));
        } catch (const qftbx::Exception &) {
            return std::nullopt;
        }
    }

    return parameters;
}

std::unique_ptr<LtiSystem> SystemDescriptionReader::makeSystem(LtiSystem::SystemType type, const std::string & name,
                                                               std::vector<Parameter> numerator,
                                                               std::vector<Parameter> denominator,
                                                               Parameter gain, Parameter delay,
                                                               const std::string & numeratorExpression,
                                                               const std::string & denominatorExpression)
{
    switch (type) {
    case LtiSystem::SystemType::ZeroPoleGain:
        return std::make_unique<ZeroPoleGain>(name, std::move(numerator), std::move(denominator),
                                              std::move(gain), std::move(delay));
    case LtiSystem::SystemType::TimeConstantGain:
        return std::make_unique<TimeConstantGain>(name, std::move(numerator), std::move(denominator),
                                                  std::move(gain), std::move(delay));
    case LtiSystem::SystemType::PolynomialForm:
        return std::make_unique<PolynomialForm>(name, std::move(numerator), std::move(denominator),
                                                std::move(gain), std::move(delay));
    case LtiSystem::SystemType::FreeForm:
        break;
    }

    return std::make_unique<FreeForm>(name, std::move(numerator), std::move(denominator),
                                      std::move(gain), std::move(delay),
                                      numeratorExpression, denominatorExpression);
}

}
