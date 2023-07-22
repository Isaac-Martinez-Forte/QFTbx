#ifndef WINDOWSGENERAL_H
#define WINDOWSGENERAL_H

#include <QMainWindow>
#include <QApplication>
//#include <QQmlApplicationEngine>

#include "intomega.h"
#include "diagramabode.h"
#include "introducirtemplates.h"
#include "viewtemplates.h"
#include "intdatosboundaries.h"
#include "viewbound.h"
#include "Consola/consola.h"
#include "introducirecontr.h"
#include "viewboundreun.h"
#include "intloopshaping.h"
#include "viewloopshaping.h"
#include "introducirplanta.h"


 /**
    * @class WindowsGeneral
    * @brief Clase gráfica que representa la pantalla principal del programa
    * 
    * Desde esta pantalla se llama al resto de funcionalidades del sistema.
    * 
    * @author Isaac Martínez Forte
   */


namespace Ui {
class WindowsGeneral;
}

class WindowsGeneral : public QMainWindow
{
    Q_OBJECT
    
public:
  
  /**
    * @fn WindowsGeneral
    * @brief Constructor de la clase que tiene como parámetro el padre de la misma.
    * 
    * 
    * @param parent padre del objeto en la jerarquía gráfica, puede ser vacío.
    */

  
    explicit WindowsGeneral(QWidget *parent = 0);    
    
    /**
    * @fn ~WindowsGeneral
    * @brief Destructor de la clase.
    */
    
    ~WindowsGeneral();

private slots:
    void on_BDefiPlanta_clicked();

    void on_BFrec_clicked();

    void on_BTemp_clicked();

    void on_BBoun_clicked();

    void on_actionGuardar_triggered();

    void on_actionGuardar_como_triggered();

    void on_actionAbrir_triggered();

    void on_actionConsola_triggered();

    void on_BEspi_clicked();

    void on_BDiLaz_clicked();

    void on_BECont_clicked();

    void on_actionNuevo_triggered();

    void on_actionDiagrama_Lazo_Nichols_2_triggered();

    void on_actionDiagrama_Lazo_Nyquist_triggered();

    void on_actionTodos_los_Diagramas_2_triggered();

    void on_actionTemplates_triggered();

    void on_actionBoundaries_triggered();

    void on_actionLazo_triggered();

private:
    Ui::WindowsGeneral *ui;

    bool paso1; //introducir planta
    bool paso2; //introducir especificacione
    bool paso3; //introducir frecuencias de diseño
    bool paso4; //templates
    bool paso5; //Boundaries
    bool paso6; //introducir controlador
    bool paso7; //Ajuste del lazo

    bool digBode;
    bool digconsola;

    bool digBodeFichero;

    qint32 posBarra;

    Controlador * controlador = NULL;
    IntroducirPlanta * intPlanta = NULL;
    IntOmega * intOmega = NULL;
    DiagramaBode * diagramaBode = NULL;
    IntroducirTemplates * vTemplates = NULL;
    ViewTemplates * graficoTemplate = NULL;
    IntDatosBoundaries * datosBoun = NULL;
    ViewBound * viewBound = NULL;
    ViewBoundReun * viewBoundReun = NULL;
    IntEspecificaciones * especificaciones = NULL;
    introducirEContr * eControlador = NULL;
    IntLoopShaping * loopShaping = NULL;
    ViewLoopShaping * viewLoopShaping = NULL;

    QString ficheroGuardar;

    void mostrarLazo (bool nichols, bool nyquist);

    void guardar ();

    void crear();
    void destruir();

};

#endif // WINDOWSGENERAL_H
