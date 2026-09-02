

#ifndef QFTBX_TEMPLATE_VIEWER_H
#define QFTBX_TEMPLATE_VIEWER_H

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


 /**
    * @class TemplateViewer
    * @brief Clase que representa gráficamente los templatesButton de una planta.
    *
    * @author Isaac Martínez Forte
   */

namespace Ui {
class TemplateViewer;
}

class TemplateViewer : public QDialog
{
    Q_OBJECT

public:

   /**
    * @fn TemplateViewer
    * @brief Constructor de la clase.
    *
    * @param parent padre de la clase en la jerarquía gráfica, puede ser vacío.
    *
   */

    explicit TemplateViewer(QWidget *parent = 0);
    ~TemplateViewer();


     /**
    * @fn plotDiagram
    * @brief Función que crea la gráfica que representa a los templatesButton de una planta.
    *
    * @param plot booleano que indica el tipo de plot a representar, plot de Nichols o plot de Nyquist.
   */

    void plotDiagram(bool plot);

   /**
    * @fn setDatos
    * @brief Función que guarda los datos necesarios para crear el gráfico.
    *
    * Esta funcion hace de resumen de otras dos funciones set para no tener que llamarlos por separado.
    *
    * The viewer does not reach into the project: it is handed what it draws.
    *
    * @param templates a representar en la gráfica.
    * @param contour a representar en la gráfica.
    * @param omega the design frequencies the templates belong to.
    * @param epsilon the tightening of each frequency, one per omega entry.
   */

    void setDatos(const qftbx::CloudSet & templates,
                  const qftbx::CloudSet & contour,
                  QVector <qreal> * omega,
                  QVector <qreal> * epsilon);

   /**
    * @brief What runs when the user asks for a tighter contour. Ownership of
    * the epsilon vector passes to the handler, which answers with
    * refreshContour().
    *
    * A plain callback rather than a Qt signal: one caller, one handler, same
    * thread. Same seam as tools::ErrorReporter.
   */

    using ContourRecomputer = std::function<void (QVector <qreal> epsilon)>;

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
                        QVector <qreal> * omega,
                        QVector <qreal> * epsilon);


   /**
    * @fn setTemplates
    * @brief Función que guarda los templatesButton de la planta para que se representen gráficamente.
    *
    *  @param templatesButton a representar en la gráfica.
    */

    void setTemplates (const qftbx::CloudSet & templatesButton);


   /**
    * @fn setContour
    * @brief Función que guarda el contourButton de los templatesButton de la planta para que se representen gráficamente.
    *
    *  @param contourButton de templatesButton a representar gráficamente.
    */

    void setContour (const qftbx::CloudSet & contourButton);


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
    void plotLine(qint32 pos, QVector <QCPGraph *> & saveImage, const QVector <qreal> & fas,
                  const QVector <qreal> & gan, bool tipo, bool visible, qint32 contador);
    void addFrequencyRow (QColor color, qint32 pos);
    void clearDiagram();

    //Its own copies now: the viewer used to alias the project's vectors,
    //which is why a recompute had to be careful about what it freed.
    qftbx::CloudSet templatesButton;
    qftbx::CloudSet contourButton;
    QVector <qreal> * omega;
    QVector <qreal> * epsilon;

    //The graphs BELONG TO QCustomPlot, which frees them on clearGraphs():
    //only these containers are the viewer's.
    QVector <QCPGraph *> templateGraphs;
    QVector <QCPGraph *> contourGraphs;
    QGroupBox * frequenciesBox;
    //The controls of a frequency row belong to their container widget: the
    //viewer deletes the rows, not these.
    QVector <QCheckBox *> checkboxes;
    QMap <qreal, QColor> colorByFrequency;

    ContourRecomputer recompute;

    QVector <QLineEdit *> epsilonEdits;
    QVector <QSlider *> epsilonSliders;

    bool templatesVisible;
    bool contourVisible;

    QVBoxLayout * colorsLayout;

    bool plot;
};

#endif // QFTBX_TEMPLATE_VIEWER_H

