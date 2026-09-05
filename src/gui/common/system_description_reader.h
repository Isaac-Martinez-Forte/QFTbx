#ifndef QFTBX_GUI_SYSTEM_DESCRIPTION_READER_H
#define QFTBX_GUI_SYSTEM_DESCRIPTION_READER_H

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <QString>


#include "src/core/system/lti_system.h"
#include "src/core/system/parameter.h"
#include "src/gui/common/coefficient_tables.h"

namespace qftbx {

/**
 * @brief Reads the text a user typed for a system - coefficients, gain,
 * delay, free-form expressions - into the tables the uncertainty dialog
 * edits, and builds the system from them.
 *
 * The plant dialog, the controller dialog and the specifications dialog
 * each carried their own copy of this: tokenising a coefficient line,
 * spotting the names that make a coefficient uncertain, refusing the names
 * the expression parser already owns, evaluating the rest, and a four-way
 * choice of system family. One copy remains, and the dialogs keep their
 * widgets.
 *
 * Every read reports its complaint through errorMessage() under the title
 * the dialog gave it, and answers false or nothing.
 */
class SystemDescriptionReader
{
public:
    /// @param title the title of the messages, naming the dialog.
    explicit SystemDescriptionReader(QString title);

    /**
     * @brief One polynomial's coefficients, space separated. A token with a
     * name in it is an uncertain parameter, recorded by that name; an empty
     * line is the constant 1.
     */
    bool readCoefficients(const QString & text, CoefficientTable & table,
                          CoefficientTable & expressionTable, UncertainTable & uncertainTable);

    /// A gain or a delay: one expression, uncertain when it names a parameter.
    bool readScalar(const QString & text, CoefficientTable & table,
                    CoefficientTable & expressionTable, UncertainTable & uncertainTable);

    /// The two ends of a gain search box, always uncertain.
    bool readGainRange(const QString & start, const QString & end, CoefficientTable & table,
                       CoefficientTable & expressionTable, UncertainTable & uncertainTable);

    /// A free-form expression in 's': every other name in it is a parameter.
    bool readFreeForm(const QString & text, CoefficientTable & table,
                      CoefficientTable & expressionTable, UncertainTable & uncertainTable);

    /// The coefficients of one row as constants, or nothing when one of them
    /// is not a valid finite expression (it used to become 0 in silence).
    std::optional<std::vector<Parameter>> buildParameters(const CoefficientRow & numbers);

    /// The value of one expression, or nothing when it does not parse.
    std::optional<double> evaluate(const QString & expression);

    /// The system of the given family over the given parameters; the two
    /// expressions are read by the free-form family only.
    static std::unique_ptr<LtiSystem> makeSystem(LtiSystem::SystemType type, const std::string & name,
                                                 std::vector<Parameter> numerator,
                                                 std::vector<Parameter> denominator,
                                                 Parameter gain, Parameter delay,
                                                 const std::string & numeratorExpression = std::string(),
                                                 const std::string & denominatorExpression = std::string());

private:
    /// The first name in 'text' that is a parameter rather than a function
    /// of the parser, or an empty string. Refuses a reserved name through
    /// errorMessage() and sets 'refused'.
    QString firstParameterName(const QString & text, bool & refused);

    QString m_title;
};

} // namespace qftbx

#endif // QFTBX_GUI_SYSTEM_DESCRIPTION_READER_H
