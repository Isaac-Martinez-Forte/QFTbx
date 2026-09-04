// Smoke tests over the Qt dialogs, run headless (QT_QPA_PLATFORM=offscreen).
//
// The dialogs are where the user's numbers become model objects, and the
// backend suite cannot reach them: they were the blind spot of the phase-9
// type changes (parameters by value, ranges as their own type, the
// optional-returning builders). These tests drive each dialog the way a
// user does - fill the fields by object name, press OK - and then check
// what the dialog handed back. The dialogs do not know the project: they
// build objects and the main window publishes them, so the assertions read
// the dialog's own answer.
//
// They are smoke tests: they answer "does the data path still work end to
// end", not "is every validation rule right".

#include <gtest/gtest.h>

#include <vector>

#include "src/core/point.h"

#include "src/core/range.h"

#include <complex>
#include <memory>

#include <QCheckBox>
#include <QDialog>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QStackedWidget>
#include <QString>
#include <QStringList>

#include "src/core/project_controller.h"
#include "src/gui/plant_dialog.h"
#include "src/gui/controller_dialog.h"
#include "src/gui/frequencies_dialog.h"
#include "src/gui/specifications_dialog.h"
#include "src/gui/template_viewer.h"
#include "src/gui/bode_viewer.h"
#include "src/core/math/sequence_vectors.h"
#include "src/core/system/polynomial_form.h"
#include "src/gui/uncertainty_dialog.h"
#include "src/gui/boundary_viewer.h"
#include "src/gui/boundary_union_viewer.h"
#include "src/gui/loop_shaping_viewer.h"
#include "src/gui/loop_boundaries_viewer.h"
#include "src/core/boundaries/boundary_data.h"
#include "src/core/loopshaping/loop_shaping_result.h"
#include "src/gui/boundary_grid_dialog.h"
#include "src/gui/templates_dialog.h"
#include "src/gui/loop_shaping_dialog.h"
#include "src/gui/main_window.h"
#include "src/gui/error_message.h"
#include "src/core/exception.h"

namespace {

//Every dialog reports invalid input through tools::errorMessage, which
//opens a MODAL dialog: an automated run would block on it forever. The
//fixture redirects it and keeps what was reported, so a rejection can be
//asserted rather than waited on.
class GuiSmoke : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_previous = tools::setErrorReporter(
            [this](const QString & message, const QString & title) {
                m_reported.push_back(title + ": " + message);
            });
    }

    void TearDown() override
    {
        tools::setErrorReporter(m_previous);
    }

    QStringList m_reported;

private:
    tools::ErrorReporter m_previous;
};

//The dialogs keep their widgets private; uic gives every one of them an
//object name, which is the handle a user's click resolves to as well.
template <typename Widget>
Widget * child(QWidget * dialog, const char * name)
{
    Widget * found = dialog->findChild<Widget *>(QString::fromUtf8(name));
    EXPECT_NE(found, nullptr) << "no widget named " << name;
    return found;
}

void type(QWidget * dialog, const char * name, const QString & text)
{
    QLineEdit * edit = child<QLineEdit>(dialog, name);
    if (edit != nullptr) {
        edit->setText(text);
    }
}

void check(QWidget * dialog, const char * name)
{
    QRadioButton * radio = dialog->findChild<QRadioButton *>(QString::fromUtf8(name));
    if (radio != nullptr) {
        radio->setChecked(true);
        return;
    }

    QCheckBox * box = child<QCheckBox>(dialog, name);
    if (box != nullptr) {
        box->setChecked(true);
    }
}

void press(QWidget * dialog, const char * name)
{
    QPushButton * button = child<QPushButton>(dialog, name);
    if (button != nullptr) {
        button->click();
    }
}

//A minimal one-frequency boundary set: one named boundary of three points
//over the default Nichols window. Enough to drive the drawing code, which
//is what the viewer tests are after.
BoundaryData oneBoundary()
{
    //Twenty-two lines of heap allocation and a takeOwnership() call became
    //this. The curve is one named boundary of three points over the default
    //Nichols window - enough to drive the drawing code, which is what the
    //viewer tests are after.
    const qftbx::Trace curve{qftbx::NicholsPoint(-270.0, 10.0), qftbx::NicholsPoint(-180.0, 4.0),
                             qftbx::NicholsPoint(-90.0, 10.0)};

    return BoundaryData({{{"Stability", {curve}}}},
                        {false}, {true}, 361, qftbx::Range(-360.0, 0.0),
                        {curve}, {qftbx::TraceSet(361)},
                        121, qftbx::Range(-60.0, 60.0));
}

// ---------------------------------------------------------------------------

