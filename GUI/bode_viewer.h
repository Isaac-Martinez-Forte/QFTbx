#ifndef QFTBX_BODE_VIEWER_H
#define QFTBX_BODE_VIEWER_H

#include <QDialog>
#include <QVector>
#include <complex>
#include <qmath.h>
#include <QFileDialog>
#include <QMessageBox>

#include "qcustomplot.h"
#include "src/core/system/lti_system.h"
#include "src/core/math/sequence_vectors.h"
#include "src/core/frequencies/omega.h"

 /**
    * @class BodeViewer
    * @brief Clase gráfica que representa un Diagrama de Bode.
    * Dicha clase se encarga de representar gráficamente un Diagrama de Bode.
    *
    * @author Isaac Martínez Forte
   */

namespace Ui {
class BodeViewer;
}

class BodeViewer : public QDialog
{
    Q_OBJECT

public:
  /**
    * @fn BodeViewer
    * @brief Constructor de la clase.
    *
    * @param parent padre del objeto en la jerarquía gráfica, puede ser vacío.
    */
    explicit BodeViewer(QWidget *parent = 0);
    ~BodeViewer();


  /**
    * @fn drawBode
    * @brief Función que dibuja gráficamente el Diagrama de Bode a partir de los parámetros pasados.
    *
    * @param planta de la cual queremos ver su Diagrama de Bode.
    * @param frequencies necesarias para resolver la Planta.
   */

    void drawBode(LtiSystem * planta, Omega * omega);

private slots:
    void on_actionExport_triggered();

private:
    Ui::BodeViewer *ui;
    void drawAxis(QString yAxisName, const QVector<qreal> & yAxis_values,
                  const QVector<qreal> & frequencies, QCustomPlot * magnitudePlot);
};

#endif // QFTBX_BODE_VIEWER_H
