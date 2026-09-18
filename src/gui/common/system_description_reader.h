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
 * A read that refuses answers false or nothing and leaves its complaint in
 * complaint(): the form decides where to put it - beside the field that is
 * wrong, these days, rather than in a message box on top of everything.
 */
class SystemDescriptionReader
{
public:
    /// @param title the title of the messages, naming the form.
    explicit SystemDescriptionReader(QString title);

    /// Why the last read refused, empty when it did not.
    const QString & complaint() const;

    /**
     * @brief One polynomial's coefficients, space separated. A token with a
     * name in it is an uncertain parameter, recorded by that name.
     *
     * @param emptyIsOne what an empty field means. For a polynomial it is
     * the constant 1, which is the polynomial that changes nothing; for a
     * family written as FACTORS it is no factors at all, and reading it as
     * a 1 would put a zero at s = -1 in a plant that has none.
     */
    bool readCoefficients(const QString & text, CoefficientTable & table,
                          CoefficientTable & expressionTable, UncertainTable & uncertainTable,
                          bool emptyIsOne = true);

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
    /// is not a valid finite expression, rather than becoming 0 in silence.
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
    /**
     * @brief Every name in 'text' that is a parameter, each once and in
     * order of appearance.
     *
     * A parameter is a whole identifier of the grammar - a letter and then
     * letters, digits or underscores - that is neither a function of the
     * parser nor the Laplace variable, and that is not the e of a number in
     * scientific notation. A reserved name (pi, e) is refused: 'refused' is
     * set and complaint() says which.
     */
    QStringList parameterNames(const QString & text, bool & refused);

    /// The first of those, or an empty string.
    QString firstParameterName(const QString & text, bool & refused);

    QString m_title;
    QString m_complaint;
};

} // namespace qftbx

#endif // QFTBX_GUI_SYSTEM_DESCRIPTION_READER_H