TEST_F(GuiSmoke, PlantDialogBuildsAZeroPoleGainPlant)
{
    PlantDialog dialog;

    type(&dialog, "nameEdit", "smoke");
    check(&dialog, "zpkRadio");
    type(&dialog, "zpkNumerator", "2");
    type(&dialog, "zpkDenominator", "5 30");
    type(&dialog, "zpkGain", "3");
    type(&dialog, "zpkDelay", "0");

    press(&dialog, "okButton");

    ASSERT_TRUE(dialog.wasAccepted()) << "the dialog rejected valid data";

    //Ownership comes with it: the window would hand it to the project.
    std::unique_ptr<LtiSystem> plant(dialog.takePlant());
    ASSERT_NE(plant, nullptr);
    EXPECT_EQ(plant->type(), LtiSystem::SystemType::ZeroPoleGain);
    EXPECT_EQ(plant->name(), "smoke");

    //The numbers must have travelled into parameter VALUES.
    ASSERT_EQ(plant->numerator().size(), 1u);
    EXPECT_DOUBLE_EQ(plant->numerator()[0].nominal(), 2.0);
    ASSERT_EQ(plant->denominator().size(), 2u);
    EXPECT_DOUBLE_EQ(plant->denominator()[0].nominal(), 5.0);
    EXPECT_DOUBLE_EQ(plant->denominator()[1].nominal(), 30.0);
    EXPECT_DOUBLE_EQ(plant->gain().nominal(), 3.0);
    EXPECT_DOUBLE_EQ(plant->delay().nominal(), 0.0);
}

TEST_F(GuiSmoke, PlantDialogRejectsAnInvalidExpression)
{
    //A malformed coefficient must be reported, not crash the application
    //(muParserX throws and the dialog used to let it through).
    PlantDialog dialog;

    type(&dialog, "nameEdit", "broken");
    check(&dialog, "zpkRadio");
    type(&dialog, "zpkNumerator", "2");
    type(&dialog, "zpkDenominator", "5");
    type(&dialog, "zpkGain", "3*/");
    type(&dialog, "zpkDelay", "0");

    press(&dialog, "okButton");

    EXPECT_FALSE(dialog.wasAccepted());
    EXPECT_EQ(dialog.takePlant(), nullptr);
    EXPECT_FALSE(m_reported.empty()) << "the rejection must be reported";
}

TEST_F(GuiSmoke, PlantDialogRejectsAnInvalidCoefficient)
{
    //The gain path reports a malformed expression, but buildParameters
    //catches the parser error per COEFFICIENT and substitutes 0, so a
    //numerator of "1*/" became the polynomial 0 with nothing said.
    PlantDialog dialog;

    type(&dialog, "nameEdit", "broken-numerator");
    check(&dialog, "zpkRadio");
    type(&dialog, "zpkNumerator", "1*/");
    type(&dialog, "zpkDenominator", "5 30");
    type(&dialog, "zpkGain", "3");
    type(&dialog, "zpkDelay", "0");

    press(&dialog, "okButton");

    EXPECT_FALSE(dialog.wasAccepted())
        << "a malformed coefficient was accepted";
    EXPECT_FALSE(m_reported.empty()) << "the rejection must be reported";

    dialog.takePlant();
}

TEST_F(GuiSmoke, PlantDialogRejectsAReservedParameterName)
{
    //"k" is the obvious name for a gain, and muParserX owns it as the unit
    //multiplier 1e3. Naming a parameter after anything the parser defines
    //used to pass the dialog - only FUNCTION names were checked - and fail
    //much later as a generic evaluation error, or worse be read as 1e3.
    PlantDialog dialog;

    type(&dialog, "nameEdit", "reserved");
    check(&dialog, "zpkRadio");
    type(&dialog, "zpkNumerator", "1");
    type(&dialog, "zpkDenominator", "k");
    type(&dialog, "zpkGain", "1");
    type(&dialog, "zpkDelay", "0");

    press(&dialog, "okButton");

    EXPECT_FALSE(dialog.wasAccepted()) << "a reserved parameter name was accepted";
    ASSERT_FALSE(m_reported.empty()) << "the rejection must be reported";
    EXPECT_TRUE(m_reported.join(QChar(' ')).contains("k"))
        << "the message must name the offending identifier: "
        << m_reported.join(QChar(' ')).toStdString();

    dialog.takePlant();
}

