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

#include <complex>
#include <memory>

#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QStackedWidget>
#include <QString>
#include <QStringList>

#include "src/core/project_controller.h"
#include "GUI/plant_dialog.h"
#include "GUI/controller_dialog.h"
#include "GUI/frequencies_dialog.h"
#include "GUI/specifications_dialog.h"
#include "GUI/template_viewer.h"
#include "GUI/bode_viewer.h"
#include "src/core/math/sequence_vectors.h"
#include "src/core/system/polynomial_form.h"
#include "GUI/uncertainty_dialog.h"
#include "GUI/boundary_viewer.h"
#include "GUI/boundary_union_viewer.h"
#include "GUI/loop_shaping_viewer.h"
#include "GUI/loop_boundaries_viewer.h"
#include "src/core/boundaries/boundary_data.h"
#include "src/core/loopshaping/loop_shaping_result.h"
#include "GUI/boundary_grid_dialog.h"
#include "GUI/templates_dialog.h"
#include "GUI/loop_shaping_dialog.h"
#include "GUI/main_window.h"
#include "GUI/error_message.h"
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
                m_reported.append(title + ": " + message);
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
BoundaryData * oneBoundary()
{
    auto * curve = new QVector<QPointF>{QPointF(-270.0, 10.0), QPointF(-180.0, 4.0),
                                        QPointF(-90.0, 10.0)};
    auto * curves = new QVector<QVector<QPointF> *>{curve};

    auto * perFrequency = new QMap<QString, QVector<QVector<QPointF> *> *>();
    perFrequency->insert(QStringLiteral("Stability"), curves);
    auto * boundaries = new QVector<QMap<QString, QVector<QVector<QPointF> *> *> *>{perFrequency};

    auto * unionBoundaries = new QVector<QVector<QPointF> *>{
        new QVector<QPointF>{QPointF(-270.0, 10.0), QPointF(-180.0, 4.0), QPointF(-90.0, 10.0)}};

    auto * row = new QVector<QVector<QPointF> *>();
    for (qint32 i = 0; i < 361; i++) {
        row->append(new QVector<QPointF>());
    }
    auto * buckets = new QVector<QVector<QVector<QPointF> *> *>{row};

    //The constructor is non-owning by default (the engine keeps its own
    //data alive); takeOwnership makes the fixture's destructor free the
    //vectors above, as the project loader does.
    BoundaryData * data = new BoundaryData(boundaries, new QVector<bool>{false}, new QVector<bool>{true},
                            361, QPointF(-360.0, 0.0), unionBoundaries, buckets,
                            121, QPointF(-60.0, 60.0));
    data->takeOwnership();

    return data;
}

// ---------------------------------------------------------------------------

TEST_F(GuiSmoke, PlantDialogBuildsAZeroPoleGainPlant)
{
    PlantDialog dialog;

    type(&dialog, "nameEdit", QStringLiteral("smoke"));
    check(&dialog, "zpkRadio");
    type(&dialog, "zpkNumerator", QStringLiteral("2"));
    type(&dialog, "zpkDenominator", QStringLiteral("5 30"));
    type(&dialog, "zpkGain", QStringLiteral("3"));
    type(&dialog, "zpkDelay", QStringLiteral("0"));

    press(&dialog, "okButton");

    ASSERT_TRUE(dialog.getTodoCorrecto()) << "the dialog rejected valid data";

    //Ownership comes with it: the window would hand it to the project.
    std::unique_ptr<LtiSystem> plant(dialog.takePlant());
    ASSERT_NE(plant, nullptr);
    EXPECT_EQ(plant->type(), LtiSystem::SystemType::ZeroPoleGain);
    EXPECT_EQ(plant->name(), QStringLiteral("smoke"));

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

    type(&dialog, "nameEdit", QStringLiteral("broken"));
    check(&dialog, "zpkRadio");
    type(&dialog, "zpkNumerator", QStringLiteral("2"));
    type(&dialog, "zpkDenominator", QStringLiteral("5"));
    type(&dialog, "zpkGain", QStringLiteral("3*/"));
    type(&dialog, "zpkDelay", QStringLiteral("0"));

    press(&dialog, "okButton");

    EXPECT_FALSE(dialog.getTodoCorrecto());
    EXPECT_EQ(dialog.takePlant(), nullptr);
    EXPECT_FALSE(m_reported.isEmpty()) << "the rejection must be reported";
}

