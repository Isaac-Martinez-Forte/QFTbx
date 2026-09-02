#ifndef QFTBX_MAIN_WINDOW_H
#define QFTBX_MAIN_WINDOW_H

#include <memory>

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

//The window is the only GUI class that talks to the project: the dialogs
//and viewers are handed what they need and give back what they built.
#include "src/core/project_controller.h"


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
    void on_plantButton_clicked();

    void on_frequenciesButton_clicked();

    void on_templatesButton_clicked();

    void on_boundariesButton_clicked();

    void on_actionSave_triggered();

    void on_actionSaveAs_triggered();

    void on_actionOpen_triggered();

    void on_actionConsole_triggered();

    void on_specificationsButton_clicked();

    void on_loopButton_clicked();

    void on_controllerButton_clicked();

    void on_actionNew_triggered();

    void on_bodeAction_triggered();

    void on_actionNicholsLoop_triggered();

    void on_actionNyquistLoop_triggered();

    void on_actionAllLoopDiagrams_triggered();

    void on_actionTemplates_triggered();

    void on_actionBoundaries_triggered();

    void on_actionLoop_triggered();

private:
    std::unique_ptr<Ui::MainWindow> ui;

    bool plantDone;
    bool specificationsDone;
    bool frequenciesDone;
    bool templatesDone;
    bool boundariesDone;
    bool controllerDone;
    bool loopDone;

    bool bodeCreated;
    bool consoleCreated;

    bool digBodeFichero;

    qint32 progressPosition;

    ProjectController * controller = nullptr;
    PlantDialog * plantDialog = nullptr;
    FrequenciesDialog * frequenciesDialog = nullptr;
    BodeViewer * bodeViewer = nullptr;
    TemplatesDialog * templatesDialog = nullptr;
    TemplateViewer * templateViewer = nullptr;
    BoundaryGridDialog * boundaryGridDialog = nullptr;
    BoundaryViewer * boundaryViewer = nullptr;
    BoundaryUnionViewer * boundaryUnionViewer = nullptr;
    SpecificationsDialog * specificationsDialog = nullptr;
    ControllerDialog * controllerDialog = nullptr;
    LoopShapingDialog * loopShapingDialog = nullptr;
    LoopShapingViewer * loopShapingViewer = nullptr;

    QString saveFilePath;

    void showLoopDiagrams (bool nichols, bool nyquist);

    void saveProject ();

    void invalidateFromTemplates();
    void invalidateFromBoundaries();
    void invalidateLoopShaping();

    void installContourRecomputer();
    void recomputeContour(QVector<qreal> epsilon);

    void createSession();
    void destroySession();

    /// The design frequency values of the project, or nullptr when they have
    /// not been entered yet. The dialogs that need them are given them.
    const QVector<qreal> * frequencyValues() const;

    void destroyDialogs();
    void stepBack(bool & paso);

};

#endif // QFTBX_MAIN_WINDOW_H