TEST_F(GuiSmoke, FrequenciesDialogBuildsTheDesignFrequencies)
{
    FrequenciesDialog dialog;

    //modeStack is the generation-mode combo (manual is entry 0); the pages
    //of values live in the selecVector stack, which follows it.
    QComboBox * mode = child<QComboBox>(&dialog, "modeStack");
    ASSERT_NE(mode, nullptr);
    mode->setCurrentIndex(0);
    type(&dialog, "manualValues", "0.1 1 10 100");

    press(&dialog, "okButton");

    ASSERT_TRUE(dialog.wasAccepted()) << "the dialog rejected valid data";

    std::unique_ptr<Omega> omega(dialog.takeOmega());
    ASSERT_NE(omega, nullptr);
    ASSERT_NE(omega->values(), nullptr);
    const std::vector<double> expected{0.1, 1.0, 10.0, 100.0};
    EXPECT_EQ(*omega->values(), expected);
    EXPECT_EQ(omega->pointCount(), 4);
}

TEST_F(GuiSmoke, SpecificationsDialogRefusesToLeaveATabItCannotRead)
{
    //Switching away used to happen anyway and the return value of the read
    //was discarded, so the whole specification was lost - not just the bad
    //field - and coming back showed a blank tab.
    const std::vector<double> frequencies{0.1, 1.0, 10.0};
    SpecificationsDialog dialog(&frequencies);

    check(&dialog, "stabilityRadio");
    dialog.findChild<QRadioButton *>("stabilityRadio")->click();

    //A valid constant stability specification.
    check(&dialog, "constantRadio");
    check(&dialog, "linearRadio");
    type(&dialog, "magnitudeEdit", "1.2");

    //Now break the band and try to leave.
    type(&dialog, "startFrequencyEdit", "not a number");
    m_reported.clear();

    dialog.findChild<QRadioButton *>("noiseRadio")->click();

    EXPECT_FALSE(m_reported.empty()) << "the refusal must be reported";
    EXPECT_TRUE(dialog.findChild<QRadioButton *>("stabilityRadio")->isChecked())
        << "the tab that could not be read must stay selected";
    EXPECT_FALSE(dialog.findChild<QRadioButton *>("noiseRadio")->isChecked());

    //The user's text is still on screen, where it can be corrected.
    EXPECT_EQ(dialog.findChild<QLineEdit *>("magnitudeEdit")->text(),
              "1.2");
}

TEST_F(GuiSmoke, FrequenciesDialogRefusesAnEmptySetInsteadOfDying)
{
    //Pressing OK on a freshly opened dialog ABORTED the application: the
    //Omega constructor refuses an empty frequency set by throwing, and that
    //exception escaped this slot into Qt's event loop.
    FrequenciesDialog dialog;

    child<QComboBox>(&dialog, "modeStack")->setCurrentIndex(0);

    press(&dialog, "okButton");

    EXPECT_FALSE(dialog.wasAccepted());
    EXPECT_FALSE(m_reported.empty()) << "the rejection must be reported";
    EXPECT_EQ(dialog.takeOmega(), nullptr);
}

TEST_F(GuiSmoke, FrequenciesDialogRefusesNonPositiveFrequencies)
{
    //A design frequency is evaluated on the imaginary axis at s = jw and
    //plotted on a logarithmic axis: zero and negative values are not a
    //frequency set, and the manual mode used to accept them.
    FrequenciesDialog dialog;

    child<QComboBox>(&dialog, "modeStack")->setCurrentIndex(0);
    type(&dialog, "manualValues", "0.1 0 10");

    press(&dialog, "okButton");

    EXPECT_FALSE(dialog.wasAccepted());
    ASSERT_FALSE(m_reported.empty()) << "the rejection must be reported";
    EXPECT_TRUE(m_reported.join(QChar(' ')).contains("positive"))
        << m_reported.join(QChar(' ')).toStdString();
}

TEST_F(GuiSmoke, FrequenciesDialogRefusesAnEmptyPointCount)
{
    //An empty count reads as zero, linspace answers an empty set, and the
    //same throw followed.
    FrequenciesDialog dialog;

    child<QComboBox>(&dialog, "modeStack")->setCurrentIndex(2);
    type(&dialog, "linStart", "1");
    type(&dialog, "linEnd", "10");
    //linCount deliberately left empty

    press(&dialog, "okButton");

    EXPECT_FALSE(dialog.wasAccepted());
    EXPECT_FALSE(m_reported.empty()) << "the rejection must be reported";
}

//A QObject whose event handling throws, to reach the net underneath.
class ThrowingObject : public QObject
{
public:
    bool event(QEvent *) override
    {
        throw qftbx::InvalidInput("the backend refused something");
    }
};

TEST_F(GuiSmoke, TheApplicationReportsABackendErrorInsteadOfDyingOfIt)
{
    //Each case belongs guarded where it happens; this is the net underneath,
    //so that the next one to be missed is a message and not a crash. The
    //suite runs under the same Application as the program.
    ThrowingObject victim;
    QEvent event(QEvent::User);

    const bool handled = qApp->notify(&victim, &event);

    EXPECT_TRUE(handled);
    ASSERT_FALSE(m_reported.empty()) << "the escaped error must be reported";
    EXPECT_TRUE(m_reported.join(QChar(' ')).contains("refused something"))
        << m_reported.join(QChar(' ')).toStdString();
}