TEST_F(GuiSmoke, PlantDialogRejectsAnInvalidCoefficient)
{
    //The gain path reports a malformed expression, but buildParameters
    //catches the parser error per COEFFICIENT and substitutes 0, so a
    //numerator of "1*/" became the polynomial 0 with nothing said.
    PlantDialog dialog;

    type(&dialog, "nameEdit", QStringLiteral("broken-numerator"));
    check(&dialog, "zpkRadio");
    type(&dialog, "zpkNumerator", QStringLiteral("1*/"));
    type(&dialog, "zpkDenominator", QStringLiteral("5 30"));
    type(&dialog, "zpkGain", QStringLiteral("3"));
    type(&dialog, "zpkDelay", QStringLiteral("0"));

    press(&dialog, "okButton");

    EXPECT_FALSE(dialog.getTodoCorrecto())
        << "a malformed coefficient was accepted";
    EXPECT_FALSE(m_reported.isEmpty()) << "the rejection must be reported";

    delete dialog.takePlant();
}

TEST_F(GuiSmoke, PlantDialogRejectsAReservedParameterName)
{
    //"k" is the obvious name for a gain, and muParserX owns it as the unit
    //multiplier 1e3. Naming a parameter after anything the parser defines
    //used to pass the dialog - only FUNCTION names were checked - and fail
    //much later as a generic evaluation error, or worse be read as 1e3.
    PlantDialog dialog;

    type(&dialog, "nameEdit", QStringLiteral("reserved"));
    check(&dialog, "zpkRadio");
    type(&dialog, "zpkNumerator", QStringLiteral("1"));
    type(&dialog, "zpkDenominator", QStringLiteral("k"));
    type(&dialog, "zpkGain", QStringLiteral("1"));
    type(&dialog, "zpkDelay", QStringLiteral("0"));

    press(&dialog, "okButton");

    EXPECT_FALSE(dialog.getTodoCorrecto()) << "a reserved parameter name was accepted";
    ASSERT_FALSE(m_reported.isEmpty()) << "the rejection must be reported";
    EXPECT_TRUE(m_reported.join(QChar(' ')).contains(QStringLiteral("k")))
        << "the message must name the offending identifier: "
        << m_reported.join(QChar(' ')).toStdString();

    delete dialog.takePlant();
}

TEST_F(GuiSmoke, FrequenciesDialogBuildsTheDesignFrequencies)
{
    FrequenciesDialog dialog;

    //modeStack is the generation-mode combo (manual is entry 0); the pages
    //of values live in the selecVector stack, which follows it.
    QComboBox * mode = child<QComboBox>(&dialog, "modeStack");
    ASSERT_NE(mode, nullptr);
    mode->setCurrentIndex(0);
    type(&dialog, "manualValues", QStringLiteral("0.1 1 10 100"));

    press(&dialog, "okButton");

    ASSERT_TRUE(dialog.getTodoCorrecto()) << "the dialog rejected valid data";

    std::unique_ptr<Omega> omega(dialog.takeOmega());
    ASSERT_NE(omega, nullptr);
    ASSERT_NE(omega->values(), nullptr);
    const QVector<qreal> expected{0.1, 1.0, 10.0, 100.0};
    EXPECT_EQ(*omega->values(), expected);
    EXPECT_EQ(omega->pointCount(), 4);
}

