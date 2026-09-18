#ifndef QFTBX_MAIN_WINDOW_H
#define QFTBX_MAIN_WINDOW_H

#include "src/core/project/settings.h"

#include "src/gui/application/phase_card.h"
#include "src/gui/application/step_panel.h"
#include <functional>
#include <vector>
#include <map>
#include <memory>

#include <QByteArray>
#include <QMainWindow>

#include "src/gui/frequencies/frequencies_form.h"
#include "src/gui/plant/bode_viewer.h"
#include "src/gui/templates/templates_form.h"
#include "src/gui/templates/template_viewer.h"
#include "src/gui/boundaries/boundary_grid_form.h"
#include "src/gui/boundaries/boundary_viewer.h"
#include "src/gui/loopshaping/controller_form.h"
#include "src/gui/specifications/specifications_form.h"
#include "src/gui/boundaries/boundary_union_viewer.h"
#include "src/gui/loopshaping/loop_shaping_form.h"
#include "src/gui/loopshaping/loop_shaping_viewer.h"
#include "src/gui/plant/plant_form.h"

//The window is the only GUI class that talks to the project: the dialogs
//and viewers are handed what they need and give back what they built.
#include "src/app/project_controller.h"


#include "src/gui/application/language.h"

class QMenu;
class QScrollArea;

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
     * @brief How a file name gets asked for.
     *
     * QFileDialog's static helpers are modal too, and they are the reason the
     * open and save paths cannot be driven either.
     * @param forSaving true when it is a name to write to, false to read.
     */
    using FileChooser = std::function<QString (bool forSaving)>;

    void setFileChooser(FileChooser choose);

    /// Whether a phase is computing on the worker. The window shows it on
    /// the card; this is what a test waits on.
    bool isComputing() const { return m_computing != nullptr; }

private slots:
    //One per phase: what used to follow the modal dialog, now run when the
    //panel of that phase says the user accepted what it holds.
    void applyPlant();
    void applySpecifications();
    void applyFrequencies();
    void applyTemplates();
    void applyBoundaries();
    void applyController();
    void applyLoopShaping();

    void on_plantButton_clicked();

    void on_frequenciesButton_clicked();

    void on_templatesButton_clicked();

    void on_boundariesButton_clicked();

    void on_actionSave_triggered();

    void on_actionSaveAs_triggered();

    void on_actionOpen_triggered();


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
    /// Retranslates the window when the interface language changes.
    void changeEvent(QEvent * event) override;

    /// Writes the canvas into the settings on the way out.
    void closeEvent(QCloseEvent * event) override;

    /// Watches the canvas for a change of width, which changes the size of
    /// every card on it.
    bool eventFilter(QObject * watched, QEvent * event) override;

    /// The texts this class sets itself, outside the form.
    void retranslate();

    std::unique_ptr<Ui::MainWindow> ui;
    QMenu * m_languageMenu = nullptr;
    QMenu * m_themeMenu = nullptr;
    std::vector<std::pair<QString, QAction *>> m_themeActions;
    QMenu * m_helpMenu = nullptr;
    QAction * m_aboutAction = nullptr;
    QAction * m_aboutQtAction = nullptr;
    std::vector<std::pair<QString, QAction *>> m_languageActions;
#ifdef QFTBX_BENCHMARK
    /// The planner, created on first use; a child window of this one.
    class BenchmarkWindow * m_benchmark = nullptr;
    QMenu * m_toolsMenu = nullptr;
    QAction * m_plannerAction = nullptr;