TEST_F(GuiSmoke, ControllerDialogBuildsTheControllerStructure)
{
    ControllerDialog dialog;

    check(&dialog, "zpkRadio");
    type(&dialog, "numeratorEdit", "1");
    type(&dialog, "denominatorEdit", "100");
    //The controller's gain is a SEARCH RANGE, not a value.
    type(&dialog, "gainStart", "1");
    type(&dialog, "gainEnd", "1000");

    press(&dialog, "okButton");

    ASSERT_TRUE(dialog.wasAccepted()) << "the dialog rejected valid data";

    std::unique_ptr<LtiSystem> structure(dialog.takeControllerStructure());
    ASSERT_NE(structure, nullptr);
    ASSERT_EQ(structure->numerator().size(), 1u);
    EXPECT_DOUBLE_EQ(structure->numerator()[0].nominal(), 1.0);
    ASSERT_EQ(structure->denominator().size(), 1u);
    EXPECT_DOUBLE_EQ(structure->denominator()[0].nominal(), 100.0);
    EXPECT_TRUE(structure->gain().isUncertain());
    EXPECT_DOUBLE_EQ(structure->gain().range().min, 1.0);
    EXPECT_DOUBLE_EQ(structure->gain().range().max, 1000.0);
}

TEST_F(GuiSmoke, ControllerDialogRejectsAnInvalidNumerator)
{
    //readTables() keeps only the LAST of its three parse results, so a
    //malformed numerator or denominator was overwritten by a gain range that
    //parsed. The plant dialog rejects the same input.
    ControllerDialog dialog;

    check(&dialog, "zpkRadio");
    type(&dialog, "numeratorEdit", "1*/");
    type(&dialog, "denominatorEdit", "100");
    type(&dialog, "gainStart", "1");
    type(&dialog, "gainEnd", "1000");

    press(&dialog, "okButton");

    EXPECT_FALSE(dialog.wasAccepted())
        << "a malformed numerator was accepted";
    EXPECT_EQ(dialog.takeControllerStructure(), nullptr);
    EXPECT_FALSE(m_reported.empty()) << "the rejection must be reported";
}

TEST_F(GuiSmoke, SpecificationsDialogNeedsTheFrequenciesFirst)
{
    //The main window gates the step order, but the dialog used to reach
    //first()/last() on a null frequency vector and take the application
    //down; it says so now.
    EXPECT_THROW(SpecificationsDialog dialog(nullptr), qftbx::InvalidInput);

    const std::vector<double> empty;
    EXPECT_THROW(SpecificationsDialog dialog(&empty), qftbx::InvalidInput);
}

TEST_F(GuiSmoke, SpecificationsDialogStoresAConstantStabilitySpecification)
{
    //Real step order: the frequencies come first, and the window hands them
    //to the dialog.
    const std::vector<double> frequencies{0.1, 1.0, 10.0, 100.0};

    SpecificationsDialog dialog(&frequencies);

    //Stability with a constant magnitude: the simplest complete slot.
    check(&dialog, "stabilityRadio");
    check(&dialog, "constantRadio");
    check(&dialog, "linearRadio");
    type(&dialog, "magnitudeEdit", "1.2");

    press(&dialog, "okButton");

    const std::optional<qftbx::SpecificationRecords> records = dialog.takeSpecifications();

    if (!records.has_value()) {
        //The dialog declined the combination; the smoke value here is that
        //it said so instead of crashing or storing a half-built record.
        EXPECT_FALSE(dialog.wasAccepted());
        return;
    }

    for (const qftbx::SpecificationRecord & record : *records) {
        if (record.used && record.constant) {
            EXPECT_GT(record.height, 0.0) << "a used constant specification "
                                             "must carry a positive magnitude";
        }
    }
}

