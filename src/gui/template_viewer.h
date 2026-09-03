

#ifndef QFTBX_TEMPLATE_VIEWER_H
#define QFTBX_TEMPLATE_VIEWER_H

#include <vector>
#include <memory>

#include <QDialog>
#include <complex>
#include <functional>
#include <qmath.h>
#include <QFileDialog>
#include <QMessageBox>
#include <math.h>
#include <QMap>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QCheckBox>

#include "src/core/math/sequence_vectors.h"
#include "src/core/frequencies/omega.h"
#include "qcustomplot.h"
#include "src/core/templates/cloud_set.h"

#include "cinterval.hpp"
#include "src/core/loopshaping/natural_interval_extension.h"


namespace Ui {
class TemplateViewer;
}

/**
 * @brief Plots the templates of a plant - its value set at every design
 * frequency - and the epsilon-hull contour computed from them.
 *
 * @author Isaac Martínez Forte
 */
class TemplateViewer : public QDialog
{
    Q_OBJECT

public:

    explicit TemplateViewer(QWidget *parent = 0);
    ~TemplateViewer();


   /**
    * @brief Builds the plot.
    *
    * @param plot which plane to draw on: see FC::diagrama - false is
    * Nichols, true is Nyquist.
    */
    void plotDiagram(bool plot);

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
    * thread. Same seam as tools::ErrorReporter.
   */

    using ContourRecomputer = std::function<void (std::vector<double> epsilon)>;

   /**
    * @fn setContourRecomputer
    * @brief Installs the handler of the recompute button. Without one the
    * button does nothing: the viewer owns no computation.
   */

    void setContourRecomputer(ContourRecomputer recompute);

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

    void syncSliders();

    void on_recomputeButton_clicked();


private:
    std::unique_ptr<Ui::TemplateViewer> ui;
    bool plotted = false;
    void plotLine(qint32 pos, QVector <QCPGraph *> & saveImage, const std::vector<double> & fas,
                  const std::vector<double> & gan, bool tipo, bool visible, qint32 frequencyIndex);
    void addFrequencyRow (QColor color, qint32 pos);
    void clearDiagram();

    //Its own copies now: the viewer used to alias the project's vectors,
    //which is why a recompute had to be careful about what it freed.
    qftbx::CloudSet m_templates;
    qftbx::CloudSet m_contour;
    std::vector<double> * omega = nullptr;
    std::vector<double> * epsilon = nullptr;

    //The graphs BELONG TO QCustomPlot, which frees them on clearGraphs():
    //only these containers are the viewer's.
    QVector <QCPGraph *> templateGraphs;
    QVector <QCPGraph *> contourGraphs;
    QGroupBox * frequenciesBox = nullptr;
    //The controls of a frequency row belong to their container widget: the
    //viewer deletes the rows, not these.
    QVector <QCheckBox *> checkboxes;
    QMap <qreal, QColor> colorByFrequency;

    ContourRecomputer recompute;

    QVector <QLineEdit *> epsilonEdits;
    QVector <QSlider *> epsilonSliders;

    bool templatesVisible = false;
    bool contourVisible = false;

    QVBoxLayout * colorsLayout = nullptr;

    bool plot = false;
};

#endif // QFTBX_TEMPLATE_VIEWER_H

