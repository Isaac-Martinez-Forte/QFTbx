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
#include "muparserx_console.h"
#include "controller_dialog.h"
#include "boundary_union_viewer.h"
#include "loop_shaping_dialog.h"
#include "loop_shaping_viewer.h"
#include "plant_dialog.h"

//The window is the only GUI class that talks to the project: the dialogs
//and viewers are handed what they need and give back what they built.
#include "src/core/project_controller.h"


namespace Ui {
class MainWindow;
}

/**
 * @brief The main window: the seven design steps as menu entries, and the
 * only place the GUI reaches the project through ProjectController.
 *
 * Every dialog and viewer of the toolbox is opened from here, in the order
 * the QFT pipeline imposes - a step whose inputs are missing is refused
 * rather than half-run.
 *
 * @author Isaac Martínez Forte
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
  
    explicit MainWindow(QWidget *parent = 0);

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

    bool plantDone = false;
    bool specificationsDone = false;
    bool frequenciesDone = false;
    bool templatesDone = false;
    bool boundariesDone = false;
    bool controllerDone = false;
    bool loopDone = false;

    bool bodeCreated = false;


    qint32 progressPosition = 0;

    //The facade is the window's own, and the only thing here that is not a
    //Qt child.
    std::unique_ptr<ProjectController> controller;
    //Every dialog and viewer below is created with THIS as its Qt parent,
    //so Qt owns it and frees it with the window. They are raw pointers on
    //purpose: holding one in a unique_ptr would make two owners and free it
    //twice. destroyDialogs() deletes them to REBUILD them for a new
    //session, which is Qt's own mechanism, not memory management of ours.
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