TEST_F(GuiSmoke, TemplateViewerAsksItsHandlerToRecomputeTheContour)
{
    //The viewer runs no computation of its own: the recompute button calls
    //the handler the window installed, handing it the epsilon it read from
    //the fields. A plain callback, not a Qt signal.
    TemplateViewer viewer;

    const qftbx::CloudSet contour{{{1.0, 0.0}, {0.0, 1.0}}};
    const qftbx::CloudSet templates{{{2.0, 0.0}, {0.0, 2.0}}};
    std::vector<double> omega{1.0};
    std::vector<double> epsilon{0.05};

    //The viewer takes its own copy of the clouds; the frequency and epsilon
    //vectors are still the project's.
    viewer.setData(templates, contour, &omega, &epsilon);
    viewer.plotDiagram(true);

    bool called = false;
    std::vector<double> asked;
    viewer.setContourRecomputer([&](std::vector<double> requested) {
        called = true;
        asked = std::move(requested);
    });

    press(&viewer, "recomputeButton");

    ASSERT_TRUE(called) << "the recompute button did not reach its handler";
    ASSERT_EQ(asked.size(), 1);
    EXPECT_DOUBLE_EQ(asked[0], 0.05);

    //Each frequency row is colour coded to its curve. The English rename
    //(a9621ff) renamed a local QColor over the CSS property name inside the
    //literal, so the stylesheet read "colorsCreated : #rrggbb" and Qt
    //dropped it with an "Unknown property" warning on stderr.
    QCheckBox * row = child<QCheckBox>(&viewer, "check");
    ASSERT_NE(row, nullptr);
    EXPECT_TRUE(row->styleSheet().startsWith("color"))
        << "the frequency row lost its colour: "
        << row->styleSheet().toStdString();
    EXPECT_FALSE(row->styleSheet().contains("colorsCreated"));
}

TEST_F(GuiSmoke, TemplateViewerWithNothingPlottedIgnoresTheRecomputeButton)
{
    //With no plot the epsilon controls do not exist yet; the button used to
    //walk pointers that had never been assigned.
    TemplateViewer viewer;

    press(&viewer, "recomputeButton");
}

TEST_F(GuiSmoke, BodeViewerDrawsBothAxesOfTheDiagram)
{
    //The menu entry had been dead since the initial upload, so nothing had
    //exercised this path. It draws two plots, magnitude and phase, over the
    //design frequency span.
    BodeViewer viewer;

    //A first-order plant, 1/(s+1): -3 dB and -45 degrees at 1 rad/s.
    std::vector<Parameter> numerator{Parameter(1.0)};
    std::vector<Parameter> denominator{Parameter(1.0), Parameter(1.0)};
    PolynomialForm plant("bode", numerator, denominator,
                         Parameter(1.0), Parameter(0.0));

    //Logarithmic span: start() and end() are EXPONENTS here, 0.01 to 100.
    Omega omega(-2.0, 2.0, 100, tools::logspace(-2.0, 2.0, 100), Omega::LogSpace);

    viewer.drawBode(&plant, &omega);

    QCustomPlot * magnitude = child<QCustomPlot>(&viewer, "magnitudePlot");
    QCustomPlot * phase = child<QCustomPlot>(&viewer, "phasePlot");
    ASSERT_NE(magnitude, nullptr);
    ASSERT_NE(phase, nullptr);

    EXPECT_EQ(magnitude->plottableCount(), 1) << "the magnitude curve is missing";
    EXPECT_EQ(phase->plottableCount(), 1) << "the phase curve is missing";

    //The sweep must start at the design start, not at a hardcoded -1: a
    //logarithmic axis cannot render a range that begins at or below zero.
    EXPECT_GT(magnitude->xAxis->range().lower, 0.0);
    EXPECT_NEAR(magnitude->xAxis->range().lower, 0.01, 1e-9);
    EXPECT_NEAR(magnitude->xAxis->range().upper, 100.0, 1e-6);

    //Phase in DEGREES: a first-order lag spans (0, -90], never radians.
    EXPECT_LT(phase->yAxis->range().lower, -80.0);
    EXPECT_GT(phase->yAxis->range().lower, -90.5);

    //Redrawing must replace the curves, not pile new ones on.
    viewer.drawBode(&plant, &omega);
    EXPECT_EQ(magnitude->plottableCount(), 1) << "a redraw piled up curves";
}

TEST_F(GuiSmoke, BoundaryGridDialogBuildsTheNicholsGrid)
{
    BoundaryGridDialog dialog;

    //The DEFAULT window must be the full [-360, 0]: loop shaping refuses a
    //narrower one, and the reader of the phase buckets is scaled by it.
    QDialogButtonBox * buttons = child<QDialogButtonBox>(&dialog, "buttonBox");
    ASSERT_NE(buttons, nullptr);

    type(&dialog, "phasePoints", "361");
    type(&dialog, "magnitudePoints", "121");

    buttons->button(QDialogButtonBox::Ok)->click();

    ASSERT_TRUE(dialog.wasAccepted()) << "the dialog rejected its own defaults";

    EXPECT_DOUBLE_EQ(dialog.phaseRangeValue().min, -360.0);
    EXPECT_DOUBLE_EQ(dialog.phaseRangeValue().max, 0.0);
    EXPECT_EQ(dialog.phaseCountValue(), 361);
    EXPECT_EQ(dialog.magnitudeCountValue(), 121);
    EXPECT_LT(dialog.magnitudeRangeValue().min, dialog.magnitudeRangeValue().max);

    //A window narrower than 360 degrees would be refused later by
    //LoopShaping::run; the default one must not be.
    EXPECT_DOUBLE_EQ(dialog.phaseRangeValue().max - dialog.phaseRangeValue().min, 360.0);
}

