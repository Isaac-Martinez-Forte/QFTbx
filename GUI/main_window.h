#ifndef QFTBX_MAIN_WINDOW_H
#define QFTBX_MAIN_WINDOW_H

#include <QMainWindow>
#include <QApplication>
//#include <QQmlApplicationEngine>

#include "frequencies_dialog.h"
#include "bode_viewer.h"
#include "templates_dialog.h"
#include "template_viewer.h"
#include "boundary_grid_dialog.h"
#include "boundary_viewer.h"
#include "Consola/consola.h"
#include "controller_dialog.h"
#include "boundary_union_viewer.h"
#include "loop_shaping_dialog.h"
#include "loop_shaping_viewer.h"
#include "plant_dialog.h"


 /**
    * @class MainWindow
    * @brief Clase gráfica que representa la pantalla principal del programa
    * 
    * Desde esta pantalla se llama al resto de funcionalidades del sistema.
    * 
    * @author Isaac Martínez Forte
   */


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
  
  /**
    * @fn MainWindow
    * @brief Constructor de la clase que tiene como parámetro el padre de la misma.
    * 
    * 
    * @param parent padre del objeto en la jerarquía gráfica, puede ser vacío.
    */

  
    explicit MainWindow(QWidget *parent = 0);    
    
    /**
    * @fn ~MainWindow
    * @brief Destructor de la clase.
    */
    
    ~MainWindow();

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
    Ui::MainWindow *ui;

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

    Controlador * controlador = nullptr;
    PlantDialog * intPlanta = nullptr;
    FrequenciesDialog * intOmega = nullptr;
    BodeViewer * diagramaBode = nullptr;
    TemplatesDialog * vTemplates = nullptr;
    TemplateViewer * graficoTemplate = nullptr;
    BoundaryGridDialog * datosBoun = nullptr;
    BoundaryViewer * viewBound = nullptr;
    BoundaryUnionViewer * viewBoundReun = nullptr;
    SpecificationsDialog * especificaciones = nullptr;
    ControllerDialog * eControlador = nullptr;
    LoopShapingDialog * loopShaping = nullptr;
    LoopShapingViewer * viewLoopShaping = nullptr;

    QString ficheroGuardar;

    void mostrarLazo (bool nichols, bool nyquist);

    void guardar ();

    void crear();
    void destruir();
    void destroyDialogs();
    void stepBack(bool & paso);

};

#endif // QFTBX_MAIN_WINDOW_H
