/**
 * @file
 * @brief Reads the text a user types for a system into coefficient tables
 * and builds the system.
 *
 * Shared by the plant, controller and specification forms: tokenising a
 * coefficient line, spotting the names that make a coefficient uncertain,
 * refusing the names the expression grammar owns, evaluating the rest, and
 * choosing among the four system families. A read that refuses answers
 * false or nothing, never a silent zero, and leaves its complaint, titled
 * with the form's name, for the form to put beside the field that is
 * wrong. An empty polynomial reads as the constant 1, except for a family
 * written as factors, where it is no factors at all: a 1 there would put a
 * zero at s = -1.
 *
 * A parameter is a whole identifier that is neither a function nor the
 * Laplace variable s nor the e of a number in scientific notation, and the
 * constants pi and e are refused as names. Coefficients are space
 * separated; a gain or a delay is one expression; in a free-form
 * expression in s every other name is a parameter; and the ends of a gain
 * search box are always uncertain.
 */

#ifndef QFTBX_GUI_SYSTEM_DESCRIPTION_READER_H
#define QFTBX_GUI_SYSTEM_DESCRIPTION_READER_H

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <QString>
#include <QStringList>

#include "src/core/system/lti_system.h"
#include "src/core/system/parameter.h"
#include "src/gui/common/coefficient_tables.h"

namespace qftbx {

class SystemDescriptionReader
{
public:
    explicit SystemDescriptionReader(QString title);

    const QString & complaint() const;

    bool readCoefficients(const QString & text, CoefficientTable & table,
                          CoefficientTable & expressionTable, UncertainTable & uncertainTable,
                          bool emptyIsOne = true);

    bool readScalar(const QString & text, CoefficientTable & table,
                    CoefficientTable & expressionTable, UncertainTable & uncertainTable);

    bool readGainRange(const QString & start, const QString & end, CoefficientTable & table,
                       CoefficientTable & expressionTable, UncertainTable & uncertainTable);

    bool readFreeForm(const QString & text, CoefficientTable & table,
                      CoefficientTable & expressionTable, UncertainTable & uncertainTable);

    std::optional<std::vector<Parameter>> buildParameters(const CoefficientRow & numbers);

    std::optional<double> evaluate(const QString & expression);

    static std::unique_ptr<LtiSystem> makeSystem(LtiSystem::SystemType type, const std::string & name,
                                                 std::vector<Parameter> numerator,
                                                 std::vector<Parameter> denominator,
                                                 Parameter gain, Parameter delay,
                                                 const std::string & numeratorExpression = std::string(),
                                                 const std::string & denominatorExpression = std::string());

private:
    QStringList parameterNames(const QString & text, bool & refused);

    QString firstParameterName(const QString & text, bool & refused);

    QString m_title;
    QString m_complaint;
};

}

#endif
