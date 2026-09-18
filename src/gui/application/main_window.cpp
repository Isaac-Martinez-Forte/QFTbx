#include "src/gui/loopshaping/loop_boundaries_viewer.h"
#include "src/gui/application/main_window.h"
#include "src/gui/common/flow_layout.h"
#include "src/gui/application/about.h"
#include "src/gui/application/theme.h"
#include "src/gui/common/number_text.h"
#include "src/gui/common/plot_setup.h"
#include "qcustomplot.h"
#include "src/gui/application/error_message.h"
#include "ui_main_window.h"
#ifdef QFTBX_BENCHMARK
#include "src/gui/bench/benchmark_window.h"
#include <QMenuBar>
#endif
#include <QMenu>

#include <QActionGroup>
#include <QCloseEvent>
#include <QApplication>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QEvent>
#include <QFileDialog>
#include <QMessageBox>

#include "src/core/math/point.h"
#include "src/core/common/exception.h"
#include "src/core/pipeline/pipeline_step.h"


#include <QPushButton>
#include <algorithm>
#include <vector>


namespace qftbx {

namespace {

//The wait cursor as an object, so it comes back whatever way the scope ends.
//It was set and unset by hand, and the arrow appeared THREE times in the
//template handler alone - once per way out - which is a shape where any new
//return leaves the hourglass spinning over a window that is done working.
class WaitCursor
{
public:
    explicit WaitCursor(QWidget * widget) : m_widget(widget)
    {
        if (m_widget != nullptr) {
            m_widget->setCursor(Qt::WaitCursor);
        }
    }

    ~WaitCursor()
    {
        if (m_widget != nullptr) {
            m_widget->setCursor(Qt::ArrowCursor);
        }
    }

