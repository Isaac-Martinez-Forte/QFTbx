/**
 * @file
 * @brief The step that asks for the parameter grids and the contour
 * epsilon.
 *
 * Declares the panel with a general point count and spacing, a row per
 * uncertain parameter with its own linear, logarithmic or manual grid, the
 * plane the templates are drawn on and the epsilon is measured in, how the
 * contour is extracted (the walk or the alpha shape) and whether the whole
 * template stands in when it does not close or the computation stops, the
 * border-only sweep, offered only for exactly two uncertain parameters, and
 * the epsilon, one per design frequency. The point-count ceiling from the
 * settings only ever refuses input, so it changes no computed result.
 *
 * An installed proposer fills the epsilon field with the least value that
 * closes each contour, for the family swept over the grids the panel holds
 * and in its plane, on launch and from the Propose button; without one the
 * field opens empty, and proposals() is empty when none was made. The
 * grids are returned by value and the panel keeps its own copy for a
 * second accept; takeEpsilon() moves the epsilon out, so the panel holds
 * nothing between accepts. The plant is an observer the project owns and
 * can take away while the panel is open: after forgetPlant() the panel
 * refuses to publish until it is given one again, and shownPlant() tells
 * the window which plant the grids on screen were built for. The line
 * edits and radio buttons of the rows are owned by Qt through their row
 * widget.
 */

#ifndef QFTBX_TEMPLATES_FORM_H
#define QFTBX_TEMPLATES_FORM_H

#include "src/core/project/settings.h"
#include "src/gui/application/step_panel.h"
#include <functional>
#include <memory>

#include <vector>

#include <QWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QWidget>
#include <QString>
#include <QMessageBox>
#include <QRadioButton>
#include <QHash>
#include <QStringList>

#include "src/gui/common/parlineedit.h"
#include "src/core/system/lti_system.h"
#include "src/core/templates/hull_metric.h"
#include "src/core/templates/parameter_grids.h"
#include "src/core/templates/template_engine.h"
#include "src/core/system/parameter.h"
#include "src/core/math/sequence_vectors.h"

namespace Ui {
class TemplatesForm;
}

namespace qftbx {

class TemplatesForm : public StepPanel
{
    Q_OBJECT

public:
    void setMaxPointCount(double points) { m_maxPointCount = points; }

    void setDefaultPointCount(std::int32_t points);

    explicit TemplatesForm(QWidget *parent = 0);

    ~TemplatesForm();

    void launch(LtiSystem * plant, qint32 frequencyCount);

    LtiSystem * shownPlant() const { return plant; }

    void forgetPlant();

    qftbx::ParameterGrids grids() const;

    std::vector<double> takeEpsilon();

    bool nicholsSelected();

    bool cudaSelected();

    qftbx::EpsilonMetric epsilonMetric() const;
    void setEpsilonMetric(qftbx::EpsilonMetric metric);

    bool wholeTemplateIfNoContour() const;
    void setWholeTemplateIfNoContour(bool standsIn);

    bool alphaShapeContour() const;
    void setAlphaShapeContour(bool alphaShape);

    bool borderSweep() const;
    void setBorderSweep(bool border);

    using EpsilonProposer = std::function<std::vector<qftbx::TemplateEngine::EpsilonProposal>(
        const qftbx::ParameterGrids &, qftbx::EpsilonMetric)>;
    void setEpsilonProposer(EpsilonProposer propose);

    const std::vector<qftbx::TemplateEngine::EpsilonProposal> & proposals() const { return m_proposals; }

    struct ThreeRadioButtons{
        QRadioButton * linear = nullptr;
        QRadioButton * logarithmic = nullptr;
        QRadioButton * manual = nullptr;
    };

private slots:
    void on_allVariablesRadio_clicked();

    void on_oneByOneRadio_clicked();
    void on_metricCombo_currentIndexChanged(int index);

    void on_numeratorRadio_clicked();

    void on_denominatorRadio_clicked();

    void on_okButton_clicked();

    void on_proposeButton_clicked();

private:
    void clearTables();

    bool readGrids(QString & reason);

    void selectDefaultsWhereEmpty();

    void proposeEpsilon();

    EpsilonProposer m_propose;
    std::vector<qftbx::TemplateEngine::EpsilonProposal> m_proposals;

    std::unique_ptr<Ui::TemplatesForm> ui;

    void buildRow (QWidget *widget, QVector<ParLineEdit> & par,
                   QVector <ThreeRadioButtons> & rowRadios);
    void buildTables(std::vector<Parameter> & numerator, std::vector<Parameter> & denominator);
    QString m_readReason;

    bool readVariable(const ParLineEdit & rowEdits, ThreeRadioButtons rowRadios, Parameter & parameter,
                         bool useLinspace, bool useLogspace);

    QVector <ParLineEdit> numeratorRows;
    QVector <ParLineEdit> denominatorRows;
    qftbx::ParameterGrids gridMap;
    QVector <ThreeRadioButtons> numeratorRadios;
    QVector <ThreeRadioButtons> denominatorRadios;
    std::vector<Parameter> numerator;
    std::vector<Parameter> denominator;
    LtiSystem * plant = nullptr;

    bool rowsBuilt = false;
    bool cudaEnabled = false;

    bool nicholsDiagram  = true;

    std::vector<double> epsilonValues;

    QStringList duplicateNames;

    qint32 frequencyCount = 0;

    double m_maxPointCount = qftbx::Settings().limits.maxTemplatePoints;

};

}

#endif