TEST_F(GuiSmoke, BoundaryGridDialogRejectsAnInvertedRange)
{
    BoundaryGridDialog dialog;

    type(&dialog, "phaseStart", "0");
    type(&dialog, "phaseEnd", "-360");

    child<QDialogButtonBox>(&dialog, "buttonBox")->button(QDialogButtonBox::Ok)->click();

    EXPECT_FALSE(dialog.wasAccepted()) << "an inverted phase range was accepted";
}

TEST_F(GuiSmoke, TemplatesDialogBuildsOneEpsilonPerFrequency)
{
    TemplatesDialog dialog;

    std::vector<Parameter> numerator{Parameter(1.0)};
    std::vector<Parameter> denominator{
        Parameter("a", qftbx::Range(1.0, 5.0), 5.0)};
    PolynomialForm plant("templates", numerator, denominator,
                         Parameter(1.0), Parameter(0.0));

    dialog.launch(&plant, 4);

    type(&dialog, "epsilonEdit", "0.05");

    //The general section demands a spacing and a point count for the
    //parameter grids.
    check(&dialog, "linspaceRadio");
    type(&dialog, "globalPointCount", "3");
    check(&dialog, "allVariablesRadio");
    check(&dialog, "nicholsRadio");

    press(&dialog, "okButton");

    ASSERT_TRUE(dialog.wasAccepted()) << "the dialog rejected valid data";

    //One epsilon per design frequency: the template computation indexes it
    //by frequency.
    const std::vector<double> epsilon = dialog.takeEpsilon();
    EXPECT_EQ(epsilon.size(), 4);
    for (double value : epsilon) {
        EXPECT_DOUBLE_EQ(value, 0.05);
    }

    //A grid per uncertain parameter, and none for the constants. By value now:
    //nothing to free, and nothing to be null.
    const qftbx::ParameterGrids grids = dialog.grids();
    EXPECT_EQ(grids.count("a"), 1u)
        << "the uncertain parameter got no grid";
}

TEST_F(GuiSmoke, LoopShapingDialogCarriesTheChosenAlgorithm)
{
    LoopShapingDialog dialog;

    type(&dialog, "epsilonEdit", "0.01");
    type(&dialog, "startEdit", "0.1");
    type(&dialog, "endEdit", "100");
    type(&dialog, "pointCountEdit", "200");
    check(&dialog, "mrRadio");

    press(&dialog, "okButton");

    ASSERT_TRUE(dialog.wasAccepted()) << "the dialog rejected valid data";

    EXPECT_EQ(dialog.algorithmValue(), tools::mr);
    EXPECT_DOUBLE_EQ(dialog.epsilonValue(), 0.01);
    EXPECT_DOUBLE_EQ(dialog.range().min, 0.1);
    EXPECT_DOUBLE_EQ(dialog.range().max, 100.0);
    EXPECT_DOUBLE_EQ(dialog.pointCountValue(), 200.0);
}

TEST_F(GuiSmoke, UncertaintyDialogBuildsAnUncertainParameter)
{
    //This is what turns a named coefficient into an uncertain Parameter, so
    //it feeds the uncertainty of every plant and controller. It is driven
    //the way the plant dialog drives it: the three parallel tables, one row
    //per uncertain name.
    UncertaintyDialog dialog;

    const CoefficientTable valueTable{{"1"}, {"a"}};
    const CoefficientTable expressionTable{{"1"}, {"a"}};
    const UncertainTable uncertainTable{{false}, {true}};

    ASSERT_TRUE(dialog.launch(valueTable, expressionTable, uncertainTable, false));

    //One uncertain name, so one generated row: [inicio, fin] with nominal.
    type(&dialog, "inicio", "1");
    type(&dialog, "fin", "5");
    type(&dialog, "nominal", "3");

    type(&dialog, "gainStart", "2");
    type(&dialog, "gainEnd", "8");
    type(&dialog, "delayStart", "0");
    type(&dialog, "delayEnd", "0");

    press(&dialog, "okButton");

    ASSERT_TRUE(dialog.wasAccepted()) << "the dialog rejected valid data";

    //The constant numerator coefficient must travel as a constant, and the
    //named denominator one as uncertain with its range and nominal.
    ASSERT_EQ(dialog.numerator().size(), 1u);
    EXPECT_FALSE(dialog.numerator()[0].isUncertain());

    ASSERT_EQ(dialog.denominator().size(), 1u);
    Parameter & uncertain = dialog.denominator()[0];
    EXPECT_TRUE(uncertain.isUncertain());
    EXPECT_EQ(uncertain.name(), "a");
    EXPECT_DOUBLE_EQ(uncertain.rawRange().min, 1.0);
    EXPECT_DOUBLE_EQ(uncertain.rawRange().max, 5.0);
    EXPECT_DOUBLE_EQ(uncertain.rawNominal(), 3.0);

    //Gain and delay are search RANGES here, not values.
    EXPECT_DOUBLE_EQ(dialog.gain().min, 2.0);
    EXPECT_DOUBLE_EQ(dialog.gain().max, 8.0);
    EXPECT_DOUBLE_EQ(dialog.delay().min, 0.0);
    EXPECT_DOUBLE_EQ(dialog.delay().max, 0.0);
}

