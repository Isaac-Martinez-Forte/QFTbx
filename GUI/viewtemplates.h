

#ifndef VIEWTEMPLATES_H
#define VIEWTEMPLATES_H

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
#include "Librerias/qcustomplot/qcustomplot.h"

#include "cinterval.hpp"
#include "Modelo/LoopShaping/NaturalIntervalExtension/natural_interval_extension.h"


 /**
    * @class ViewTemplates
    * @brief Clase que representa gráficamente los templates de una planta.
    *
    * @author Isaac Martínez Forte
   */

namespace Ui {
class ViewTemplates;
}

class ViewTemplates : public QDialog
{
    Q_OBJECT

public:

   /**
    * @fn ViewTemplates
    * @brief Constructor de la clase.
    *
    * @param parent padre de la clase en la jerarquía gráfica, puede ser vacío.
    *
   */

    explicit ViewTemplates(QWidget *parent = 0);
    ~ViewTemplates();


     /**
    * @fn pintarGrafico
    * @brief Función que crea la gráfica que representa a los templates de una planta.
    *
    * @param diagrama booleano que indica el tipo de diagrama a representar, diagrama de Nichols o diagrama de Nyquist.
   */

    void pintarGrafico(bool diagrama);

   /**
    * @fn setDatos
    * @brief Función que guarda los datos necesarios para crear el gráfico.
    *
    * Esta funcion hace de resumen de otras dos funciones set para no tener que llamarlos por separado.
    *
    * @param templates a representar en la gráfica.
    * @param contorno a representar en la gráfica.
   */

    void setDatos(Controlador *controlador);


   /**
    * @fn setTemplates
    * @brief Función que guarda los templates de la planta para que se representen gráficamente.
    *
    *  @param templates a representar en la gráfica.
    */

    void setTemplates (QVector<QVector<std::complex<qreal> > *> * templates);


   /**
    * @fn setContorno
    * @brief Función que guarda el contorno de los templates de la planta para que se representen gráficamente.
    *
    *  @param contorno de templates a representar gráficamente.
    */

    void setContorno (QVector<QVector<std::complex<qreal> > *> * contorno);


private slots:
    void on_guardar_clicked();

    void on_templates_clicked();

    void on_contorno_clicked();

    void on_eContorno_clicked();

    void revisarCheckBox ();

    void moverSliders();

    void on_recalcular_clicked();

    void on_eTemplate_clicked();

private:
    Ui::ViewTemplates *ui;
    bool ejecutada = false;
    void pintarLinea(qint32 pos, QVector <QCPGraph *> * guardar, QVector <qreal> * fas, QVector <qreal> * gan, bool tipo, bool visible, qint32 contador);
    void pintarCuadro (QColor color, qint32 pos);

    QVector <QVector<std::complex<qreal> > *> * templates;
    QVector <QVector<std::complex<qreal> > *> * contorno;
    QVector <qreal> * omega;
    QVector <qreal> * epsilon;

    QVector <QCPGraph * > * graTemplates;
    QVector <QCPGraph * > * graContorno;
    QGroupBox * cajaFrecuencias;
    QVector <QCheckBox *> * checkbox;
    QMap <qreal, QColor> * colores;

    QVector <QLineEdit *> * lineas;
    QVector <QSlider *> * sliders;

    bool verTemplate;
    bool verContorno;

    Controlador * controlador;

    QVBoxLayout * layoutColores;

    bool diagrama;
    bool color;
};

#endif // VIEWTEMPLATES_H

