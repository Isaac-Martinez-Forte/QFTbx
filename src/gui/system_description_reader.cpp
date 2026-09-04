#include "src/gui/system_description_reader.h"

#include <QObject>
#include <QRegularExpression>

#include "src/core/exception.h"
#include "src/core/math/expression_cache.h"
#include "src/core/system/free_form.h"
#include "src/core/system/polynomial_form.h"
#include "src/core/system/time_constant_gain.h"
#include "src/core/system/zero_pole_gain.h"
#include "src/core/text_tokens.h"
#include "src/gui/error_message.h"

namespace qftbx {

SystemDescriptionReader::SystemDescriptionReader(QString title)
    : m_title(std::move(title)), m_parser(mup::pckALL_NON_COMPLEX)
{
}

QString SystemDescriptionReader::firstParameterName(const QString & text, bool & refused)
{
    refused = false;

    static const QRegularExpression name("[a-zA-Z]+");

    QString rest = text;
    QString capture = name.match(rest).captured(0);

    while (!capture.isNull()) {
        //A name the parser already owns cannot become a parameter: the
        //binding fails later and the system stops evaluating. The constants
        //(e, pi, i) and the unit operators - "k", "m", "u" - are reserved
        //too, where "k" is the obvious name for a gain and would otherwise
        //be read as the multiplier 1e3.
        if (math::isReservedName(capture.toStdString())) {
            errorMessage(QObject::tr("\"%1\" cannot be used as a parameter name: "
                                     "the expression parser already defines it.").arg(capture),
                         m_title);
            refused = true;
            return QString();
        }

        if (!m_parser.IsFunDefined(capture.toStdString()) && capture != "s") {
            return capture;
        }

        rest.remove(capture);
        capture = name.match(rest).captured(0);
    }

    return QString();
}

bool SystemDescriptionReader::readCoefficients(const QString & text, CoefficientTable & table,
                                               CoefficientTable & expressionTable,
                                               UncertainTable & uncertainTable)
{
    CoefficientRow expressions;
    for (const std::string & token : text::tokens(text.toStdString())) {
        expressions.push_back(QString::fromStdString(token));
    }

    CoefficientRow values;
    UncertainRow uncertainFlags;

    if (text.isEmpty()) {
        //An empty polynomial is the constant 1.
        expressions.push_back("1");
        values.push_back("1");
        uncertainFlags.push_back(false);
    } else {
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
    CoefficientRow names;
    UncertainRow flags;

    //Every name in the expression that is neither a parser function nor the
    //Laplace variable is a parameter, recorded once.
    QString rest = text;
    while (true) {
        bool refused = false;
        const QString parameter = firstParameterName(rest, refused);
        if (refused) {
            return false;
        }
        if (parameter.isEmpty()) {
            break;
        }
        if (!names.empty() && std::find(names.begin(), names.end(), parameter) != names.end()) {
            rest.remove(parameter);
            continue;
        }
        names.push_back(parameter);
        flags.push_back(true);
        rest.remove(parameter);
    }

    table.push_back(names);
    expressionTable.push_back(names);
    uncertainTable.push_back(flags);

    return true;
}

std::optional<double> SystemDescriptionReader::evaluate(const QString & expression)
{
    try {
        m_parser.SetExpr(expression.toStdString());
        return m_parser.Eval().GetFloat();
    } catch (mup::ParserError &) {
        return std::nullopt;
    }
}

std::optional<std::vector<Parameter>> SystemDescriptionReader::buildParameters(const CoefficientRow & numbers)
{
    std::vector<Parameter> parameters;
    parameters.reserve(numbers.size());

    for (const QString & number : numbers) {
        const std::optional<double> value = evaluate(number);
        if (!value.has_value()) {
            //An invalid coefficient used to become 0 here, silently: the
            //system that got designed was not the one the user typed.
            return std::nullopt;
        }
        try {
            parameters.push_back(Parameter(*value));
        } catch (const qftbx::Exception &) {
            //A coefficient that is not a finite number: same answer.
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

} // namespace qftbx