TEST_F(GuiSmoke, UncertaintyDialogRejectsAnEmptyRange)
{
    //An empty range used to be read as a null sentinel; it must be reported
    //and refused, not turned into a parameter.
    UncertaintyDialog dialog;

    const CoefficientTable valueTable{{"1"}, {"a"}};
    const CoefficientTable expressionTable{{"1"}, {"a"}};
    const UncertainTable uncertainTable{{false}, {true}};

    ASSERT_TRUE(dialog.launch(valueTable, expressionTable, uncertainTable, false));

    //The row is left blank on purpose.
    type(&dialog, "gainStart", "2");
    type(&dialog, "gainEnd", "8");
    type(&dialog, "delayStart", "0");
    type(&dialog, "delayEnd", "0");

    press(&dialog, "okButton");

    EXPECT_FALSE(dialog.wasAccepted()) << "a blank range was accepted";
}

TEST_F(GuiSmoke, BoundaryViewerDrawsTheBoundariesItIsGiven)
{
    //Draw-only viewer: the value here is that the drawing code runs over a
    //real boundary set and leaves curves behind, and that a redraw does not
    //pile them up (the bug the Bode and template viewers both had).
    BoundaryViewer viewer;

    const BoundaryData boundaries = oneBoundary();
    std::vector<double> omega{1.0};

    viewer.setData(&boundaries, &omega);
    viewer.showDiagram();

    QCustomPlot * plot = child<QCustomPlot>(&viewer, "plot");
    ASSERT_NE(plot, nullptr);
    const int drawn = plot->plottableCount();
    EXPECT_GT(drawn, 0) << "nothing was drawn";

    viewer.showDiagram();
    EXPECT_EQ(plot->plottableCount(), drawn) << "a redraw piled up curves";
}

TEST_F(GuiSmoke, BoundaryUnionViewerDrawsTheUnion)
{
    BoundaryUnionViewer viewer;

    const qftbx::UnionTraces traces{{qftbx::NicholsPoint(-270.0, 10.0), qftbx::NicholsPoint(-180.0, 4.0),
                                     qftbx::NicholsPoint(-90.0, 10.0)}};
    std::vector<double> omega{1.0};

    viewer.setData(traces, &omega);
    viewer.showDiagram();

    QCustomPlot * plot = child<QCustomPlot>(&viewer, "plot");
    ASSERT_NE(plot, nullptr);
    const int drawn = plot->plottableCount();
    EXPECT_GT(drawn, 0) << "nothing was drawn";

    viewer.showDiagram();
    EXPECT_EQ(plot->plottableCount(), drawn) << "a redraw piled up curves";
}

TEST_F(GuiSmoke, LoopShapingViewerDrawsTheShapedLoop)
{
    LoopShapingViewer viewer;

    //1/(s+1) as the plant and a unit gain as the computed controller.
    std::vector<Parameter> numerator{Parameter(1.0)};
    std::vector<Parameter> denominator{Parameter(1.0), Parameter(1.0)};
    PolynomialForm plant("loop", numerator, denominator,
                         Parameter(1.0), Parameter(0.0));

    std::vector<Parameter> one{Parameter(1.0)};
    LoopShapingResult result(std::make_unique<PolynomialForm>("k", one, one,
                                                              Parameter(1.0), Parameter(0.0)),
                             qftbx::Range(0.1, 100.0), 50);
    const qftbx::UnionTraces traces{{qftbx::NicholsPoint(-270.0, 10.0), qftbx::NicholsPoint(-180.0, 4.0),
                                     qftbx::NicholsPoint(-90.0, 10.0)}};
    std::vector<double> omega{1.0, 10.0};

    viewer.setData(traces, &omega, &result, &plant, false);
    viewer.showDiagram();

    QCustomPlot * plot = child<QCustomPlot>(&viewer, "plot");
    ASSERT_NE(plot, nullptr);
    EXPECT_GT(plot->plottableCount(), 0) << "nothing was drawn";
}