    WaitCursor(const WaitCursor &) = delete;
    WaitCursor & operator=(const WaitCursor &) = delete;

private:
    QWidget * m_widget;
};

}

MainWindow::MainWindow(qftbx::Settings settings, QWidget *parent) :
    QMainWindow(parent),
    ui(std::make_unique<Ui::MainWindow>()),
    m_settings(std::move(settings))
{
    
    ui->setupUi(this);

    //How many digits of a number the forms show. One answer for the whole
    //interface, taken from the settings once: the files keep every digit
    //whatever this says.
    setShownDigits(m_settings.interface.digits);

    //The look, under View: the machine's own, or the toolbox's light and
    //dark. Choosing one dresses every window on the spot and writes the
    //choice into the settings file in use, like the language below.
    m_themeMenu = ui->menuView->addMenu(QString());
    auto * themes = new QActionGroup(this);
    const QString wearing = QString::fromStdString(m_settings.interface.theme);

    for (const QString & code : availableThemes()) {
        QAction * action = m_themeMenu->addAction(themeName(code));
        action->setObjectName(QStringLiteral("actionTheme_%1").arg(code));
        action->setCheckable(true);
        action->setChecked(code == wearing || (code == kSystemTheme && !isAvailableTheme(wearing)));
        themes->addAction(action);
        m_themeActions.emplace_back(code, action);
        connect(action, &QAction::triggered, this, [this, code]() {
            applyTheme(code);
            //The diagrams that already exist take the new colours too: the
            //style sheet does not reach inside a QCustomPlot. The
            //application's palette and not this window's, which Qt has not
            //updated yet while this runs.
            for (QCustomPlot * plot : findChildren<QCustomPlot *>()) {
                applyPlotPalette(*plot, QApplication::palette());
            }
            m_settings.interface.theme = code.toStdString();
            try {
                storeTheme(code, m_settings.source);
            } catch (const qftbx::Exception & failure) {
                errorMessage(translated(failure), tr("Appearance"));
            }
        });
    }

    //The interface language, under View: the system's, English or Spanish.
    //Choosing one installs the translators and retranslates this window on
    //the spot; the dialogs are built when they open.
    m_languageMenu = ui->menuView->addMenu(QString());
    auto * group = new QActionGroup(this);
    const QString chosen = QString::fromStdString(m_settings.interface.language);
    for (const QString & code : availableLanguages()) {
        QAction * action = m_languageMenu->addAction(languageName(code));
        action->setObjectName(QStringLiteral("actionLanguage_%1").arg(code));
        action->setCheckable(true);
        action->setChecked(code == chosen || (code == kSystemLanguage && !isAvailableLanguage(chosen)));
        group->addAction(action);
        m_languageActions.emplace_back(code, action);
        connect(action, &QAction::triggered, this, [this, code]() {
            applyLanguage(code);
            m_settings.interface.language = code.toStdString();
            try {
                storeLanguage(code, m_settings.source);
            } catch (const qftbx::Exception & failure) {
                errorMessage(translated(failure), tr("Language"));
            }
        });
    }
    retranslate();

#ifdef QFTBX_BENCHMARK
    //The benchmark planner, when the build carries it: a window of its own,
    //non-modal, so a plan can run while the project is worked on.
    m_toolsMenu = menuBar()->addMenu(tr("&Tools"));
    m_plannerAction = m_toolsMenu->addAction(tr("Benchmark &planner..."));
    m_plannerAction->setObjectName("actionBenchmarkPlanner");
    connect(m_plannerAction, &QAction::triggered, this, [this]() {
        if (m_benchmark == nullptr) {
            m_benchmark = new BenchmarkWindow(this);
            m_benchmark->setWindowFlag(Qt::Window);
        }
        m_benchmark->show();
        m_benchmark->raise();
        m_benchmark->activateWindow();
    });
#endif

    //Help, last as menus go: what the toolbox is and who wrote it, and
    //Qt's own box (its text is Qt's, in whatever languages Qt ships).
    m_helpMenu = menuBar()->addMenu(QString());
    m_aboutAction = m_helpMenu->addAction(QString());
    m_aboutAction->setObjectName("actionAbout");
    connect(m_aboutAction, &QAction::triggered, this, [this]() { showAbout(this); });
    m_aboutQtAction = m_helpMenu->addAction(QString());
    m_aboutQtAction->setObjectName("actionAboutQt");
    connect(m_aboutQtAction, &QAction::triggered, this, []() { QApplication::aboutQt(); });
    retranslate();

    //The canvas the phases live on: a row of cards that wraps to the next
    //row when the window is too narrow for one more, and scrolls
    //downwards. The steps used to be modal dialogs, then docks that divided
    //the window between them; a project with six phases came back as six
    //slivers of 300 pixels because a dock area shares out what it has.
    m_canvasContent = new QWidget;
    m_canvasContent->setObjectName("canvasContent");
    m_canvasLayout = new FlowLayout(m_canvasContent);

    m_canvas = new QScrollArea(this);
    m_canvas->setObjectName("canvas");
    m_canvas->setWidget(m_canvasContent);
    m_canvas->setWidgetResizable(true);
    m_canvas->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_canvas->setFrameShape(QFrame::NoFrame);
    m_canvas->viewport()->installEventFilter(this);

    //Under the strip of steps, which keeps its own height.
    ui->centralwidget->layout()->addWidget(m_canvas);

    //How the canvas was left the last time: which phase where, and how big
    //each one was. A line nobody types by hand, so anything in it that this
    //build does not recognise is ignored rather than refused.
    for (const QString & entry : QString::fromStdString(m_settings.interface.canvas)
                                     .split(' ', Qt::SkipEmptyParts)) {
        const QStringList parts = entry.split(':');
        if (parts.size() == 2) {
            m_rememberedCanvas.emplace_back(parts.at(0), parts.at(1).toInt());
        }
    }

    //Nothing remembered - a first run, or a settings file that is not
    //being written - and the canvas opens in the order that packs: the
    //specifications are worth two squares, so they follow the templates
    //instead of leaving a hole beside them on a screen three columns wide.
    //The order of the DESIGN is the strip of steps above; this is the order
    //of the wall, and one drag of the user's replaces it.
    m_canvasRemembered = !m_rememberedCanvas.empty();

    if (m_rememberedCanvas.empty()) {
        for (const char * name : {"plantCard", "frequenciesCard", "templatesCard",
                                  "specificationsCard", "boundariesCard", "controllerCard",
                                  "loopShapingCard"}) {
            m_rememberedCanvas.emplace_back(QString::fromLatin1(name), 1);
        }
    }

    //And how big it was when it closed, so it comes up the same size. A
    //size this build cannot read is no reason to refuse to start: the
    //window opens at the size its form was drawn at.
    const QString window = QString::fromStdString(m_settings.interface.window).trimmed();

    if (window == "maximized") {
        setWindowState(windowState() | Qt::WindowMaximized);
    } else {
        const QStringList size = window.split(' ', Qt::SkipEmptyParts);
        if (size.size() == 2 && size.at(0).toInt() > 0 && size.at(1).toInt() > 0) {
            resize(size.at(0).toInt(), size.at(1).toInt());
        }
    }

    //7 real steps: with the 0-8 range the bar never reached 100%.
    ui->progressBar->setRange(0,7);

    createSession();
}

MainWindow::~MainWindow()
{
    destroySession();
}

//The canvas decides how big a card is, from how wide it is itself: three
//cards across a wide screen, one across a narrow one, and always filling
//the width instead of leaving a ragged margin.
bool MainWindow::eventFilter(QObject * watched, QEvent * event)
{
    if (m_canvas != nullptr && watched == m_canvas->viewport() && event->type() == QEvent::Resize) {
        resizeCards();
    }

    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::resizeCards()
{
    if (m_canvasContent == nullptr) {
        return;
    }

    const QMargins margins = m_canvasLayout->contentsMargins();
    const int room = m_canvas->viewport()->width() - margins.left() - margins.right();
    const QSize unit = PhaseCard::unitFor(room);

    for (PhaseCard * card : m_canvasContent->findChildren<PhaseCard *>()) {
        card->setUnit(unit);
    }

    packTemplatesAndSpecifications(PhaseCard::columnsFor(room));
}

/**
 * @brief Which of the templates and the specifications comes first, so that
 * neither leaves a hole beside it.
 *
 * The specifications are the one phase worth two squares, so they are the
 * one phase that can fall off the end of a row. The templates are the one
 * phase that can change places with them without lying about the design:
 * the two do not depend on each other, and everything else does - the
 * boundaries are computed FROM the specifications, and a wall that showed
 * them first would be telling the user the wrong story about the order of
 * the work.
 *
 * So: the specifications go first when they fit in what is left of the row
 * they would start in, and the templates go first when they do not, filling
 * that last square themselves and leaving the specifications a row of their
 * own. On three columns that is plant, frequencies, templates / two squares
 * of specifications and the boundaries; on two, plant and frequencies / the
 * specifications whole / templates and boundaries. Neither leaves a hole.
 *
 * Only while the user has not ordered the canvas himself: from the first
 * drag on, the order is his.
 */
void MainWindow::packTemplatesAndSpecifications(int columns)
{
    if (m_canvasOrdered || m_canvasLayout == nullptr || columns < 1) {
        return;
    }

    const int templates = m_canvasLayout->indexOf(templatesCard);
    const int specifications = m_canvasLayout->indexOf(specificationsCard);

    if (templates < 0 || specifications < 0) {
        return;
    }

    //How many squares the phases in front of the pair take, so that we know
    //where in its row the first of the two would land.
    const int first = std::min(templates, specifications);
    int used = 0;
    for (int i = 0; i < first; ++i) {
        if (auto * card = qobject_cast<PhaseCard *>(m_canvasLayout->itemAt(i)->widget())) {
            used += card->span();
        }
    }

    const int left = columns - used % columns;
    const bool specificationsFirst = left >= specificationsCard->span();

    const int wanted = specificationsFirst ? specifications : templates;
    if (wanted != first) {
        m_canvasLayout->move(wanted, first);
        m_canvasContent->updateGeometry();
    }
}

//On the way out: the canvas as the user left it, into the settings file in
//use. Without one to write to - a build run with no settings at all, which
//is what the tests do - there is nowhere to put it and nothing to do.
void MainWindow::closeEvent(QCloseEvent * event)
{
    //A run in flight is given up on before the window goes: the project
    //joins its worker when it dies, and a search that had forty minutes
    //left would hold the whole application closing for them.
    if (controller != nullptr && controller->isComputing()) {
        controller->cancelComputation();
        controller->waitForComputation();
    }

    rememberCanvas();
    QMainWindow::closeEvent(event);
}

void MainWindow::rememberCanvas()
{
    if (m_settings.source.empty() || m_canvasLayout == nullptr) {
        return;
    }

    QStringList entries;
    for (int i = 0; i < m_canvasLayout->count(); ++i) {
        const QLayoutItem * item = m_canvasLayout->itemAt(i);
        if (auto * card = qobject_cast<PhaseCard *>(item->widget())) {
            entries << card->objectName() + ":" + QString::number(card->span());
        }
    }

    const QString window = isMaximized()
            ? QString("maximized")
            : QString("%1 %2").arg(width()).arg(height());

    try {
        qftbx::writeSetting(m_settings.source, "interface.canvas", entries.join(' ').toStdString());
        qftbx::writeSetting(m_settings.source, "interface.window", window.toStdString());
    } catch (const qftbx::Exception & failure) {
        //The layout is not worth a complaint on the way out.
        (void) failure;
    }
}

void MainWindow::changeEvent(QEvent * event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        retranslate();
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::relaunchTemplates()
{
    if (templatesForm == nullptr || controller->plant() == nullptr
            || controller->omega() == nullptr) {
        return;
    }

    templatesForm->setEpsilonMetric(controller->epsilonMetric());
    templatesForm->launch(controller->plant(),
                          qint32(controller->omega()->values()->size()));
    m_templatesStale = false;
}

void MainWindow::retranslate()
{
    setWindowTitle(tr("QFT: Quantitative feedback theory"));
    m_languageMenu->setTitle(tr("&Language"));
    if (m_themeMenu != nullptr) {
        m_themeMenu->setTitle(tr("&Appearance"));
        for (auto & [code, action] : m_themeActions) {
            action->setText(themeName(code));
        }
    }
    for (auto & [code, action] : m_languageActions) {
        action->setText(languageName(code));
    }
    if (m_helpMenu != nullptr) {
        m_helpMenu->setTitle(tr("&Help"));
        m_aboutAction->setText(tr("&About QFTbx..."));
        m_aboutQtAction->setText(tr("About &Qt..."));
    }
    levelStepButtons();
#ifdef QFTBX_BENCHMARK
    if (m_toolsMenu != nullptr) {
        m_toolsMenu->setTitle(tr("&Tools"));
        m_plannerAction->setText(tr("Benchmark &planner..."));
    }
#endif
}

void MainWindow::levelStepButtons()
{
    int tallest = 0;
    for (QPushButton * step : ui->stepsLayout->parentWidget()->findChildren<QPushButton *>()) {
        if (ui->stepsLayout->indexOf(step) >= 0) {
            step->setMinimumHeight(0);
            tallest = std::max(tallest, step->sizeHint().height());
        }
    }
    for (QPushButton * step : ui->stepsLayout->parentWidget()->findChildren<QPushButton *>()) {
        if (ui->stepsLayout->indexOf(step) >= 0) {
            step->setMinimumHeight(tallest);
        }
    }
}

void MainWindow::createSession(){

    controller = std::make_unique<ProjectController>();

    //Told by the project rather than remembered at every way out: what is on
    //screen is derived from what the project holds, so the one place that
    //knows it moved is the project itself. A session outlives no window, so
    //the captured this is always alive when the handler runs.
    controller->setChangeHandler([this]() { refreshAvailability(); });

    //What the core needs of the settings; the dialogs get theirs when they
    //are built.
    controller->applySettings(m_settings);

    //A new project measures its contour epsilon in the plane the settings
    //say (the Nichols plane unless told otherwise); a loaded one keeps the
    //plane its file says.
    controller->setEpsilonMetric({m_settings.defaults.epsilonInNichols ? qftbx::HullMetric::Nichols
                                                                        : qftbx::HullMetric::ComplexPlane,
                                  m_settings.defaults.dbPerDegree});

    //An empty project: every step undone, so this switches the buttons off
    //and puts the bar at zero without enumerating either.
    refreshAvailability();

    //Without this, Save after New overwrote the last opened file with the
    //empty project.
    saveFilePath.clear();
}

//Qt's own mechanism: every dialog and viewer here is a child of this window,
//and destroying one is how a new session gets a fresh one. The POINTER
//decides, with no progress flag to consult: a flag beside it is a second
//answer to the same question and the two drift apart. See destroyLater() for
//why they are not deleted outright.
void MainWindow::destroyCard(PhaseCard *& card)
{
    if (card == nullptr) {
        return;
    }

    card->hide();
    m_canvasLayout->removeWidget(card);
    card->setParent(nullptr);
    card->deleteLater();
    card = nullptr;
}

void MainWindow::destroyPhases(){
    //The dock owns the form and the diagrams of its phase, so destroying it
    //destroys them: what is left here is forgetting the pointers, which are
    //about to dangle.
    destroyCard(plantCard);
    destroyCard(frequenciesCard);
    destroyCard(specificationsCard);
    destroyCard(templatesCard);
    destroyCard(boundariesCard);
    destroyCard(controllerCard);
    destroyCard(loopShapingCard);

    plantForm = nullptr;
    bodeViewer = nullptr;
    frequenciesForm = nullptr;
    specificationsForm = nullptr;
    templatesForm = nullptr;
    templateViewer = nullptr;
    boundaryGridForm = nullptr;
    boundaryViewer = nullptr;
    boundaryUnionViewer = nullptr;
    controllerForm = nullptr;
    loopShapingForm = nullptr;
    loopShapingViewer = nullptr;

}

//One phase, one card: what it was asked for and what came out of it, on
//the canvas with the others.
PhaseCard * MainWindow::addPhaseCard(const QString & title, const QString & name,
                                     QWidget * form,
                                     const std::vector<std::pair<QString, QWidget *>> & views)
{
    auto * card = new PhaseCard(title, name, form, views, m_canvasContent);

    //Unfolding a form makes its card bigger, and the canvas moves the rest
    //out of the way instead of squeezing anybody.
    connect(card, &PhaseCard::sizeChanged, this, [this]() {
        m_canvasLayout->invalidate();
        m_canvasContent->updateGeometry();
    });

    //And the button that gives up on the computation of this phase, which
    //is the only thing its bar offers while one is running.
    connect(card, &PhaseCard::cancelAsked, this, [this]() {
        controller->cancelComputation();
    });

    connect(card, &PhaseCard::closeAsked, this, [this, card]() {
        card->hide();
        m_canvasLayout->invalidate();
        m_canvasContent->updateGeometry();
    });

    //Dragged by its bar, a card changes places with the one the cursor is
    //over, while the drag is still going on.
    connect(card, &PhaseCard::draggedTo, this, [this, card](QPoint where) {
        dragCardTo(card, where);
    });

    m_canvasLayout->addWidget(card);
    applyRememberedPlace(card);

    //Built after the canvas has a width of its own: it is told the square
    //it counts in right away, and not at the next resize.
    resizeCards();

    return card;
}

//A card built now goes where the last session left it, at the size it had
//there. The cards appear in the order of the design, so a card whose place
//is further back waits for the ones in front of it to be built: the order
//is applied against the names, not against how many there are yet.
void MainWindow::applyRememberedPlace(PhaseCard * card)
{
    const auto remembered = std::find_if(m_rememberedCanvas.begin(), m_rememberedCanvas.end(),
                                         [card](const auto & entry) {
                                             return entry.first == card->objectName();
                                         });

    if (remembered == m_rememberedCanvas.end()) {
        return;
    }

    //A size restored from the last session is the user's and stays; the one
    //a first run starts from leaves the card free to ask for two squares
    //when its form does not fit in one.
    card->setSpan(remembered->second, m_canvasRemembered);

    //Its place among the cards that ARE there: how many of the ones before
    //it in the remembered order have been built.
    int place = 0;
    for (auto entry = m_rememberedCanvas.begin(); entry != remembered; ++entry) {
        if (m_canvasContent->findChild<PhaseCard *>(entry->first) != nullptr) {
            ++place;
        }
    }

    m_canvasLayout->move(m_canvasLayout->indexOf(card), place);
}

//Where the cursor is, in the canvas: the card under it changes places with
//the one being dragged, and the layout opens the hole by itself.
void MainWindow::dragCardTo(PhaseCard * card, QPoint where)
{
    const int from = m_canvasLayout->indexOf(card);
    const QPoint inside = m_canvasContent->mapFromGlobal(where);

    int to = m_canvasLayout->indexAt(inside);

    if (from < 0 || to < 0) {
        return;
    }

    //The index was read with the card still in its old place, so anything
    //past it has to come back one.
    if (to > from) {
        --to;
    }

    if (to == from) {
        return;
    }

    //From here on the order of the wall is the user's, and nothing rearranges
    //it behind him.
    m_canvasOrdered = true;

    m_canvasLayout->move(from, to);
    m_canvasContent->updateGeometry();
}

//Pressing a step is going to that phase to enter something: its card
//unfolds its form and the canvas scrolls to it.
void MainWindow::showPhase(PhaseCard * card)
{
    if (card == nullptr) {
        return;
    }

    card->show();
    card->showForm(true);
    m_canvasLayout->invalidate();
    m_canvasContent->updateGeometry();
    m_canvas->ensureWidgetVisible(card);
}

//The POINTER says whether a step's widgets exist, which is the question
//being asked. The flag it replaced answered a different one - whether the
//step was done - and the two only agreed by being maintained together.
void MainWindow::ensurePlantPhase()
{
    if (plantForm == nullptr){
        plantForm = new PlantForm(this);
        bodeViewer = new BodeViewer(this);
        connect(plantForm, &StepPanel::accepted, this, &MainWindow::applyPlant);
        plantCard = addPhaseCard(tr("Plant"), "plantCard", plantForm,
                                 {{tr("Bode"), bodeViewer}});
        plantForm->setFromProject(controller->plant());
    }
}

void MainWindow::ensureSpecificationsPhase()
{
    if (specificationsForm == nullptr){
        specificationsForm = new SpecificationsForm(frequencyValues(),
                                                        controller->specifications(),
                                                        this);
        connect(specificationsForm, &StepPanel::accepted, this, &MainWindow::applySpecifications);
        specificationsCard = addPhaseCard(tr("Specifications"), "specificationsCard",
                                          specificationsForm, {});
    }
}

void MainWindow::ensureFrequenciesPhase()
{
    if (frequenciesForm == nullptr){
        frequenciesForm = new FrequenciesForm(this);
        frequenciesForm->applyFrequencyCountLimit(m_settings.limits.maxFrequencyCount);
        connect(frequenciesForm, &StepPanel::accepted, this, &MainWindow::applyFrequencies);
        frequenciesCard = addPhaseCard(tr("Design frequencies"), "frequenciesCard",
                                       frequenciesForm, {});
        frequenciesForm->setFromProject(controller->omega());
    }
}

void MainWindow::ensureTemplatesPhase()
{
    if (templatesForm == nullptr){
        templatesForm = new TemplatesForm(this);
        templatesForm->setMaxPointCount(m_settings.limits.maxTemplatePoints);
        templatesForm->setDefaultPointCount(m_settings.defaults.templatePointCount);
        templatesForm->setWholeTemplateIfNoContour(m_settings.algorithms.wholeTemplateIfNoContour);
        templatesForm->setAlphaShapeContour(m_settings.algorithms.alphaShapeContour);
        templatesForm->setBorderSweep(m_settings.algorithms.borderSweep);
        //The field opens with the epsilon the family asks for: a sweep over
        //the grids as entered, the same one OK runs next.
        templatesForm->setEpsilonProposer([this](const qftbx::ParameterGrids & grids,
                                                   qftbx::EpsilonMetric metric) {
            const WaitCursor waiting(this);
            //The proposal depends on how the contour is extracted.
            controller->setAlphaShapeContour(templatesForm->alphaShapeContour());
            controller->setBorderSweep(templatesForm->borderSweep());
            return controller->proposeEpsilon(grids, metric);
        });
        templateViewer = new TemplateViewer(this);
        installContourRecomputer();
        connect(templatesForm, &StepPanel::accepted, this, &MainWindow::applyTemplates);
        templatesCard = addPhaseCard(tr("Templates"), "templatesCard", templatesForm,
                                     {{tr("Templates"), templateViewer}});
        relaunchTemplates();
    }
}

void MainWindow::ensureBoundariesPhase()
{
    if (boundaryGridForm == nullptr){
        boundaryGridForm = new BoundaryGridForm(this);
        boundaryGridForm->setMaxGridCells(m_settings.limits.maxGridCells);
        boundaryGridForm->applyDefaults(m_settings.defaults);
        boundaryViewer = new BoundaryViewer(this);
        boundaryUnionViewer = new BoundaryUnionViewer(this);
        connect(boundaryGridForm, &StepPanel::accepted, this, &MainWindow::applyBoundaries);
        //The union first: it is the answer - what the loop has to clear at
        //every frequency at once - and the per-frequency view is where you
        //go to see which boundary came from where.
        boundariesCard = addPhaseCard(tr("Boundaries"), "boundariesCard", boundaryGridForm,
                                      {{tr("Union"), boundaryUnionViewer},
                                       {tr("Per frequency"), boundaryViewer}});
        boundaryGridForm->setFromProject(controller->boundaries());
    }
}

void MainWindow::ensureControllerPhase()
{
    if (controllerForm == nullptr){
        controllerForm = new ControllerForm(this);
        connect(controllerForm, &StepPanel::accepted, this, &MainWindow::applyController);
        controllerCard = addPhaseCard(tr("Controller structure"), "controllerCard",
                                      controllerForm, {});
        controllerForm->setFromProject(controller->controllerStructure());
    }
}

void MainWindow::ensureLoopShapingPhase()
{
    if (loopShapingForm == nullptr){
        loopShapingForm = new LoopShapingForm(this);
        loopShapingForm->setLimits(m_settings.limits.maxMagnitude,
                                     m_settings.limits.maxTemplatePoints);
        loopShapingForm->applyDefaults(m_settings.defaults);
        loopShapingForm->setConservativeColumns(m_settings.algorithms.conservativeBoundaryColumns);
        loopShapingViewer = new LoopShapingViewer(this);
        connect(loopShapingForm, &StepPanel::accepted, this, &MainWindow::applyLoopShaping);

        //How many digits the numbers are shown at is chosen where they are
        //read, and kept for the next session like the theme and the canvas.
        connect(loopShapingViewer, &LoopShapingViewer::digitsChanged, this,
                [this](int digits) {
                    m_settings.interface.digits = digits;
                    if (m_settings.source.empty()) {
                        return;
                    }
                    try {
                        qftbx::writeSetting(m_settings.source, "interface.digits",
                                            std::to_string(digits));
                    } catch (const qftbx::Exception & failure) {
                        //A preference that could not be written is not worth
                        //stopping the user over.
                        (void) failure;
                    }
                });
        loopShapingCard = addPhaseCard(tr("Loop shaping"), "loopShapingCard", loopShapingForm,
                                       {{tr("Loop"), loopShapingViewer}});
        loopShapingForm->setFromProject(controller->loopShapingResult());
    }
}

void MainWindow::setFileChooser(FileChooser choose)
{
    m_chooseFile = std::move(choose);
}

QString MainWindow::chooseFile(bool forSaving, const QString & title)
{
    if (m_chooseFile != nullptr) {
        return m_chooseFile(forSaving);
    }

    return forSaving
            ? QFileDialog::getSaveFileName(this, title, "plant",
                                           tr("QFT Files (*.qft)"))
            : QFileDialog::getOpenFileName(this, title, "plant",
                                           tr("QFT Files (*.qft)"));
}

//Every enable rule, and the progress bar, DERIVED from what the project
//holds. This is the third of the four hand-written copies of the pipeline's
//dependency order to go: the facade owns the order, completed() answers it,
//and this asks rather than remembers.
//
//What it replaces: an enable condition inside each of the seven handlers, the
//same conditions again in the open handler, seven booleans and a counter kept
//by hand, and a stepBack() that walked the counter backwards.
//
//One rule is deliberately tighter than what it replaces. The Bode action used
//to be enabled by the frequencies alone, while the action itself refuses
//without a plant as well - so it could be pressed to no effect. It follows the
//action's own guard now, and nothing that worked stops working.
void MainWindow::refreshAvailability()
{
    if (controller == nullptr) {
        return;
    }

    const qftbx::StepSet done = controller->completed();

    //A step the project has lost takes its diagram with it. The panels stay
    //- they are filled from the project every time they are shown, so they
    //cannot show a step that is gone - but what a viewer holds are
    //OBSERVERS on what the project dropped, and those pointers dangle.
    //Destroying the widgets is what this used to do, and it is what made
    //cancelling a dialog undo a step the project still held.
    if (!done.has(qftbx::Step::Plant) && bodeViewer != nullptr) {
        bodeViewer->clear();
    }

    if (!done.has(qftbx::Step::Templates) && templateViewer != nullptr) {
        templateViewer->clear();
    }

    if (!done.has(qftbx::Step::Boundaries) && boundaryViewer != nullptr) {
        boundaryViewer->clear();
        boundaryUnionViewer->clear();
    }

    if (!done.has(qftbx::Step::LoopShaping) && loopShapingViewer != nullptr) {
        loopShapingViewer->clear();
    }

    //The specifications panel holds the project's frequency values, and a
    //new set destroys the ones it was given. It stays open, so it is told.
    if (specificationsForm != nullptr) {
        specificationsForm->setFrequencies(frequencyValues());
    }

    //And the templates panel holds the plant its grids were built for, so a
    //plant that is replaced or dropped is taken from it: the grids describe
    //nothing until the step is opened again, which is where they are built.
    //
    //Taken from it, NOT rebuilt here. Rebuilding them is
    //TemplatesForm::launch, which ends by proposing an epsilon - a sweep
    //of the whole plant family over the grids - and this runs on every
    //change the project reports, the one at the end of loading a file
    //included. Opening a project ran a sweep nobody had asked for.
    if (templatesForm != nullptr && templatesForm->shownPlant() != controller->plant()) {
        templatesForm->forgetPlant();
    }

    const bool plant          = done.has(qftbx::Step::Plant);
    const bool specifications = done.has(qftbx::Step::Specifications);
    const bool frequencies    = done.has(qftbx::Step::Frequencies);
    const bool templates      = done.has(qftbx::Step::Templates);
    const bool boundaries     = done.has(qftbx::Step::Boundaries);
    const bool structure      = done.has(qftbx::Step::Controller);

    ui->specificationsButton->setEnabled(frequencies);
    ui->templatesButton->setEnabled(plant && frequencies);
    ui->boundariesButton->setEnabled(templates && specifications);
    ui->loopButton->setEnabled(boundaries && structure);
    ui->bodeAction->setEnabled(plant && frequencies);

    ui->progressBar->setValue(static_cast<int>(done.count()));
}

void MainWindow::installContourRecomputer(){
    templateViewer->setContourRecomputer([this](std::vector<double> epsilon) {
        recomputeContour(std::move(epsilon));
    });
    templateViewer->setEpsilonProposer([this]() {
        return controller->proposeEpsilon();
    });
    templateViewer->setContourReporter([this]() {
        return controller->contourReports();
    });
}

void MainWindow::recomputeContour(std::vector<double> epsilon){
    //The viewer asked for a tighter contour: the computation, and the
    //reporting of its failure, belong here.
    try {
        //It walks the epsilon-hull over every cloud, which is work, and this
        //was the one computation in the window with no cursor at all: it
        //froze under a pointer that said nothing was happening.
        const WaitCursor waiting(this);

        controller->recomputeContour(std::move(epsilon));
    } catch (const qftbx::Exception & e) {
        QMessageBox::critical(this, tr("Template computation"), translated(e));
        return;
    }

    templateViewer->refreshContour(controller->contour(),
                                   controller->omega()->values(),
                                   controller->epsilon());
}

const std::vector<double> * MainWindow::frequencyValues() const{
    Omega * omega = controller->omega();

    if (omega == nullptr){
        return nullptr;
    }

    return omega->values();
}

void MainWindow::destroySession(){
    destroyPhases();

    controller.reset();
}

//The step buttons no longer run anything: they bring their phase to the
//front, filled with what the project holds. What used to follow the modal
//dialog is the apply* below, which the panel asks for when the user presses
//its button.
void MainWindow::on_plantButton_clicked()
{
    ensurePlantPhase();
    plantForm->clearAcceptance();
    showPhase(plantCard);
}

void MainWindow::applyPlant()
{
    //The panels only describe; publishing into the project is the window's
    //job, so a panel never needs to know the facade.
    std::unique_ptr<LtiSystem> described = plantForm->takePlant();
    //Publish only what was actually received. The payload is MOVED out of
    //the panel, so asking twice gives a null the second time - and
    //publishing a null wipes the step from the project, and everything
    //computed from it. The invariant: nothing moved-from goes into the
    //project.
    if (described == nullptr){
        return;
    }

    //The project drops the templates and everything after them when the
    //plant changes, and the window follows when the project says so:
    //nothing is decided here.
    controller->setPlant(std::move(described));
    m_templatesStale = true;

    drawBodeIfPossible();
}

void MainWindow::on_specificationsButton_clicked()
{

    //Both of these refuse a project with no design frequencies by throwing,
    //and an exception leaving a slot takes the application down. The button
    //is only enabled once the frequencies are in, so this is a broken
    //invariant rather than a user error - which is exactly the kind that
    //should be reported instead of aborting.
    try {
        ensureSpecificationsPhase();
    } catch (const qftbx::Exception & e) {
        QMessageBox::critical(this, tr("Specifications input"), translated(e));
        return;
    }

    specificationsForm->clearAcceptance();
    showPhase(specificationsCard);
}

void MainWindow::applySpecifications()
{
    //See applyPlant: nothing moved-from goes in, and an empty answer here
    //would wipe the specifications.
    std::optional<qftbx::SpecificationRecords> described =
            specificationsForm->takeSpecifications();

    if (!described.has_value()){
        return;
    }

    //The templates do not depend on the specifications; the boundaries do,
    //and the project drops them and says so.
    controller->setSpecifications(std::move(described));
}

void MainWindow::on_frequenciesButton_clicked()
{
    ensureFrequenciesPhase();
    frequenciesForm->clearAcceptance();
    showPhase(frequenciesCard);
}

void MainWindow::applyFrequencies()
{
    std::unique_ptr<Omega> described = frequenciesForm->takeOmega();
    //See applyPlant: nothing moved-from goes in.
    if (described == nullptr){
        return;
    }

    //See applyPlant: the project decides what a new set of frequencies
    //drops, and the window follows.
    controller->setOmega(std::move(described));
    m_templatesStale = true;

    drawBodeIfPossible();
}

void MainWindow::on_templatesButton_clicked()
{
    ensureTemplatesPhase();

    if (m_templatesStale) {
        relaunchTemplates();
    }
    templatesForm->clearAcceptance();
    showPhase(templatesCard);
}

void MainWindow::applyTemplates()
{
    //Read on this thread, while the form still says what the user asked
    //for: the worker gets values, not widgets.
    controller->setEpsilonMetric(templatesForm->epsilonMetric());
    controller->setWholeCloudStandsIn(templatesForm->wholeTemplateIfNoContour());
    controller->setAlphaShapeContour(templatesForm->alphaShapeContour());
    controller->setBorderSweep(templatesForm->borderSweep());

    const bool nichols = templatesForm->nicholsSelected();

    runInBackground(templatesCard, tr("Templates: sweeping..."), tr("Template computation"),
                    [this](std::function<void ()> done) {
                        return controller->startTemplates(templatesForm->takeEpsilon(),
                                                          templatesForm->grids(),
                                                          templatesForm->cudaSelected(),
                                                          std::move(done));
                    },
                    [this, nichols]() {
                        templateViewer->setData(controller->templates(),
                                                controller->contour(),
                                                controller->omega()->values(),
                                                controller->epsilon());
                        templateViewer->plotDiagram(nichols);
                    });
}

void MainWindow::on_boundariesButton_clicked()
{
    ensureBoundariesPhase();
    boundaryGridForm->clearAcceptance();
    showPhase(boundariesCard);
}

void MainWindow::applyBoundaries()
{
    runInBackground(boundariesCard, tr("Boundaries: computing..."), tr("Boundary computation"),
                    [this](std::function<void ()> done) {
                        return controller->startBoundaries(boundaryGridForm->phaseRangeValue(),
                                                           boundaryGridForm->phaseCountValue(),
                                                           boundaryGridForm->magnitudeRangeValue(),
                                                           boundaryGridForm->magnitudeCountValue(),
                                                           boundaryGridForm->infinityValue(),
                                                           boundaryGridForm->contourSelected(),
                                                           boundaryGridForm->cudaSelected(),
                                                           std::move(done));
                    },
                    [this]() {
                        boundaryViewer->setData(controller->boundaries(), controller->omega()->values());
                        boundaryViewer->showDiagram();

                        boundaryUnionViewer->setData(controller->unionBoundaries(),
                                                     controller->omega()->values());
                        boundaryUnionViewer->showDiagram();
                    });
}


void MainWindow::on_controllerButton_clicked()
{
    ensureControllerPhase();
    controllerForm->clearAcceptance();
    showPhase(controllerCard);
}

void MainWindow::applyController()
{
    std::unique_ptr<LtiSystem> described = controllerForm->takeControllerStructure();
    //See applyPlant: nothing moved-from goes in.
    if (described == nullptr){
        return;
    }

    //A different structure voids the design found for the old one; the
    //project drops it and the window follows.
    controller->setControllerStructure(std::move(described));
}

void MainWindow::on_loopButton_clicked()
{
    ensureLoopShapingPhase();

    loopShapingForm->clearAcceptance();
    showPhase(loopShapingCard);
}

void MainWindow::applyLoopShaping()
{
    //The reading of the boundary columns is a setting the form exposes for
    //this run; the core gets it the way it gets every setting.
    m_settings.algorithms.conservativeBoundaryColumns = loopShapingForm->conservativeColumns();
    controller->applySettings(m_settings);

    const bool linSpace = loopShapingForm->isLinSpace();

    runInBackground(loopShapingCard, tr("Loop: searching..."), tr("Loop Shaping"),
                    [this](std::function<void ()> done) {
                        return controller->startLoopShaping(loopShapingForm->epsilonValue(),
                                                            loopShapingForm->algorithmValue(),
                                                            loopShapingForm->range(),
                                                            loopShapingForm->pointCountValue(),
                                                            loopShapingForm->initialisationValue(),
                                                            std::move(done));
                    },
                    [this, linSpace]() {
                        loopShapingViewer->setData(controller->unionBoundaries(),
                                                   controller->omega()->values(),
                                                   controller->loopShapingResult(),
                                                   controller->plant(), linSpace);
                        loopShapingViewer->showDiagram();
                    });
}

void MainWindow::on_actionSave_triggered()
{
    if(saveFilePath.isEmpty()){
        on_actionSaveAs_triggered();
    } else {
        saveProject();
    }
}

void MainWindow::on_actionSaveAs_triggered()
{
    const QString fileName = chooseFile(true, tr("Save file"));


    if (!fileName.isEmpty()){

        if (fileName.right(4) != ".qft"){
            saveFilePath = fileName+".qft";
        } else {
            saveFilePath = fileName;
        }
        saveProject();
    }
}

void MainWindow::saveProject(){
    try {
        controller->save(saveFilePath.toStdString());
    } catch (const qftbx::Exception & e) {
        QMessageBox::critical(this, tr("Save file"), translated(e));
    }
}

void MainWindow::on_actionOpen_triggered()
{
    const QString fileName = chooseFile(false, tr("Open project"));

    if (!fileName.isEmpty()){

        qftbx::StepSet loaded;

        try {
            loaded = controller->load(fileName.toStdString());
        } catch (const qftbx::Exception & e) {
            QMessageBox::critical(this, tr("Open project"), translated(e));
            return;
        }

        //The previous session's widgets are freed, so the bar does not
        //accumulate steps across files.
        destroyPhases();

        //Save writes back to the file that was just opened.
        saveFilePath = fileName;

        //The widgets of the steps the file carried: the same ensure*() the
        //handlers use, so a step's widgets are built in one place. Which
        //steps are done is derived from the project, and the buttons and
        //the bar come from the one call at the end.
        //In the order of the pipeline, which is the order the cards then
        //appear in on the canvas.
        if (loaded.has(qftbx::Step::Plant)) {
            ensurePlantPhase();
        }
        if (loaded.has(qftbx::Step::Frequencies)) {
            ensureFrequenciesPhase();
        }
        if (loaded.has(qftbx::Step::Specifications)) {
            ensureSpecificationsPhase();
        }
        if (loaded.has(qftbx::Step::Templates)) {
            ensureTemplatesPhase();
        }
        if (loaded.has(qftbx::Step::Boundaries)) {
            ensureBoundariesPhase();
        }
        if (loaded.has(qftbx::Step::Controller)) {
            ensureControllerPhase();
        }
        if (loaded.has(qftbx::Step::LoopShaping)) {
            ensureLoopShapingPhase();
        }

        //A file that carries results has them on screen the moment it is
        //opened: the diagram is beside the data that produced it now, and
        //an empty plot next to a full form says the project lost something.
        showResults();

        //And every card comes up folded: the diagrams of a finished project
        //share the canvas, and what the user came to look at is the
        //diagrams. His own step button unfolds the form of a phase.
        for (PhaseCard * card : {plantCard, templatesCard, boundariesCard, loopShapingCard}) {
            if (card != nullptr) {
                card->showForm(false);
            }
        }
    }

}

//Everything a loaded project has to show, in one place and behind one
//report: this runs from the open handler, which is a slot, and an exception
//leaving a slot takes the application down. Drawing is not worth that - a
//project whose diagram cannot be built still has its numbers.
//One computation at a time, on the worker, with the card of its phase
//saying so and offering to give up on it. What used to happen here was that
//the whole window froze for as long as the search took - tens of minutes on
//a real problem - with an hourglass over it and no way out but killing the
//process.
void MainWindow::runInBackground(PhaseCard * card, const QString & what, const QString & title,
                                 const std::function<bool (std::function<void ()>)> & start,
                                 const std::function<void ()> & collected)
{
    if (m_computing != nullptr) {
        errorMessage(tr("Another phase is computing. Wait for it or cancel it."), title);
        return;
    }

    //The finished handler runs ON THE WORKER, so all it does is hop back
    //here: everything below touches widgets and the project.
    const auto whenDone = [this, card, title, collected]() {
        QMetaObject::invokeMethod(this, [this, card, title, collected]() {
            card->setBusy(false);
            m_computing = nullptr;

            //What the run means for the rest of the project, applied here
            //and not on the worker, and announced once.
            controller->collectComputation();

            if (!controller->lastComputationError().empty()) {
                errorMessage(QString::fromStdString(controller->lastComputationError()), title);
                return;
            }
            if (controller->lastComputationCancelled() || !controller->lastComputationProduced()) {
                return;
            }

            collected();
        }, Qt::QueuedConnection);
    };

    try {
        if (!start(whenDone)) {
            errorMessage(tr("A computation is already running."), title);
            return;
        }
    } catch (const qftbx::Exception & e) {
        //The prerequisites are checked on this thread, so a project that
        //cannot start says so here.
        errorMessage(translated(e), title);
        return;
    }

    m_computing = card;
    card->setBusy(true, what);
}

void MainWindow::showResults()
{
    try {
        drawResults();
    } catch (const qftbx::Exception & e) {
        QMessageBox::critical(this, tr("Open project"), translated(e));
    }
}

void MainWindow::drawResults()
{
    const qftbx::StepSet done = controller->completed();

    drawBodeIfPossible();

    if (done.has(qftbx::Step::Templates) && done.has(qftbx::Step::Frequencies)
            && templateViewer != nullptr) {
        templateViewer->setData(controller->templates(), controller->contour(),
                                controller->omega()->values(), controller->epsilon());
        templateViewer->plotDiagram(true);
    }

    if (done.has(qftbx::Step::Boundaries) && done.has(qftbx::Step::Frequencies)
            && boundaryViewer != nullptr) {
        boundaryViewer->setData(controller->boundaries(), controller->omega()->values());
        boundaryViewer->showDiagram();

        boundaryUnionViewer->setData(controller->unionBoundaries(), controller->omega()->values());
        boundaryUnionViewer->showDiagram();
    }

    if (done.has(qftbx::Step::LoopShaping) && done.has(qftbx::Step::Boundaries)
            && done.has(qftbx::Step::Frequencies) && loopShapingViewer != nullptr) {
        loopShapingViewer->setData(controller->unionBoundaries(), controller->omega()->values(),
                                   controller->loopShapingResult(), controller->plant(),
                                   loopShapingForm->isLinSpace());
        loopShapingViewer->showDiagram();
    }
}

void MainWindow::on_actionNew_triggered()
{
    destroySession();
    createSession();
}

//The menu entry was enabled and disabled with care but connected to
//nothing: the handler had been commented out since the initial upload and
//the action it was named after has since been renamed, so nothing wired it
//up. Reconnected here; the drawing itself needed fixing (see drawBode).
void MainWindow::on_bodeAction_triggered()
{
    if (!drawBodeIfPossible()){
        errorMessage(tr("To show the Bode diagram, first enter a valid plant and a set of design frequencies"), tr("QFT"));
        return;
    }

    showPhase(plantCard);
}

//The Bode diagram lives in the plant's dock, beside the plant it draws, and
//is redrawn whenever the plant or the frequencies change - it used to be a
//window of its own that had to be asked for and went stale in silence.
bool MainWindow::drawBodeIfPossible()
{
    const qftbx::StepSet done = controller->completed();

    if (!done.has(qftbx::Step::Plant) || !done.has(qftbx::Step::Frequencies)){
        return false;
    }

    ensurePlantPhase();

    //The drawing evaluates the plant, which is a model the user wrote and
    //can refuse to be evaluated. It runs by itself now, from wherever the
    //plant or the frequencies change, and a slot that lets an exception
    //out takes the application down with it.
    try {
        bodeViewer->drawBode(controller->plant(), controller->omega());
    } catch (const qftbx::Exception & e) {
        bodeViewer->clear();
        QMessageBox::critical(this, tr("Bode diagram"), translated(e));
    }

    return true;
}

void MainWindow::on_actionNicholsLoop_triggered()
{
    showLoopDiagrams(true, false);
}

void MainWindow::on_actionNyquistLoop_triggered()
{
    showLoopDiagrams(false, true);
}

void MainWindow::on_actionAllLoopDiagrams_triggered()
{
    showLoopDiagrams(true, true);
}

void MainWindow::showLoopDiagrams(bool nichols, bool nyquist){

    //Without boundaries and a controller structure there is no loop to
    //show.
    const qftbx::StepSet done = controller->completed();

    if (!done.has(qftbx::Step::Boundaries) || !done.has(qftbx::Step::Controller)){
        errorMessage(tr("To show the loop diagram, first compute the boundaries and enter the controller structure."), tr("QFT"));
        return;
    }

    BoundaryData * boundaries = controller->boundaries();

    //The same union read on the complex plane, for the Nyquist half of the
    //view: the viewer takes the curves it draws, and the conversion is
    //qftbx::toNyquist.
    qftbx::NyquistTraces nyquistTraces;
    nyquistTraces.reserve(boundaries->unionBoundaries().size());

    for (const qftbx::Trace & trace : boundaries->unionBoundaries()) {

        qftbx::NyquistTrace converted;
        converted.reserve(trace.size());

        for (const qftbx::NicholsPoint & point : trace) {
            converted.push_back(qftbx::toNyquist(point));
        }

        nyquistTraces.push_back(std::move(converted));
    }


    //Modal and parentless, so it is this scope's: on the stack. The Nyquist
    //boundaries and their buckets are held by value and die here too.
    LoopBoundariesViewer ver;

    ver.setData(boundaries, nyquistTraces, controller->omega()->values(), controller->plant(),
                 controller->controllerStructure(), nichols, nyquist);

    ver.showDiagram();

    ver.exec();
}

//The three view-again actions: each brings its phase to the front, drawn
//from what the project holds. A step nobody has computed has nothing to
//show and the action does nothing, which is what its guard says.
void MainWindow::on_actionTemplates_triggered()
{
    if (!controller->completed().has(qftbx::Step::Templates)){
        return;
    }

    if (!controller->templates().empty() && !controller->contour().empty()){
        ensureTemplatesPhase();
        templateViewer->setData(controller->templates(),
                                 controller->contour(),
                                 controller->omega()->values(),
                                 controller->epsilon());
        templateViewer->plotDiagram(true);

        showPhase(templatesCard);
    }
}

void MainWindow::on_actionBoundaries_triggered()
{
    if (!controller->completed().has(qftbx::Step::Boundaries)){
        return;
    }

    ensureBoundariesPhase();
    boundaryUnionViewer->setData(controller->unionBoundaries(), controller->omega()->values());
    boundaryUnionViewer->showDiagram();

    showPhase(boundariesCard);
}

void MainWindow::on_actionLoop_triggered()
{
    if (!controller->completed().has(qftbx::Step::LoopShaping)){
        return;
    }

    ensureLoopShapingPhase();
    loopShapingViewer->setData(controller->unionBoundaries(),controller->omega()->values(),
                              controller->loopShapingResult(), controller->plant(), loopShapingForm->isLinSpace());

    loopShapingViewer->showDiagram();

    showPhase(loopShapingCard);
}

} // namespace qftbx
