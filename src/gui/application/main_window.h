/**
 * @file
 * @brief The main window: the seven design steps, their cards, and the only
 * GUI access to the project.
 *
 * Every form and viewer of the toolbox is created from here, in the order
 * the QFT pipeline imposes; a step whose inputs are missing is refused
 * rather than half-run. The window alone talks to the project, through the
 * ProjectController it owns: the forms describe what the user typed and the
 * viewers draw what they are handed. Each phase is a card holding its form
 * and its diagrams, on a canvas of rows that wrap; which phase is where and
 * how big is written to the settings on close, and once the user has moved
 * a card nothing reorders the canvas behind his back.
 *
 * The settings are copied and immutable here; the default is the compiled
 * one, which the GUI tests use so that a test never inherits the
 * developer's own settings file. The file chooser is replaceable, so the
 * open and save paths can be driven by a test, and isComputing() is what a
 * test waits on. One phase computes at a time, the pipeline being
 * sequential, and what a run produced is drawn back on the GUI thread.
 *
 * Every form, viewer and card has the window as its Qt parent, so Qt owns
 * them; the raw pointers are deliberate, a unique_ptr would free them
 * twice. destroyPhases() deletes them to rebuild them for a new session,
 * always hidden and through deleteLater(), never delete: they are destroyed
 * from refreshAvailability(), which the project calls from inside a
 * widget's own slot, and a card leaves the canvas before it is queued. The
 * templates form is rebuilt over the plant and the design frequencies,
 * discarding what was typed into it, only when the phase is built or
 * either of the two has been applied since.
 */

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

#include "src/app/project_controller.h"

#include "src/gui/application/language.h"

class QMenu;
class QScrollArea;

namespace Ui {
class MainWindow;
}

namespace qftbx {

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:

    explicit MainWindow(qftbx::Settings settings = qftbx::Settings(),
                        QWidget *parent = nullptr);

    ~MainWindow();

public:
    using FileChooser = std::function<QString (bool forSaving)>;

    void setFileChooser(FileChooser choose);

    bool isComputing() const { return m_computing != nullptr; }

private slots:
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

    void on_actionQuit_triggered();

private:
    void changeEvent(QEvent * event) override;

    void openProject(const QString & fileName);

    void buildExamplesMenu();

    void showExamples(const QString & selected);

    void closeEvent(QCloseEvent * event) override;

    bool eventFilter(QObject * watched, QEvent * event) override;

    void retranslate();

    void levelStepButtons();

    std::unique_ptr<Ui::MainWindow> ui;
    QMenu * m_languageMenu = nullptr;
    QMenu * m_themeMenu = nullptr;
    std::vector<std::pair<QString, QAction *>> m_themeActions;
    QMenu * m_examplesMenu = nullptr;
    QAction * m_browseExamplesAction = nullptr;
    QMenu * m_helpMenu = nullptr;
    QAction * m_aboutAction = nullptr;
    QAction * m_aboutQtAction = nullptr;
    std::vector<std::pair<QString, QAction *>> m_languageActions;
#ifdef QFTBX_BENCHMARK
    class BenchmarkWindow * m_benchmark = nullptr;
    QMenu * m_toolsMenu = nullptr;
    QAction * m_plannerAction = nullptr;
#endif

    std::unique_ptr<ProjectController> controller;

    PhaseCard * addPhaseCard(const QString & title, const QString & name,
                             QWidget * form,
                             const std::vector<std::pair<QString, QWidget *>> & views);

    void showPhase(PhaseCard * card);

    void resizeCards();

    void packTemplatesAndSpecifications(int columns);

    void dragCardTo(PhaseCard * card, QPoint where);

    void applyRememberedPlace(PhaseCard * card);

    void rememberCanvas();

    void runInBackground(PhaseCard * card, const QString & what, const QString & title,
                         const std::function<bool (std::function<void ()>)> & start,
                         const std::function<void ()> & collected);

    PhaseCard * m_computing = nullptr;

    void showResults();
    void drawResults();

    bool drawBodeIfPossible();

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

    void destroyCard(PhaseCard *& card);

    QString chooseFile(bool forSaving, const QString & title);

    qftbx::Settings m_settings;

    std::vector<std::pair<QString, int>> m_rememberedCanvas;

    bool m_canvasRemembered = false;

    bool m_canvasOrdered = false;

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

    PhaseCard * plantCard = nullptr;
    PhaseCard * frequenciesCard = nullptr;
    PhaseCard * specificationsCard = nullptr;
    PhaseCard * templatesCard = nullptr;
    PhaseCard * boundariesCard = nullptr;
    PhaseCard * controllerCard = nullptr;
    PhaseCard * loopShapingCard = nullptr;

    QScrollArea * m_canvas = nullptr;
    QWidget * m_canvasContent = nullptr;
    class FlowLayout * m_canvasLayout = nullptr;

    QString saveFilePath;
    QString lastDirectory;

    void saveProject ();

    void refreshAvailability();

    void installContourRecomputer();
    void recomputeContour(std::vector<double> epsilon);

    void createSession();
    void destroySession();

    const std::vector<double> * frequencyValues() const;

    void destroyPhases();

    void ensurePlantPhase();
    void ensureSpecificationsPhase();
    void ensureFrequenciesPhase();
    void ensureTemplatesPhase();

    void relaunchTemplates();
    void ensureBoundariesPhase();
    void ensureControllerPhase();
    void ensureLoopShapingPhase();

};

}

#endif
