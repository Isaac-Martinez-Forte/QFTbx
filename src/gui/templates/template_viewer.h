

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


/**
 * @brief Plots the templates of a plant - its value set at every design
 * frequency - and the epsilon-hull contour computed from them.
 *
 * @author Isaac Martínez Forte
 */
class TemplateViewer : public QWidget
{
    Q_OBJECT

public:

    explicit TemplateViewer(QWidget *parent = 0);
    ~TemplateViewer();


   /**
    * @brief Builds the plot.
    *
    * @param plot which plane to draw on: false is Nichols, true is
    * Nyquist.
    */
    void plotDiagram(bool plot);

    /**
     * @brief Forgets what it was drawing and empties the plot.
     *
     * A viewer lives in its phase's dock for as long as the window does,
     * and what it holds are observers on the project: when the project
     * drops a step, the pointers behind them go with it. This is what is
     * called then, instead of destroying the viewer.
     */
    void clear();


   /**
    * @brief Publishes everything the plot needs at once, instead of
    * calling the two setters separately.
    *
    * The viewer does not reach into the project: it is handed what it
    * draws.
    *
    * @param templates the plant value set at every design frequency.
    * @param contour the epsilon-hull of each of those.
    * @param omega the design frequencies the templates belong to.
    * @param epsilon the tightening of each frequency, one per omega entry.
    */

    void setData(const qftbx::CloudSet & templates,
                  const qftbx::CloudSet & contour,
                  std::vector<double> * omega,
                  std::vector<double> * epsilon);

   /**
    * @brief What runs when the user asks for a tighter contour. Ownership of
    * the epsilon vector passes to the handler, which answers with
    * refreshContour().
    *
    * A plain callback rather than a Qt signal: one caller, one handler, same
    * thread. Same seam as qftbx::ErrorReporter.
   */

    using ContourRecomputer = std::function<void (std::vector<double> epsilon)>;

   /**
    * @fn setContourRecomputer
    * @brief Installs the handler of the recompute button. Without one the
    * button does nothing: the viewer owns no computation.
   */

    void setContourRecomputer(ContourRecomputer recompute);

    /// What answers the "propose epsilon" button and fills the coarseness
    /// of each template: the epsilon each cloud asks for, in the project's
    /// plane, and its diameter (TemplateEngine::EpsilonProposal).
    using EpsilonProposer = std::function<std::vector<qftbx::TemplateEngine::EpsilonProposal> ()>;
    void setEpsilonProposer(EpsilonProposer propose);

    /// Where the viewer reads what the last contour computation reported
    /// (TemplateEngine::ContourReport), to mark the frequencies where the
    /// whole template stands in for a contour that did not close.
    using ContourReporter = std::function<std::vector<qftbx::TemplateEngine::ContourReport> ()>;
    void setContourReporter(ContourReporter report);

   /**
    * @fn refreshContour
    * @brief Answer to recomputeRequested: the new contour and the epsilon
    * that produced it, redrawn without rebuilding the frequency colours.
   */

    void refreshContour(const qftbx::CloudSet & contour,
                        std::vector<double> * omega,
                        std::vector<double> * epsilon);


   /// @param templates the plant value set at every design frequency.
    void setTemplates (const qftbx::CloudSet & templates);


   /// @param contour the epsilon-hull of each template.
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
    /// The cloud of a frequency, as the points it is: a value set has no
    /// order, so there is no line to draw through it.
    void plotCloud(const std::vector<double> & phases, const std::vector<double> & magnitudes,
                   qint32 frequency);

    /// And its contour, as the closed curve it is. A QCPCurve and not a
    /// QCPGraph: a graph is a function of its x, and it SORTS its points by
    /// phase, which is what made the line cross the cloud instead of
    /// walking its border - a contour is multivalued in phase by nature.
    void plotContour(const std::vector<double> & phases, const std::vector<double> & magnitudes,
                     qint32 frequency);

    void addFrequencyRow (QColor color, qint32 pos);
    FrequencyLegend * legend = nullptr;
    void clearDiagram();

    //Its OWN copies, not aliases of the project's vectors,
    //which is why a recompute had to be careful about what it freed.
    qftbx::CloudSet m_templates;
    qftbx::CloudSet m_contour;
    std::vector<double> m_omega;
    std::vector<double> m_epsilon;

    //The graphs BELONG TO QCustomPlot, which frees them on clearGraphs():
    //only these containers are the viewer's.
    //The clouds are scatters, where the order of the points does not
    //matter; the contours are curves, where it is everything.
    QVector <QCPGraph *> templateGraphs;
    //One entry per frequency, and inside it one curve per piece of its
    //contour: a cloud with more than one component is walked once per
    //component, so the row of the legend and the curve are not one to one.
    QVector <QVector <QCPCurve *>> contourCurves;
    QMap <qreal, QColor> colorByFrequency;

    ContourRecomputer recompute;

    QVector <QLineEdit *> epsilonEdits;

    //What the buttons say when the card opens: the contour is drawn and the
    //cloud behind it is not. The second flag said the opposite while the
    //drawing hardcoded the truth, so the first press of "Hide contour" only
    //set the text it already had.
    bool templatesVisible = false;
    bool contourVisible = true;


    bool plot = false;
};

} // namespace qftbx

#endif // QFTBX_TEMPLATE_VIEWER_H