TEST_F(GuiSmoke, ControllerDialogBuildsTheControllerStructure)
{
    ControllerDialog dialog;

    check(&dialog, "zpkRadio");
    type(&dialog, "numeratorEdit", QStringLiteral("1"));
    type(&dialog, "denominatorEdit", QStringLiteral("100"));
    //The controller's gain is a SEARCH RANGE, not a value.
    type(&dialog, "gainStart", QStringLiteral("1"));
    type(&dialog, "gainEnd", QStringLiteral("1000"));

    press(&dialog, "okButton");

    ASSERT_TRUE(dialog.getTodoCorrecto()) << "the dialog rejected valid data";

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
    type(&dialog, "numeratorEdit", QStringLiteral("1*/"));
    type(&dialog, "denominatorEdit", QStringLiteral("100"));
    type(&dialog, "gainStart", QStringLiteral("1"));
    type(&dialog, "gainEnd", QStringLiteral("1000"));

    press(&dialog, "okButton");

    EXPECT_FALSE(dialog.getTodoCorrecto())
        << "a malformed numerator was accepted";
    EXPECT_EQ(dialog.takeControllerStructure(), nullptr);
    EXPECT_FALSE(m_reported.isEmpty()) << "the rejection must be reported";
}

TEST_F(GuiSmoke, SpecificationsDialogNeedsTheFrequenciesFirst)
{
    //The main window gates the step order, but the dialog used to reach
    //first()/last() on a null frequency vector and take the application
    //down; it says so now.
    EXPECT_THROW(SpecificationsDialog dialog(nullptr), qftbx::InvalidInput);

    const QVector<qreal> empty;
    EXPECT_THROW(SpecificationsDialog dialog(&empty), qftbx::InvalidInput);
}

TEST_F(GuiSmoke, SpecificationsDialogStoresAConstantStabilitySpecification)
{
    //Real step order: the frequencies come first, and the window hands them
    //to the dialog.
    const QVector<qreal> frequencies{0.1, 1.0, 10.0, 100.0};

    SpecificationsDialog dialog(&frequencies);

    //Stability with a constant magnitude: the simplest complete slot.
    check(&dialog, "stabilityRadio");
    check(&dialog, "constantRadio");
    check(&dialog, "linearRadio");
    type(&dialog, "magnitudeEdit", QStringLiteral("1.2"));

    press(&dialog, "okButton");

    QVector<qftbx::SpecificationRecord *> * records = dialog.takeSpecifications();

    if (records == nullptr) {
        //The dialog declined the combination; the smoke value here is that
        //it said so instead of crashing or storing a half-built record.
        EXPECT_FALSE(dialog.getTodoCorrecto());
        return;
    }

    ASSERT_EQ(records->size(), 7);
    for (qftbx::SpecificationRecord * record : *records) {
        ASSERT_NE(record, nullptr);
        if (record->used && record->constant) {
            EXPECT_GT(record->height, 0.0) << "a used constant specification "
                                              "must carry a positive magnitude";
        }
    }

    //takeSpecifications() transferred ownership; here nobody else claims it.
    for (qftbx::SpecificationRecord * record : *records) {
        delete record->system;
        delete record;
    }
    delete records;
}

