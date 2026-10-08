/**
 * @file
 * @brief Plots the templates of a plant and the epsilon-hull contour of each.
 *
 * Declares the viewer of the value set at every design frequency and its
 * contour, on the Nichols plane or, when plotDiagram is given true, on the
 * Nyquist one, with a legend row per frequency that carries the epsilon it
 * was walked with, the epsilon it asks for and whether the whole template
 * stands in for a contour that did not close. The viewer does not reach
 * into the project: it is handed what it draws, one epsilon per design
 * frequency, and holds its own copies, since a project may carry templates
 * and no epsilon. It lives as long as the window, so when the project
 * drops a step it is cleared, not destroyed.
 *
 * Recomputing, proposing and reporting are plain callbacks installed by
 * whoever owns the computation: one caller, one handler, one thread.
 * Without a recomputer the button does nothing; the recomputer takes the
 * epsilons and answers with refreshContour, which redraws the contour
 * without rebuilding the frequency colours. The plot owns the graphs and
 * curves. A cloud is a scatter, having no order, and a contour a QCPCurve,
 * never a QCPGraph, which would sort it by phase although a contour is
 * multivalued in phase; a cloud of several components has one curve per
 * component, so legend rows and curves are not one to one. The card opens
 * with the contour shown and the cloud hidden.
 */

#ifndef QFTBX_TEMPLATE_VIEWER_H
#define QFTBX_TEMPLATE_VIEWER_H

#include <vector>
#include <memory>

#include <QWidget>
#include <QLabel>
#include "src/core/templates/template_engine.h"
#include <complex>
#include <functional>
#include <QMap>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QCheckBox>

#include "src/core/math/sequence_vectors.h"
#include "src/core/frequencies/omega.h"
#include "qcustomplot.h"
#include "src/gui/common/frequency_legend.h"
#include "src/core/templates/cloud_set.h"

namespace Ui {
class TemplateViewer;
}

namespace qftbx {

class TemplateViewer : public QWidget
{
    Q_OBJECT

public:

    explicit TemplateViewer(QWidget *parent = 0);
    ~TemplateViewer();

    void plotDiagram(bool plot);

    void clear();

    void setData(const qftbx::CloudSet & templates,
                  const qftbx::CloudSet & contour,
                  std::vector<double> * omega,
                  std::vector<double> * epsilon);

    using ContourRecomputer = std::function<void (std::vector<double> epsilon)>;

    void setContourRecomputer(ContourRecomputer recompute);

    using EpsilonProposer = std::function<std::vector<qftbx::TemplateEngine::EpsilonProposal> ()>;
    void setEpsilonProposer(EpsilonProposer propose);

    using ContourReporter = std::function<std::vector<qftbx::TemplateEngine::ContourReport> ()>;
    void setContourReporter(ContourReporter report);

    void refreshContour(const qftbx::CloudSet & contour,
                        std::vector<double> * omega,
                        std::vector<double> * epsilon);

    void setTemplates (const qftbx::CloudSet & templates);

    void setContour (const qftbx::CloudSet & contour);

private slots:
    void on_saveImage_clicked();

    void on_templatesButton_clicked();

    void on_contourButton_clicked();

    void applyCheckboxes ();

    void on_recomputeButton_clicked();
    void on_proposeButton_clicked();

private:
    std::unique_ptr<Ui::TemplateViewer> ui;
    EpsilonProposer propose;
    ContourReporter report;
    std::vector<QLabel *> gapLabels;
    std::vector<QLabel *> stateLabels;
    void showContourState();
    std::vector<qftbx::TemplateEngine::EpsilonProposal> m_proposals;
    void showProposals();
    bool plotted = false;
    void plotCloud(const std::vector<double> & phases, const std::vector<double> & magnitudes,
                   qint32 frequency);

    void plotContour(const std::vector<double> & phases, const std::vector<double> & magnitudes,
                     qint32 frequency);

    void addFrequencyRow (QColor color, qint32 pos);
    FrequencyLegend * legend = nullptr;
    void clearDiagram();

    qftbx::CloudSet m_templates;
    qftbx::CloudSet m_contour;
    std::vector<double> m_omega;
    std::vector<double> m_epsilon;

    QVector <QCPGraph *> templateGraphs;
    QVector <QVector <QCPCurve *>> contourCurves;
    QMap <qreal, QColor> colorByFrequency;

    ContourRecomputer recompute;

    QVector <QLineEdit *> epsilonEdits;

    bool templatesVisible = false;
    bool contourVisible = true;

    bool plot = false;
};

}

#endif
