/**
 * @file
 * @brief The parametric uncertainty of a plant or controller, one row per
 * name.
 *
 * Declares a panel, not a dialog: a page of the form that owns it, reached
 * by a button and left by another, since the uncertainty is part of
 * describing the system. It edits the range and the nominal value of every
 * coefficient the user named, plus the gain and the delay. It works on the
 * tables the form read out of its fields and hands them back as
 * parameters, one per coefficient, and never touches the project. There is
 * one row per name rather than per coefficient, so a name that appears
 * twice is one parameter with one range. In range-only mode, for a
 * controller structure, the nominal is hidden and the midpoint stands in
 * for it. Applying reads every row, marks the wrong fields and publishes
 * nothing unless all are valid; it then emits applied(). setParameters
 * opens the rows on the intervals a loaded system brought and counts as
 * applied, so a form over a loaded project answers for its uncertainty
 * until the user edits it. The row widgets belong to the layout of the
 * scroll area and die with it when the rows are rebuilt.
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

class UncertaintyPanel : public QWidget
{
    Q_OBJECT

public:
    explicit UncertaintyPanel(QWidget * parent = nullptr);
    ~UncertaintyPanel();

    std::vector<Parameter> & numerator();

    std::vector<Parameter> & denominator();

    Range gain();

    Range delay();

    bool wasAccepted() const;

    bool launch(CoefficientTable valueTable, CoefficientTable expressionTable,
                UncertainTable uncertainTable, bool rangeOnly);

    void setParameters(const std::vector<Parameter> & numerator,
                       const std::vector<Parameter> & denominator,
                       const Range & gain, const Range & delay);

    void setTitle(const QString & title);

signals:
    void applied();

    void cancelled();

private slots:
    void on_applyButton_clicked();
    void on_backButton_clicked();

private:
    struct Row
    {
        QString name;
        QLineEdit * minimum = nullptr;
        QLineEdit * nominal = nullptr;
        QLineEdit * maximum = nullptr;
    };

    std::vector<QString> uncertainNames() const;

    void buildRows();

    bool readRanges();

    std::vector<Parameter> parametersOf(std::size_t slot,
                                        const std::vector<Parameter> & named,
                                        bool & valid);

    static std::optional<double> valueOf(QLineEdit * field);

    void say(const QString & complaint);

    std::unique_ptr<Ui::UncertaintyPanel> ui;

    std::vector<Row> m_rows;

    std::vector<QWidget *> m_rowWidgets;

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