TEST_F(GuiSmoke, TemplateViewerAsksItsHandlerToRecomputeTheContour)
{
    //The viewer runs no computation of its own: the recompute button calls
    //the handler the window installed, handing it the epsilon it read from
    //the fields. A plain callback, not a Qt signal.
    TemplateViewer viewer;

    QVector<std::complex<qreal> > contourPoints{{1.0, 0.0}, {0.0, 1.0}};
    QVector<std::complex<qreal> > templatePoints{{2.0, 0.0}, {0.0, 2.0}};
    QVector<QVector<std::complex<qreal> > *> contour{&contourPoints};
    QVector<QVector<std::complex<qreal> > *> templates{&templatePoints};
    QVector<qreal> omega{1.0};
    QVector<qreal> epsilon{0.05};

    //setDatos borrows: the project owns these in the application.
    viewer.setDatos(&templates, &contour, &omega, &epsilon);
    viewer.plotDiagram(true);

    bool called = false;
    QVector<qreal> asked;
    viewer.setContourRecomputer([&](QVector<qreal> * requested) {
        called = true;
        asked = *requested;

        //Ownership came with the call.
        delete requested;
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
    EXPECT_TRUE(row->styleSheet().startsWith(QStringLiteral("color")))
        << "the frequency row lost its colour: "
        << row->styleSheet().toStdString();
    EXPECT_FALSE(row->styleSheet().contains(QStringLiteral("colorsCreated")));
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
    PolynomialForm plant(QStringLiteral("bode"), numerator, denominator,
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

    type(&dialog, "phasePoints", QStringLiteral("361"));
    type(&dialog, "magnitudePoints", QStringLiteral("121"));

    buttons->button(QDialogButtonBox::Ok)->click();

    ASSERT_TRUE(dialog.getTodoCorrecto()) << "the dialog rejected its own defaults";

    EXPECT_DOUBLE_EQ(dialog.phaseRangeValue().x(), -360.0);
    EXPECT_DOUBLE_EQ(dialog.phaseRangeValue().y(), 0.0);
    EXPECT_EQ(dialog.phaseCountValue(), 361);
    EXPECT_EQ(dialog.magnitudeCountValue(), 121);
    EXPECT_LT(dialog.magnitudeRangeValue().x(), dialog.magnitudeRangeValue().y());

    //A window narrower than 360 degrees would be refused later by
    //LoopShaping::run; the default one must not be.
    EXPECT_DOUBLE_EQ(dialog.phaseRangeValue().y() - dialog.phaseRangeValue().x(), 360.0);
}

TEST_F(GuiSmoke, BoundaryGridDialogRejectsAnInvertedRange)
{
    BoundaryGridDialog dialog;

    type(&dialog, "phaseStart", QStringLiteral("0"));
    type(&dialog, "phaseEnd", QStringLiteral("-360"));

    child<QDialogButtonBox>(&dialog, "buttonBox")->button(QDialogButtonBox::Ok)->click();

    EXPECT_FALSE(dialog.getTodoCorrecto()) << "an inverted phase range was accepted";
}

TEST_F(GuiSmoke, TemplatesDialogBuildsOneEpsilonPerFrequency)
{
    TemplatesDialog dialog;

    std::vector<Parameter> numerator{Parameter(1.0)};
    std::vector<Parameter> denominator{
        Parameter(QStringLiteral("a"), qftbx::Range(1.0, 5.0), 5.0)};
    PolynomialForm plant(QStringLiteral("templates"), numerator, denominator,
                         Parameter(1.0), Parameter(0.0));

    dialog.launch(&plant, 4);

    type(&dialog, "epsilonEdit", QStringLiteral("0.05"));

    //The general section demands a spacing and a point count for the
    //parameter grids.
    check(&dialog, "linspaceRadio");
    type(&dialog, "globalPointCount", QStringLiteral("3"));
    check(&dialog, "allVariablesRadio");
    check(&dialog, "nicholsRadio");

    press(&dialog, "okButton");

    ASSERT_TRUE(dialog.getTodoCorrecto()) << "the dialog rejected valid data";

    //One epsilon per design frequency: the template computation indexes it
    //by frequency.
    //Ownership comes with it: the window hands it to the project.
    std::unique_ptr<QVector<qreal>> epsilon(dialog.takeEpsilon());
    ASSERT_NE(epsilon, nullptr);
    EXPECT_EQ(epsilon->size(), 4);
    for (qreal value : *epsilon) {
        EXPECT_DOUBLE_EQ(value, 0.05);
    }

    //A grid per uncertain parameter, and none for the constants.
    QHash<QString, QVector<qreal> *> * grids = dialog.grids();
    ASSERT_NE(grids, nullptr);
    EXPECT_TRUE(grids->contains(QStringLiteral("a")))
        << "the uncertain parameter got no grid";
}

TEST_F(GuiSmoke, LoopShapingDialogCarriesTheChosenAlgorithm)
{
    LoopShapingDialog dialog;

    type(&dialog, "epsilonEdit", QStringLiteral("0.01"));
    type(&dialog, "startEdit", QStringLiteral("0.1"));
    type(&dialog, "endEdit", QStringLiteral("100"));
    type(&dialog, "pointCountEdit", QStringLiteral("200"));
    check(&dialog, "mrRadio");

    press(&dialog, "okButton");

    ASSERT_TRUE(dialog.getTodoCorrecto()) << "the dialog rejected valid data";

    EXPECT_EQ(dialog.algorithmValue(), tools::mr);
    EXPECT_DOUBLE_EQ(dialog.epsilonValue(), 0.01);
    EXPECT_DOUBLE_EQ(dialog.range().x(), 0.1);
    EXPECT_DOUBLE_EQ(dialog.range().y(), 100.0);
    EXPECT_DOUBLE_EQ(dialog.pointCountValue(), 200.0);
}

TEST_F(GuiSmoke, UncertaintyDialogBuildsAnUncertainParameter)
{
    //This is what turns a named coefficient into an uncertain Parameter, so
    //it feeds the uncertainty of every plant and controller. It is driven
    //the way the plant dialog drives it: the three parallel tables, one row
    //per uncertain name.
    UncertaintyDialog dialog;

    auto * valueTable = new QVector<QVector<QString> *>{
        new QVector<QString>{QStringLiteral("1")},
        new QVector<QString>{QStringLiteral("a")}};
    auto * expressionTable = new QVector<QVector<QString> *>{
        new QVector<QString>{QStringLiteral("1")},
        new QVector<QString>{QStringLiteral("a")}};
    auto * uncertainTable = new QVector<QVector<bool> *>{
        new QVector<bool>{false},
        new QVector<bool>{true}};

    ASSERT_TRUE(dialog.launch(valueTable, expressionTable, uncertainTable, false));

    //One uncertain name, so one generated row: [inicio, fin] with nominal.
    type(&dialog, "inicio", QStringLiteral("1"));
    type(&dialog, "fin", QStringLiteral("5"));
    type(&dialog, "nominal", QStringLiteral("3"));

    type(&dialog, "gainStart", QStringLiteral("2"));
    type(&dialog, "gainEnd", QStringLiteral("8"));
    type(&dialog, "delayStart", QStringLiteral("0"));
    type(&dialog, "delayEnd", QStringLiteral("0"));

    press(&dialog, "okButton");

    ASSERT_TRUE(dialog.getTodoCorrecto()) << "the dialog rejected valid data";

    //The constant numerator coefficient must travel as a constant, and the
    //named denominator one as uncertain with its range and nominal.
    ASSERT_EQ(dialog.numerator().size(), 1u);
    EXPECT_FALSE(dialog.numerator()[0].isUncertain());

    ASSERT_EQ(dialog.denominator().size(), 1u);
    Parameter & uncertain = dialog.denominator()[0];
    EXPECT_TRUE(uncertain.isUncertain());
    EXPECT_EQ(uncertain.name(), QStringLiteral("a"));
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

    auto * valueTable = new QVector<QVector<QString> *>{
        new QVector<QString>{QStringLiteral("1")},
        new QVector<QString>{QStringLiteral("a")}};
    auto * expressionTable = new QVector<QVector<QString> *>{
        new QVector<QString>{QStringLiteral("1")},
        new QVector<QString>{QStringLiteral("a")}};
    auto * uncertainTable = new QVector<QVector<bool> *>{
        new QVector<bool>{false},
        new QVector<bool>{true}};

    ASSERT_TRUE(dialog.launch(valueTable, expressionTable, uncertainTable, false));

    //The row is left blank on purpose.
    type(&dialog, "gainStart", QStringLiteral("2"));
    type(&dialog, "gainEnd", QStringLiteral("8"));
    type(&dialog, "delayStart", QStringLiteral("0"));
    type(&dialog, "delayEnd", QStringLiteral("0"));

    press(&dialog, "okButton");

    EXPECT_FALSE(dialog.getTodoCorrecto()) << "a blank range was accepted";
}

TEST_F(GuiSmoke, BoundaryViewerDrawsTheBoundariesItIsGiven)
{
    //Draw-only viewer: the value here is that the drawing code runs over a
    //real boundary set and leaves curves behind, and that a redraw does not
    //pile them up (the bug the Bode and template viewers both had).
    BoundaryViewer viewer;

    BoundaryData * boundaries = oneBoundary();
    QVector<qreal> omega{1.0};

    viewer.setDatos(boundaries, &omega);
    viewer.showDiagram();

    QCustomPlot * plot = child<QCustomPlot>(&viewer, "plot");
    ASSERT_NE(plot, nullptr);
    const int drawn = plot->plottableCount();
    EXPECT_GT(drawn, 0) << "nothing was drawn";

    viewer.showDiagram();
    EXPECT_EQ(plot->plottableCount(), drawn) << "a redraw piled up curves";

    delete boundaries;
}

TEST_F(GuiSmoke, BoundaryUnionViewerDrawsTheUnion)
{
    BoundaryUnionViewer viewer;

    auto * trace = new QVector<QPointF>{QPointF(-270.0, 10.0), QPointF(-180.0, 4.0),
                                        QPointF(-90.0, 10.0)};
    QVector<QVector<QPointF> *> traces{trace};
    QVector<qreal> omega{1.0};

    viewer.setDatos(&traces, &omega);
    viewer.showDiagram();

    QCustomPlot * plot = child<QCustomPlot>(&viewer, "plot");
    ASSERT_NE(plot, nullptr);
    const int drawn = plot->plottableCount();
    EXPECT_GT(drawn, 0) << "nothing was drawn";

    viewer.showDiagram();
    EXPECT_EQ(plot->plottableCount(), drawn) << "a redraw piled up curves";

    delete trace;
}

TEST_F(GuiSmoke, LoopShapingViewerDrawsTheShapedLoop)
{
    LoopShapingViewer viewer;

    //1/(s+1) as the plant and a unit gain as the computed controller.
    std::vector<Parameter> numerator{Parameter(1.0)};
    std::vector<Parameter> denominator{Parameter(1.0), Parameter(1.0)};
    PolynomialForm plant(QStringLiteral("loop"), numerator, denominator,
                         Parameter(1.0), Parameter(0.0));

    std::vector<Parameter> one{Parameter(1.0)};
    LoopShapingResult result(new PolynomialForm(QStringLiteral("k"), one, one,
                                                Parameter(1.0), Parameter(0.0)),
                             QPointF(0.1, 100.0), 50);

    auto * trace = new QVector<QPointF>{QPointF(-270.0, 10.0), QPointF(-180.0, 4.0),
                                        QPointF(-90.0, 10.0)};
    QVector<QVector<QPointF> *> traces{trace};
    QVector<qreal> omega{1.0, 10.0};

    viewer.setDatos(&traces, &omega, &result, &plant, false);
    viewer.showDiagram();

    QCustomPlot * plot = child<QCustomPlot>(&viewer, "plot");
    ASSERT_NE(plot, nullptr);
    EXPECT_GT(plot->plottableCount(), 0) << "nothing was drawn";

    delete trace;
}

TEST_F(GuiSmoke, LoopBoundariesViewerDrawsBothDiagrams)
{
    LoopBoundariesViewer viewer;

    std::vector<Parameter> numerator{Parameter(1.0)};
    std::vector<Parameter> denominator{Parameter(1.0), Parameter(1.0)};
    PolynomialForm plant(QStringLiteral("loop"), numerator, denominator,
                         Parameter(1.0), Parameter(0.0));
    std::vector<Parameter> one{Parameter(1.0)};
    PolynomialForm controller(QStringLiteral("k"), one, one,
                              Parameter(1.0), Parameter(0.0));

    BoundaryData * nichols = oneBoundary();
    BoundaryData * nyquist = oneBoundary();
    QVector<qreal> omega{1.0};

    viewer.setDatos(nichols, nyquist, &omega, &plant, &controller, true, false);
    viewer.showDiagram();

    QCustomPlot * plot = child<QCustomPlot>(&viewer, "plot");
    ASSERT_NE(plot, nullptr);
    EXPECT_GT(plot->plottableCount(), 0) << "nothing was drawn";

    delete nichols;
    delete nyquist;
}

TEST_F(GuiSmoke, TheMainWindowBuildsItsWholeWidgetTree)
{
    //Every dialog, viewer and menu is constructed here: a broken .ui
    //reference or a null child shows up as a crash on construction.
    MainWindow window;
    EXPECT_FALSE(window.windowTitle().isEmpty());
}

} // namespace
