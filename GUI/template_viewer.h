

#ifndef QFTBX_TEMPLATE_VIEWER_H
#define QFTBX_TEMPLATE_VIEWER_H

#include <QDialog>
#include <complex>
#include <qmath.h>
#include <QFileDialog>
#include <QMessageBox>
#include <math.h>
#include <QMap>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QCheckBox>

#include "Modelo/Herramientas/tools.h"
#include "Modelo/controlador.h"
#include "qcustomplot.h"

#include "cinterval.hpp"
#include "Modelo/LoopShaping/NaturalIntervalExtension/natural_interval_extension.h"


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
    * @param templatesButton a representar en la gráfica.
    * @param contourButton a representar en la gráfica.
   */

    void setDatos(Controlador *controller);


   /**
    * @fn setTemplates
    * @brief Función que guarda los templatesButton de la planta para que se representen gráficamente.
    *
    *  @param templatesButton a representar en la gráfica.
    */

    void setTemplates (QVector<QVector<std::complex<qreal> > *> * templatesButton);


   /**
    * @fn setContour
    * @brief Función que guarda el contourButton de los templatesButton de la planta para que se representen gráficamente.
    *
    *  @param contourButton de templatesButton a representar gráficamente.
    */

    void setContour (QVector<QVector<std::complex<qreal> > *> * contourButton);


private slots:
    void on_saveImage_clicked();

    void on_templatesButton_clicked();

    void on_contourButton_clicked();

    void on_exportContourButton_clicked();

    void applyCheckboxes ();

    void syncSliders();

    void on_recomputeButton_clicked();

    void on_exportTemplateButton_clicked();

private:
    Ui::TemplateViewer *ui;
    bool plotted = false;
    void plotLine(qint32 pos, QVector <QCPGraph *> * saveImage, QVector <qreal> * fas, QVector <qreal> * gan, bool tipo, bool visible, qint32 contador);
    void addFrequencyRow (QColor colorsCreated, qint32 pos);
    void clearDiagram();

    QVector <QVector<std::complex<qreal> > *> * templatesButton;
    QVector <QVector<std::complex<qreal> > *> * contourButton;
    QVector <qreal> * omega;
    QVector <qreal> * epsilon;

    QVector <QCPGraph * > * templateGraphs;
    QVector <QCPGraph * > * contourGraphs;
    QGroupBox * frequenciesBox;
    QVector <QCheckBox *> * checkboxes;
    QMap <qreal, QColor> * colorByFrequency;

    QVector <QLineEdit *> * epsilonEdits;
    QVector <QSlider *> * epsilonSliders;

    bool templatesVisible;
    bool contourVisible;

    Controlador * controller;

    QVBoxLayout * colorsLayout;

    bool plot;
    bool colorsCreated;
};

#endif // QFTBX_TEMPLATE_VIEWER_H