TEST_F(GuiSmoke, LoopBoundariesViewerDrawsBothDiagrams)
{
    LoopBoundariesViewer viewer;

    std::vector<Parameter> numerator{Parameter(1.0)};
    std::vector<Parameter> denominator{Parameter(1.0), Parameter(1.0)};
    PolynomialForm plant("loop", numerator, denominator,
                         Parameter(1.0), Parameter(0.0));
    std::vector<Parameter> one{Parameter(1.0)};
    PolynomialForm controller("k", one, one,
                              Parameter(1.0), Parameter(0.0));

    const BoundaryData nichols = oneBoundary();
    std::vector<double> omega{1.0};

    //The Nyquist half is now the curves themselves, converted from the same
    //union: the viewer no longer takes a BoundaryData built to look like
    //something it is not.
    qftbx::NyquistTraces nyquist;
    for (const qftbx::Trace & trace : nichols.unionBoundaries()) {
        qftbx::NyquistTrace converted;
        for (const qftbx::NicholsPoint & point : trace) {
            converted.push_back(qftbx::toNyquist(point));
        }
        nyquist.push_back(std::move(converted));
    }

    viewer.setData(&nichols, nyquist, &omega, &plant, &controller, true, false);
    viewer.showDiagram();

    QCustomPlot * plot = child<QCustomPlot>(&viewer, "plot");
    ASSERT_NE(plot, nullptr);
    EXPECT_GT(plot->plottableCount(), 0) << "nothing was drawn";
}

TEST_F(GuiSmoke, TheMainWindowBuildsItsWholeWidgetTree)
{
    //Every dialog, viewer and menu is constructed here: a broken .ui
    //reference or a null child shows up as a crash on construction.
    MainWindow window;
    EXPECT_FALSE(window.windowTitle().isEmpty());
}

// --- the window driven, not just built -------------------------------------
//
// Until now this file could construct MainWindow and nothing more: every step
// handler calls dialog->exec(), which is modal, so a test that pressed a step
// button hung there for ever. MainWindow::setDialogRunner is the seam that
// changes it - the test becomes the user, filling the dialog's fields by name
// and pressing its OK button, exactly as the dialog tests above already do.
//
// This is the net the rest of the window work needs: the seven bool flags,
// the repeated teardown and the invalidation ladder are about to be replaced,
// and until now there was nothing watching.

TEST_F(GuiSmoke, PressingThePlantStepPublishesAPlantAndOpensTheNextSteps)
{
    MainWindow window;

    //The runner is the user. It receives the very dialog the handler built.
    window.setDialogRunner([](QDialog * dialog) {
        type(dialog, "nameEdit", "driven");
        check(dialog, "zpkRadio");
        type(dialog, "zpkNumerator", "2");
        type(dialog, "zpkDenominator", "5 30");
        type(dialog, "zpkGain", "3");
        type(dialog, "zpkDelay", "0");
        press(dialog, "okButton");
    });

    QPushButton * plantButton = child<QPushButton>(&window, "plantButton");
    ASSERT_NE(plantButton, nullptr);
    plantButton->click();

    //The step counts as done, which the progress bar is what says out loud.
    QProgressBar * progress = child<QProgressBar>(&window, "progressBar");
    ASSERT_NE(progress, nullptr);
    EXPECT_EQ(progress->value(), 1);

    //A plant on its own opens nothing: the templates need the frequencies as
    //well, and everything below needs the templates. Written down because it
    //is the sort of thing the seven flags could get wrong quietly.
    QPushButton * templates = child<QPushButton>(&window, "templatesButton");
    ASSERT_NE(templates, nullptr);
    EXPECT_FALSE(templates->isEnabled());

    QPushButton * boundaries = child<QPushButton>(&window, "boundariesButton");
    ASSERT_NE(boundaries, nullptr);
    EXPECT_FALSE(boundaries->isEnabled());
}

TEST_F(GuiSmoke, ARejectedDialogLeavesTheStepUndone)
{
    MainWindow window;

    //A user who opens the dialog and cancels: the handler has to undo the
    //step rather than leave it half done.
    window.setDialogRunner([](QDialog *) { /* opened and closed */ });

    QPushButton * plantButton = child<QPushButton>(&window, "plantButton");
    ASSERT_NE(plantButton, nullptr);
    plantButton->click();

    QProgressBar * progress = child<QProgressBar>(&window, "progressBar");
    ASSERT_NE(progress, nullptr);
    EXPECT_EQ(progress->value(), 0)
        << "a cancelled step must not count as done";
}

} // namespace