#endif


    //The facade is the window's own, and the only thing here that is not a
    //Qt child.
    std::unique_ptr<ProjectController> controller;
    //Every dialog and viewer below is created with THIS as its Qt parent,
    //so Qt owns it and frees it with the window. They are raw pointers on
    //purpose: holding one in a unique_ptr would make two owners and free it
    //twice. destroyPhases() deletes them to REBUILD them for a new
    //session, which is Qt's own mechanism, not memory management of ours.
    /**
     * @brief Builds the card of one phase - its form and its diagrams
     * together - and puts it on the canvas.
     *
     * The canvas lays the cards out in rows and wraps what does not fit to
     * the row below, so the phases of a project are on the screen at once
     * and none of them is squeezed to make room for the others.
     */
    PhaseCard * addPhaseCard(const QString & title, const QString & name,
                             QWidget * form,
                             const std::vector<std::pair<QString, QWidget *>> & views);

    /// Scrolls the canvas to a phase and unfolds its form: what pressing a
    /// step does.
    void showPhase(PhaseCard * card);

    /// Works out the square of the canvas from its width and hands it to
    /// every card: how many cards fit across, and how big each one is.
    void resizeCards();

    /// Which of the two phases that may change places goes first, so that
    /// neither leaves a hole in its row. See the implementation.
    void packTemplatesAndSpecifications(int columns);

    /// Moves a card being dragged to where the cursor is, if that is not
    /// where it already is: the hole opens under the cursor while the drag
    /// goes on, and the drop is only the end of it.
    void dragCardTo(PhaseCard * card, QPoint where);

    /// Puts a card where the last session left it, at the size it had.
    void applyRememberedPlace(PhaseCard * card);

    /// Writes the canvas - which phase where, and how big - into the
    /// settings, so the next start comes up as this one was left.
    void rememberCanvas();


    /**
     * @brief Starts a computation on the worker and dresses its card for
     * it, or reports why it cannot start.
     *
     * @param card the phase that is computing: it says so and offers to
     *        give up on it.
     * @param what the line its bar shows while it runs.
     * @param start what actually starts it; it returns false when a run is
     *        already in flight.
     * @param collected what to do on THIS thread once it has finished and
     *        produced a result: draw it.
     */
    void runInBackground(PhaseCard * card, const QString & what, const QString & title,
                         const std::function<bool (std::function<void ()>)> & start,
                         const std::function<void ()> & collected);

    /// The phase that is computing, or nullptr: one at a time, because the
    /// pipeline is sequential.
    PhaseCard * m_computing = nullptr;

    /// Draws in every viewer what the project holds for its step: what a
    /// file that carries results has to show the moment it is opened.
    /// Reports what it cannot draw instead of letting it out of the slot.
    void showResults();
    void drawResults();

    /// Draws the Bode diagram of the plant when there is one to draw, and
    /// says whether there was. Called wherever the plant or the frequencies
    /// change: the diagram is beside them now, not behind a menu.
    bool drawBodeIfPossible();

    /**
     * @brief Destroys a dialog or a viewer and forgets it.
     *
     * deleteLater() and not delete: these are destroyed from
     * refreshAvailability(), which the project calls when it changes, and a
     * project changes from inside a widget's own slot - the template
     * viewer's Recompute button asks for a contour and the window computes
     * it. A plain delete there would free the widget whose slot is still on
     * the stack. Qt destroys it when control is back at the event loop
     * instead, and hiding it first is what makes that invisible.
     */
    template <typename Widget>
    static void destroyLater(Widget *& widget)
    {
        if (widget == nullptr) {
            return;
        }
        widget->hide();
        widget->deleteLater();
        widget = nullptr;
    }

    /**
     * @brief Destroys the card of a phase and forgets it.
     *
     * Like destroyLater(), and one thing more: the card leaves the canvas
     * BEFORE it is queued for deletion. Deferred deletion happens when
     * control is back at the event loop, so between destroying the phases
     * and rebuilding them the canvas would otherwise be laying out cards
     * that are on their way to being freed.
     */
    void destroyCard(PhaseCard *& card);

    /// Asks for a file name through the chooser, or QFileDialog when none.
    QString chooseFile(bool forSaving, const QString & title);

    /// Read once by the application, immutable here.
    qftbx::Settings m_settings;

    /// The canvas of the last session: the name of each phase in the order
    /// it was in, with the size it had.
    std::vector<std::pair<QString, int>> m_rememberedCanvas;

    //Whether that order came from the settings (the user's) or from the
    //default this build starts a canvas with.
    bool m_canvasRemembered = false;

    //And whether the user has moved a card himself, after which nothing
    //reorders the canvas behind his back.
    bool m_canvasOrdered = false;

    /// Whether the plant or the design frequencies have been applied since
    /// the templates form was last built over them.
    bool m_templatesStale = true;

    FileChooser m_chooseFile;

    PlantForm * plantForm = nullptr;
    FrequenciesForm * frequenciesForm = nullptr;
    BodeViewer * bodeViewer = nullptr;
    TemplatesForm * templatesForm = nullptr;
    TemplateViewer * templateViewer = nullptr;
    BoundaryGridForm * boundaryGridForm = nullptr;
    BoundaryViewer * boundaryViewer = nullptr;
    BoundaryUnionViewer * boundaryUnionViewer = nullptr;
    SpecificationsForm * specificationsForm = nullptr;
    ControllerForm * controllerForm = nullptr;
    LoopShapingForm * loopShapingForm = nullptr;
    LoopShapingViewer * loopShapingViewer = nullptr;

    //The card of each phase, created with the widgets it holds. Destroying
    //one destroys them: they are its children once it has them.
    PhaseCard * plantCard = nullptr;
    PhaseCard * frequenciesCard = nullptr;
    PhaseCard * specificationsCard = nullptr;
    PhaseCard * templatesCard = nullptr;
    PhaseCard * boundariesCard = nullptr;
    PhaseCard * controllerCard = nullptr;
    PhaseCard * loopShapingCard = nullptr;

    /// The canvas the cards live on: rows that wrap, and a scrollbar for
    /// what does not fit downwards.
    QScrollArea * m_canvas = nullptr;
    QWidget * m_canvasContent = nullptr;
    class FlowLayout * m_canvasLayout = nullptr;

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

    void destroyPhases();

    //One per step: the dialog (and viewers) of a step, created on first use
    //with the settings applied, and reused afterwards. Each of these blocks
    //was written twice, in the step's handler and in the open handler.
    void ensurePlantPhase();
    void ensureSpecificationsPhase();
    void ensureFrequenciesPhase();
    void ensureTemplatesPhase();

    /// Builds the templates form over the plant and the design frequencies as
    /// they stand now, which throws away what was typed into it. The
    /// templates are made FROM those two - which parameters there are, the
    /// grids over them, the epsilon proposed on those grids - so this is the
    /// one phase whose form cannot simply be left as the user wrote it: what
    /// he wrote may be about a plant that is no longer there. Called when
    /// the phase is built and when either of the two has been applied since
    /// (m_templatesStale), and at no other time.
    void relaunchTemplates();
    void ensureBoundariesPhase();
    void ensureControllerPhase();
    void ensureLoopShapingPhase();

};

} // namespace qftbx

#endif // QFTBX_MAIN_WINDOW_H
