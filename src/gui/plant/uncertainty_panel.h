/**
 * @file
 * @brief The parametric uncertainty of a plant or controller, one row per name.
 *
 * Declares a panel, not a dialog: a page of the form that owns it, since
 * the uncertainty is part of describing the system. It works on the tables
 * the form read out of its fields and hands them back as parameters, one
 * row per name rather than per coefficient, so a name that appears twice
 * is one parameter with one range. In range-only mode, for a controller
 * structure, the nominal is hidden and the midpoint stands in for it.
 */

#ifndef QFTBX_UNCERTAINTY_PANEL_H
#define QFTBX_UNCERTAINTY_PANEL_H

#include <memory>
#include <vector>

#include <QLineEdit>
#include <QString>
#include <QWidget>

#include "src/core/math/range.h"
#include "src/core/system/parameter.h"
#include "src/gui/common/coefficient_tables.h"

namespace Ui {
class UncertaintyPanel;
}

namespace qftbx {

/**
 * @brief The parametric uncertainty of a plant or a controller: the range
 * and the nominal value of every coefficient the user named, plus the gain
 * and the delay.
 *
 * A PANEL and not a dialog: it is a page of the form that owns it, reached
 * by a button and left by another, because the uncertainty of a plant is
 * part of describing the plant and not a question asked on top of it.
 *
 * It works on the tables the form read out of its fields and hands them
 * back as parameters; it never touches the project.
 *
 * One row per NAME, not per coefficient: a name that appears twice is one
 * parameter with one range, which is what makes it uncertainty and not two
 * unrelated numbers.
 *
 * @author Isaac Martínez Forte
 */
class UncertaintyPanel : public QWidget
{
    Q_OBJECT

public:
    explicit UncertaintyPanel(QWidget * parent = nullptr);
    ~UncertaintyPanel();

    /// The numerator coefficients as the user left them, one Parameter per
    /// coefficient, uncertain ones carrying their range.
    std::vector<Parameter> & numerator();

    /// The denominator coefficients, likewise.
    std::vector<Parameter> & denominator();

    /// Range of the gain k.
    Range gain();

    /// Range of the transport delay.
    Range delay();

    /// True when the panel was applied with valid ranges.
    bool wasAccepted() const;

    /**
     * @brief Builds the rows over the tables the caller read out of its
     * fields.
     *
     * @param valueTable the numeric value of every coefficient, by row.
     * @param expressionTable the reparametrising expression of each, if any.
     * @param uncertainTable which of them the user marked as uncertain.
     * @param rangeOnly whether only the ranges are asked for: the nominal
     * field is hidden and the midpoint stands in for it.
     */
    bool launch(CoefficientTable valueTable, CoefficientTable expressionTable,
                UncertainTable uncertainTable, bool rangeOnly);

    /**
     * @brief The parameters a system arrived with, so that a form opened
     * over a loaded project can answer for its uncertainty without the user
     * entering it again.
     *
     * The rows open on the interval each name already has, and the panel
     * counts as applied: its numerator() and denominator() are the ones
     * given here until the user edits them.
     */
    void setParameters(const std::vector<Parameter> & numerator,
                       const std::vector<Parameter> & denominator,
                       const Range & gain, const Range & delay);

    /// What the panel is the uncertainty OF, for its title.
    void setTitle(const QString & title);

signals:
    /// The ranges were read and are valid: the form may go back.
    void applied();

    /// The user left without applying.
    void cancelled();

private slots:
    void on_applyButton_clicked();
    void on_backButton_clicked();

private:
    /// One row of the table: the name it stands for and its three fields.
    struct Row
    {
        QString name;
        QLineEdit * minimum = nullptr;
        QLineEdit * nominal = nullptr;
        QLineEdit * maximum = nullptr;
    };

    /// The distinct uncertain names of the two polynomials, in order of
    /// appearance.
    std::vector<QString> uncertainNames() const;

    void buildRows();

    /// Reads every row into a parameter, marking the fields that are wrong
    /// and saying why. False when any row is unreadable, and then nothing
    /// is published.
    bool readRanges();

    /// The parameters of one polynomial, from the rows just read: one entry
    /// per coefficient, in its own order.
    std::vector<Parameter> parametersOf(std::size_t slot,
                                        const std::vector<Parameter> & named,
                                        bool & valid);

    /// The value of a field, or nothing when it is not an expression.
    static std::optional<double> valueOf(QLineEdit * field);

    void say(const QString & complaint);

    std::unique_ptr<Ui::UncertaintyPanel> ui;

    std::vector<Row> m_rows;

    /// The row widgets belong to the layout of the scroll area's content, and
    /// are destroyed with it when the rows are rebuilt.
    std::vector<QWidget *> m_rowWidgets;

    /// The intervals a loaded system brought, by name: what a row opens on
    /// when the text names a parameter the project already knows.
    std::vector<Parameter> m_known;

    std::vector<Parameter> m_numerator;
    std::vector<Parameter> m_denominator;

    CoefficientTable m_values;
    CoefficientTable m_expressions;
    UncertainTable m_uncertain;

    bool m_rangeOnly = false;
    bool m_accepted = false;
};

}

#endif
