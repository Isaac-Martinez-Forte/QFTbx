#ifndef QFTBX_MAIN_WINDOW_H
#define QFTBX_MAIN_WINDOW_H

#include "src/core/project/settings.h"
#include <QDialog>

#include "src/gui/application/step_dialog.h"
#include <functional>
#include <vector>
#include <memory>

#include <QMainWindow>

#include "src/gui/frequencies/frequencies_dialog.h"
#include "src/gui/plant/bode_viewer.h"
#include "src/gui/templates/templates_dialog.h"
#include "src/gui/templates/template_viewer.h"
#include "src/gui/boundaries/boundary_grid_dialog.h"
#include "src/gui/boundaries/boundary_viewer.h"
#include "src/gui/application/muparserx_console.h"
#include "src/gui/loopshaping/controller_dialog.h"
#include "src/gui/specifications/specifications_dialog.h"
#include "src/gui/boundaries/boundary_union_viewer.h"
#include "src/gui/loopshaping/loop_shaping_dialog.h"
#include "src/gui/loopshaping/loop_shaping_viewer.h"
#include "src/gui/plant/plant_dialog.h"

//The window is the only GUI class that talks to the project: the dialogs
//and viewers are handed what they need and give back what they built.
#include "src/app/project_controller.h"


namespace Ui {
class MainWindow;
}

namespace qftbx {


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
  
    /**
     * @brief Builds the window with the settings the application read.
     * @param settings copied and kept: they are immutable, and every dialog
     *        that has a configurable limit is handed the values it needs when
     *        it is built. The default is the compiled defaults, which is what
     *        the GUI tests use - a test must never inherit the developer's
     *        own settings file.
     */
    explicit MainWindow(qftbx::Settings settings = qftbx::Settings(),
                        QWidget *parent = nullptr);

    ~MainWindow();

public:
    /**
     * @brief How a step's dialog gets shown.
     *
     * By default it is QDialog::exec(): modal and BLOCKING, which is exactly
     * what the application wants and exactly what a headless test cannot
     * survive - a test that presses a step button would hang there for ever.
     * That is why nothing in this window has ever been tested beyond the fact
     * that it builds its widget tree.
     *
     * With this, a test installs its own: fill the dialog's fields by name and
     * press its OK button, which is what the dialog smoke tests already do,
     * and the handler carries on as if a user had done it. Same seam as
     * qftbx::ErrorReporter and TemplateViewer's ContourRecomputer - a plain
     * callback, one caller, one handler, same thread.
     */
    using DialogRunner = std::function<void (QDialog * dialog)>;

    void setDialogRunner(DialogRunner run);

    /**
     * @brief How a file name gets asked for.
     *
     * QFileDialog's static helpers are modal too, and they are the reason the
     * open and save paths cannot be driven either.
     * @param forSaving true when it is a name to write to, false to read.
     */
    using FileChooser = std::function<QString (bool forSaving)>;

    void setFileChooser(FileChooser choose);

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


    //The facade is the window's own, and the only thing here that is not a
    //Qt child.
    std::unique_ptr<ProjectController> controller;
    //Every dialog and viewer below is created with THIS as its Qt parent,
    //so Qt owns it and frees it with the window. They are raw pointers on
    //purpose: holding one in a unique_ptr would make two owners and free it
    //twice. destroyDialogs() deletes them to REBUILD them for a new
    //session, which is Qt's own mechanism, not memory management of ours.
    /// Shows a dialog through the runner, or exec() when there is none.
    void runDialog(StepDialog * dialog);

    /// Asks for a file name through the chooser, or QFileDialog when none.
    QString chooseFile(bool forSaving, const QString & title);

    /// Read once by the application, immutable here.
    qftbx::Settings m_settings;

    DialogRunner m_runDialog;
    FileChooser m_chooseFile;

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


    /// Buttons and progress bar from ProjectController::completed(). See the
    /// definition for what it replaces.
    void refreshAvailability();

    void installContourRecomputer();
    void recomputeContour(std::vector<double> epsilon);

    void createSession();
    void destroySession();

    /// The design frequency values of the project, or nullptr when they have
    /// not been entered yet. The dialogs that need them are given them.
    const std::vector<double> * frequencyValues() const;

    void destroyDialogs();

    //One per step: the dialog (and viewers) of a step, created on first use
    //with the settings applied, and reused afterwards. Each of these blocks
    //was written twice, in the step's handler and in the open handler.
    void ensurePlantDialog();
    void ensureSpecificationsDialog();
    void ensureFrequenciesDialog();
    void ensureTemplatesWidgets();
    void ensureBoundariesWidgets();
    void ensureControllerDialog();
    void ensureLoopShapingWidgets();

};

} // namespace qftbx

#endif // QFTBX_MAIN_WINDOW_H
