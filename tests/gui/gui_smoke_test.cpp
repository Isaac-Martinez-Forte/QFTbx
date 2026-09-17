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

#include "src/core/loopshaping/loop_shaping_types.h"
#include "src/core/specifications/specification_record.h"
#include <cstdio>
#include <gtest/gtest.h>

#include <vector>

#include "src/core/math/point.h"

#include "src/core/math/range.h"

#include <complex>
#include <memory>

#include <QAction>
#include "src/core/pipeline/pipeline_step.h"
#include <QScrollArea>
#include "src/gui/common/frequency_legend.h"
#include <QTemporaryDir>
#include <QImage>
#include <QPixmap>
#include <QPainter>
#include <QFile>
#include "src/gui/common/plot_export.h"
#include <QSet>
#include <QThread>
#include <QCoreApplication>
#include <QEvent>
#include "src/gui/common/plot_setup.h"
#include <QCheckBox>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QIcon>
#include <QDockWidget>
#include <QToolButton>
#include <QStackedWidget>
#include <QTableWidget>
#include <QString>
#include <QStringList>

#include "src/app/project_controller.h"
#include "src/gui/common/formula_delegate.h"
#include "src/gui/common/formula_view.h"
#include "src/core/system/system_formula.h"
#include "src/gui/application/about.h"
#include "src/gui/application/language.h"
#include "src/gui/application/main_window.h"
#include "src/gui/application/phase_card.h"
#include "src/gui/application/theme.h"
#include "src/gui/plant/plant_form.h"
#include "src/gui/loopshaping/controller_form.h"
#include "src/gui/frequencies/frequencies_form.h"
#include "src/gui/specifications/specifications_form.h"
#include "src/gui/templates/template_viewer.h"
#include "src/gui/plant/bode_viewer.h"
#include "src/core/math/sequence_vectors.h"
#include "src/core/system/polynomial_form.h"
#include "src/gui/plant/uncertainty_panel.h"
#include "src/gui/boundaries/boundary_viewer.h"
#include "src/gui/boundaries/boundary_union_viewer.h"
#include "src/gui/loopshaping/loop_shaping_viewer.h"
#include "src/gui/loopshaping/loop_boundaries_viewer.h"
#include "src/core/boundaries/boundary_data.h"
#include "src/core/loopshaping/loop_shaping_result.h"
#include "src/gui/boundaries/boundary_grid_form.h"
#include "src/gui/templates/templates_form.h"
#include "src/gui/loopshaping/loop_shaping_form.h"
#include "src/gui/application/error_message.h"
#include "src/core/common/exception.h"

using namespace qftbx;

namespace {

//Every dialog reports invalid input through qftbx::errorMessage, which
//opens a MODAL dialog: an automated run would block on it forever. The
//fixture redirects it and keeps what was reported, so a rejection can be
//asserted rather than waited on.
class GuiSmoke : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_previous = qftbx::setErrorReporter(
            [this](const QString & message, const QString & title) {
                m_reported.push_back(title + ": " + message);
            });
    }

    void TearDown() override
    {
        qftbx::setErrorReporter(m_previous);
    }

    QStringList m_reported;

private:
    qftbx::ErrorReporter m_previous;
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

//The forms mark what is wrong WHERE it is wrong and say it in their status
//line, instead of stopping everything with a message box.
QString complaintOf(QWidget * form)
{
    QLabel * status = form->findChild<QLabel *>(QStringLiteral("statusLabel"));
    return status != nullptr ? status->text() : QString();
}

bool isMarked(QWidget * form, const char * name)
{
    QWidget * field = form->findChild<QWidget *>(QString::fromUtf8(name));
    return field != nullptr && field->property("wrong").toBool();
}

//One button and two steps: nothing is applied that has not been verified,
//and verifying is what draws the formula.
void verifyAndApply(QWidget * form)
{
    press(form, "okButton");
    press(form, "okButton");
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

TEST_F(GuiSmoke, PlantFormBuildsAZeroPoleGainPlant)
{
    PlantForm form;

    type(&form, "nameEdit", "smoke");
    type(&form, "descriptionEdit", "the one of the smoke test");
    check(&form, "transferFunctionRadio");
    check(&form, "zerosPolesRadio");
    check(&form, "zpkRadio");
    type(&form, "numeratorEdit", "2");
    type(&form, "denominatorEdit", "5 30");
    type(&form, "gainEdit", "3");
    type(&form, "delayEdit", "0");

    //Verifying draws the formula and offers to apply; nothing is applied
    //before that.
    press(&form, "okButton");
    EXPECT_FALSE(form.wasAccepted()) << "verifying is not applying";
    EXPECT_EQ(child<QPushButton>(&form, "okButton")->text(), QString("Apply"));

    press(&form, "okButton");

    ASSERT_TRUE(form.wasAccepted()) << "the form rejected valid data: "
                                    << complaintOf(&form).toStdString();

    //Ownership comes with it: the window would hand it to the project.
    std::unique_ptr<LtiSystem> plant(form.takePlant());
    ASSERT_NE(plant, nullptr);
    EXPECT_EQ(plant->type(), LtiSystem::SystemType::ZeroPoleGain);
    EXPECT_EQ(plant->name(), "smoke");
    EXPECT_EQ(plant->description(), "the one of the smoke test");

    //The numbers must have travelled into parameter VALUES.
    ASSERT_EQ(plant->numerator().size(), 1u);
    EXPECT_DOUBLE_EQ(plant->numerator()[0].nominal(), 2.0);
    ASSERT_EQ(plant->denominator().size(), 2u);
    EXPECT_DOUBLE_EQ(plant->denominator()[0].nominal(), 5.0);
    EXPECT_DOUBLE_EQ(plant->denominator()[1].nominal(), 30.0);
    EXPECT_DOUBLE_EQ(plant->gain().nominal(), 3.0);
    EXPECT_DOUBLE_EQ(plant->delay().nominal(), 0.0);
}

TEST_F(GuiSmoke, ThePathOfTheFamilyStaysMarkedWholeAndTheFormulaFollowsIt)
{
    //Every level of the path is a group of its own, so choosing the leaf
    //does not unmark the branch it hangs from - a plant written as zeros
    //and poles is a transfer function first.
    PlantForm form;

    type(&form, "nameEdit", "path");
    check(&form, "transferFunctionRadio");
    check(&form, "zerosPolesRadio");
    check(&form, "tcgRadio");

    EXPECT_TRUE(child<QRadioButton>(&form, "transferFunctionRadio")->isChecked());
    EXPECT_TRUE(child<QRadioButton>(&form, "zerosPolesRadio")->isChecked());
    EXPECT_TRUE(child<QRadioButton>(&form, "tcgRadio")->isChecked());

    //And the fields are named for the family in use.
    EXPECT_EQ(child<QLabel>(&form, "numeratorLabel")->text(), QString("Zeros:"));
    EXPECT_EQ(child<QLabel>(&form, "denominatorLabel")->text(), QString("Poles:"));

    type(&form, "numeratorEdit", "");
    type(&form, "denominatorEdit", "2");
    type(&form, "gainEdit", "5");

    QStackedWidget * figures = child<QStackedWidget>(&form, "figureStack");
    ASSERT_NE(figures, nullptr);
    EXPECT_EQ(figures->currentIndex(), 0) << "the figure of the family until it is verified";

    press(&form, "okButton");

    EXPECT_EQ(figures->currentIndex(), 1) << "verified, the formula takes its place";

    FormulaView * formula = child<FormulaView>(&form, "formulaView");
    ASSERT_NE(formula, nullptr);
    EXPECT_EQ(formula->latex().toStdString(),
              "5 \\cdot \\frac{1}{\\left(1 + \\frac{s}{2}\\right)}");

    //Changing the family takes the formula away again: it described the
    //other one.
    check(&form, "zpkRadio");
    EXPECT_EQ(figures->currentIndex(), 0);
    EXPECT_EQ(child<QPushButton>(&form, "okButton")->text(), QString("Verify"));
}

TEST_F(GuiSmoke, PlantFormRejectsAnInvalidExpression)
{
    //A malformed coefficient must be reported, not crash the application
    //(the expression parser throws and the form used to let it through).
    PlantForm form;

    type(&form, "nameEdit", "broken");
    check(&form, "zpkRadio");
    type(&form, "numeratorEdit", "2");
    type(&form, "denominatorEdit", "5");
    type(&form, "gainEdit", "3*/");
    type(&form, "delayEdit", "0");

    verifyAndApply(&form);

    EXPECT_FALSE(form.wasAccepted());
    EXPECT_EQ(form.takePlant(), nullptr);
    EXPECT_FALSE(complaintOf(&form).isEmpty()) << "the rejection must be said";
    EXPECT_TRUE(isMarked(&form, "gainEdit")) << "the field that is wrong must be marked";
}

TEST_F(GuiSmoke, PlantFormRejectsAnInvalidCoefficient)
{
    //The gain path reports a malformed expression, but buildParameters
    //catches the parser error per COEFFICIENT and substitutes 0, so a
    //numerator of "1*/" became the polynomial 0 with nothing said.
    PlantForm form;

    type(&form, "nameEdit", "broken-numerator");
    check(&form, "zpkRadio");
    type(&form, "numeratorEdit", "1*/");
    type(&form, "denominatorEdit", "5 30");
    type(&form, "gainEdit", "3");
    type(&form, "delayEdit", "0");

    verifyAndApply(&form);

    EXPECT_FALSE(form.wasAccepted()) << "a malformed coefficient was accepted";
    EXPECT_FALSE(complaintOf(&form).isEmpty()) << "the rejection must be said";
    EXPECT_TRUE(isMarked(&form, "numeratorEdit"));

    form.takePlant();
}

TEST_F(GuiSmoke, PlantFormRejectsAReservedParameterName)
{
    //"pi" is a constant of the expression grammar: a parameter under that
    //name would be read as the constant, never as the parameter. Naming a
    //parameter after anything the grammar defines used to pass the form -
    //only FUNCTION names were checked - and fail much later.
    PlantForm form;

    type(&form, "nameEdit", "reserved");
    check(&form, "zpkRadio");
    type(&form, "numeratorEdit", "1");
    type(&form, "denominatorEdit", "pi");
    type(&form, "gainEdit", "1");
    type(&form, "delayEdit", "0");

    verifyAndApply(&form);

    EXPECT_FALSE(form.wasAccepted()) << "a reserved parameter name was accepted";
    EXPECT_TRUE(complaintOf(&form).contains("pi"))
        << "the complaint must name the offending identifier: "
        << complaintOf(&form).toStdString();
    EXPECT_TRUE(isMarked(&form, "denominatorEdit"));

    form.takePlant();
}

TEST_F(GuiSmoke, ANameWithADigitInItIsTheWholeName)
{
    //The reader took the LETTERS of a coefficient as its name, so "z1" was
    //a parameter called "z": the ranges of a controller written with z1 and
    //p1 were filed under names nothing else in the program had heard of,
    //and the uncertainty page came up empty over a project that had them.
    ControllerForm form;

    check(&form, "zpkRadio");
    type(&form, "numeratorEdit", "z1");
    type(&form, "denominatorEdit", "p1 p2");
    type(&form, "gainStart", "1");
    type(&form, "gainEnd", "1000");

    press(&form, "uncertaintyButton");

    //One row per NAME, and the names are whole.
    qftbx::UncertaintyPanel * freedom = form.findChild<qftbx::UncertaintyPanel *>();
    ASSERT_NE(freedom, nullptr);

    QStringList named;
    for (QLabel * label : freedom->findChildren<QLabel *>()) {
        if (label->objectName().isEmpty() && !label->text().isEmpty()) {
            named << label->text();
        }
    }

    EXPECT_TRUE(named.contains("z1")) << named.join(" ").toStdString();
    EXPECT_TRUE(named.contains("p1")) << named.join(" ").toStdString();
    EXPECT_TRUE(named.contains("p2")) << named.join(" ").toStdString();
    EXPECT_FALSE(named.contains("z")) << "a name was cut at its first digit";
}

TEST_F(GuiSmoke, ACoefficientInScientificNotationIsANumberAndNotAParameter)
{
    //The e of 1e3 was read as the constant e, and the whole plant was
    //refused with a complaint about a name nobody had typed.
    PlantForm form;

    type(&form, "nameEdit", "scientific");
    check(&form, "transferFunctionRadio");
    check(&form, "polynomialRadio");
    type(&form, "numeratorEdit", "1e3");
    type(&form, "denominatorEdit", "1 2.5E-4");
    type(&form, "gainEdit", "1");
    type(&form, "delayEdit", "0");

    verifyAndApply(&form);

    ASSERT_TRUE(form.wasAccepted()) << complaintOf(&form).toStdString();

    const std::unique_ptr<LtiSystem> plant = form.takePlant();
    ASSERT_NE(plant, nullptr);
    ASSERT_EQ(plant->numerator().size(), 1u);
    EXPECT_FALSE(plant->numerator()[0].isUncertain());
    EXPECT_DOUBLE_EQ(plant->numerator()[0].nominal(), 1000.0);
    ASSERT_EQ(plant->denominator().size(), 2u);
    EXPECT_DOUBLE_EQ(plant->denominator()[1].nominal(), 2.5e-4);
}

TEST_F(GuiSmoke, APlantWithoutANameIsNotVerified)
{
    PlantForm form;

    check(&form, "zpkRadio");
    type(&form, "numeratorEdit", "1");
    type(&form, "denominatorEdit", "2");

    verifyAndApply(&form);

    EXPECT_FALSE(form.wasAccepted());
    EXPECT_TRUE(isMarked(&form, "nameEdit"));
}

TEST_F(GuiSmoke, FrequenciesDialogBuildsTheDesignFrequencies)
{
    FrequenciesForm dialog;

    //modeStack is the generation-mode combo (manual is entry 0); the pages
    //of values live in the selecVector stack, which follows it.
    QComboBox * mode = child<QComboBox>(&dialog, "modeStack");
    ASSERT_NE(mode, nullptr);
    mode->setCurrentIndex(0);
    type(&dialog, "manualValues", "0.1 1 10 100");

    press(&dialog, "okButton");

    ASSERT_TRUE(dialog.wasAccepted()) << "the form rejected valid data";

    std::unique_ptr<Omega> omega(dialog.takeOmega());
    ASSERT_NE(omega, nullptr);
    ASSERT_NE(omega->values(), nullptr);
    const std::vector<double> expected{0.1, 1.0, 10.0, 100.0};
    EXPECT_EQ(*omega->values(), expected);
    EXPECT_EQ(omega->pointCount(), 4);
}

TEST_F(GuiSmoke, ASpecificationThatCannotBeReadIsNotAddedToTheList)
{
    //What the seven tabs used to get wrong: a field that could not be read
    //lost the whole specification and showed a blank tab. Here nothing
    //leaves the form until it has been verified, the field that is wrong is
    //marked, and what was typed stays where it can be corrected.
    const std::vector<double> frequencies{0.1, 1.0, 10.0};
    SpecificationsForm form(&frequencies);

    QComboBox * typeCombo = child<QComboBox>(&form, "typeCombo");
    ASSERT_NE(typeCombo, nullptr);
    typeCombo->setCurrentIndex(int(qftbx::SpecificationType::Stability));

    check(&form, "constantRadio");
    check(&form, "linearRadio");
    type(&form, "magnitudeEdit", "1.2*/");

    press(&form, "addButton");

    EXPECT_FALSE(complaintOf(&form).isEmpty()) << "the refusal must be said";
    EXPECT_TRUE(isMarked(&form, "magnitudeEdit"));
    EXPECT_EQ(child<QTableWidget>(&form, "specificationsTable")->rowCount(), 0)
        << "a specification that could not be read was added anyway";

    //The user's text is still on screen, where it can be corrected.
    EXPECT_EQ(child<QLineEdit>(&form, "magnitudeEdit")->text(), "1.2*/");
}

TEST_F(GuiSmoke, TheSpecificationsEnteredAreListedWithTheirBound)
{
    //The list is the design: every specification entered is on it, with the
    //band it holds over and the bound drawn as the formula it is.
    const std::vector<double> frequencies{0.1, 1.0, 10.0, 100.0};
    SpecificationsForm form(&frequencies);

    QComboBox * typeCombo = child<QComboBox>(&form, "typeCombo");
    QTableWidget * table = child<QTableWidget>(&form, "specificationsTable");
    ASSERT_NE(typeCombo, nullptr);
    ASSERT_NE(table, nullptr);

    //A constant stability bound, verified and added.
    typeCombo->setCurrentIndex(int(qftbx::SpecificationType::Stability));
    check(&form, "constantRadio");
    check(&form, "decibelsRadio");
    type(&form, "magnitudeEdit", "3.5");

    press(&form, "addButton");
    EXPECT_EQ(child<QPushButton>(&form, "addButton")->text(), QString("Add"))
        << "verifying offers to add: " << complaintOf(&form).toStdString();
    press(&form, "addButton");

    ASSERT_EQ(table->rowCount(), 1);

    //And a tracking upper bound as a transfer function.
    typeCombo->setCurrentIndex(int(qftbx::SpecificationType::TrackingUpper));
    check(&form, "systemRadio");
    check(&form, "polynomialRadio");
    type(&form, "numeratorEdit", "0.6584");
    type(&form, "denominatorEdit", "1 4 19.752");
    type(&form, "k", "1");

    press(&form, "addButton");
    press(&form, "addButton");

    ASSERT_EQ(table->rowCount(), 2);

    //And the stability row, which is the same closed loop without the
    //prefilter: the six requirements are the literature's, one per slot.
    const QVariant stability = table->item(1, 2)->data(qftbx::FormulaDelegate::formulaRole);
    ASSERT_TRUE(stability.canConvert<qftbx::Formula>());
    EXPECT_EQ(qftbx::latexOf(stability.value<qftbx::Formula>()),
              std::string("\\left|\\frac{L}{1 + L}\\right| \\leq 3.5 dB"));

    //The list is in the order of the slots, not of the typing: the tracking
    //bound comes before the stability one wherever they were entered.
    EXPECT_EQ(table->item(0, 0)->text(), QString("Tracking, upper bound"));
    EXPECT_EQ(table->item(1, 0)->text(), QString("Stability"));

    //The bound of the row carries its formula, which is what the list is
    //for: a quotient read as "1 4 19.752" says nothing.
    const QVariant held = table->item(0, 2)->data(qftbx::FormulaDelegate::formulaRole);
    ASSERT_TRUE(held.canConvert<qftbx::Formula>());
    //The whole requirement: what the program checks of the loop, the sign,
    //and the bound that was typed.
    EXPECT_EQ(qftbx::latexOf(held.value<qftbx::Formula>()),
              std::string("\\left|F\\frac{L}{1 + L}\\right| \\leq "
                          "\\frac{0.6584}{s^{2} + 4s + 19.75}"));

    //And the lower bound of the tracking band is the one that reads the
    //other way.
    typeCombo->setCurrentIndex(int(qftbx::SpecificationType::TrackingLower));
    check(&form, "constantRadio");
    check(&form, "decibelsRadio");
    type(&form, "magnitudeEdit", "-3");
    press(&form, "addButton");
    press(&form, "addButton");

    const QVariant lower = table->item(0, 2)->data(qftbx::FormulaDelegate::formulaRole);
    ASSERT_TRUE(lower.canConvert<qftbx::Formula>());
    EXPECT_EQ(qftbx::latexOf(lower.value<qftbx::Formula>()),
              std::string("\\left|F\\frac{L}{1 + L}\\right| \\geq -3 dB"));

    //Applying publishes the seven slots with the two that are used.
    press(&form, "okButton");
    ASSERT_TRUE(form.wasAccepted());

    const std::optional<qftbx::SpecificationRecords> records = form.takeSpecifications();
    ASSERT_TRUE(records.has_value());
    EXPECT_TRUE(records->at(int(qftbx::SpecificationType::Stability)).used);
    EXPECT_TRUE(records->at(int(qftbx::SpecificationType::TrackingUpper)).used);
    EXPECT_FALSE(records->at(int(qftbx::SpecificationType::SensorNoise)).used);

    //3.5 dB is what was typed, and linear is what the record keeps.
    EXPECT_NEAR(records->at(int(qftbx::SpecificationType::Stability)).height,
                qftbx::dbToLinear(3.5), 1e-12);
}

TEST_F(GuiSmoke, ASpecificationAppliesAtTheFrequenciesThatAreTicked)
{
    //The band is not typed: it is the design frequencies, ticked. And a
    //frequency in the middle can be taken out, which a pair of numbers
    //cannot say.
    const std::vector<double> frequencies{0.1, 1.0, 10.0, 100.0};
    SpecificationsForm form(&frequencies);

    QComboBox * typeCombo = child<QComboBox>(&form, "typeCombo");
    typeCombo->setCurrentIndex(int(qftbx::SpecificationType::ControlEffort));

    qftbx::FrequencyLegend * ticks = form.findChild<qftbx::FrequencyLegend *>();
    ASSERT_NE(ticks, nullptr);
    ASSERT_EQ(ticks->rowCount(), 4) << "one tick per design frequency";
    for (int i = 0; i < ticks->rowCount(); ++i) {
        EXPECT_TRUE(ticks->isRowChecked(i)) << "a new specification applies everywhere";
    }

    //Out with the second one, and with the last.
    ticks->setRowChecked(1, false);
    ticks->setRowChecked(3, false);

    check(&form, "constantRadio");
    check(&form, "linearRadio");
    type(&form, "magnitudeEdit", "2");

    press(&form, "addButton");
    press(&form, "addButton");
    press(&form, "okButton");

    ASSERT_TRUE(form.wasAccepted()) << complaintOf(&form).toStdString();

    const std::optional<qftbx::SpecificationRecords> records = form.takeSpecifications();
    ASSERT_TRUE(records.has_value());

    const qftbx::SpecificationRecord & effort =
            records->at(int(qftbx::SpecificationType::ControlEffort));

    //The band runs from the first tick to the last, and the hole in the
    //middle travels as an exception.
    EXPECT_DOUBLE_EQ(effort.omegaStart, 0.1);
    EXPECT_DOUBLE_EQ(effort.omegaEnd, 10.0);
    ASSERT_EQ(effort.skipped.size(), 1u);
    EXPECT_DOUBLE_EQ(effort.skipped.front(), 1.0);

    //Which is what the specification the engines see answers.
    const qftbx::Specification specification =
            qftbx::toSpecification(effort, qftbx::SpecificationType::ControlEffort);
    EXPECT_TRUE(specification.appliesAt(0.1));
    EXPECT_FALSE(specification.appliesAt(1.0)) << "the frequency taken out is out";
    EXPECT_TRUE(specification.appliesAt(10.0));
    EXPECT_FALSE(specification.appliesAt(100.0)) << "and so is everything past the band";
}

TEST_F(GuiSmoke, ASpecificationIsRemovedFromTheListItIsOn)
{
    const std::vector<double> frequencies{0.1, 1.0, 10.0};
    SpecificationsForm form(&frequencies);

    QComboBox * typeCombo = child<QComboBox>(&form, "typeCombo");
    QTableWidget * table = child<QTableWidget>(&form, "specificationsTable");

    typeCombo->setCurrentIndex(int(qftbx::SpecificationType::ControlEffort));
    check(&form, "constantRadio");
    check(&form, "linearRadio");
    type(&form, "magnitudeEdit", "2");
    press(&form, "addButton");
    press(&form, "addButton");

    ASSERT_EQ(table->rowCount(), 1);

    table->setCurrentCell(0, 0);
    press(&form, "removeButton");

    EXPECT_EQ(table->rowCount(), 0);

    press(&form, "okButton");
    EXPECT_FALSE(form.wasAccepted()) << "there is nothing to apply";
}

TEST_F(GuiSmoke, FrequenciesDialogRefusesAnEmptySetInsteadOfDying)
{
    //Pressing OK on a freshly opened dialog ABORTED the application: the
    //Omega constructor refuses an empty frequency set by throwing, and that
    //exception escaped this slot into Qt's event loop.
    FrequenciesForm dialog;

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
    FrequenciesForm dialog;

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
    FrequenciesForm dialog;

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

TEST_F(GuiSmoke, ControllerFormBuildsTheControllerStructure)
{
    ControllerForm dialog;

    check(&dialog, "zpkRadio");
    type(&dialog, "numeratorEdit", "1");
    type(&dialog, "denominatorEdit", "100");
    //The controller's gain is a SEARCH RANGE, not a value.
    type(&dialog, "gainStart", "1");
    type(&dialog, "gainEnd", "1000");

    press(&dialog, "okButton");
    EXPECT_FALSE(dialog.wasAccepted()) << "verifying is not applying";
    press(&dialog, "okButton");

    ASSERT_TRUE(dialog.wasAccepted()) << "the form rejected valid data: "
                                      << complaintOf(&dialog).toStdString();

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

TEST_F(GuiSmoke, ControllerFormRejectsAnInvalidNumerator)
{
    //readTables() keeps only the LAST of its three parse results, so a
    //malformed numerator or denominator was overwritten by a gain range that
    //parsed. The plant dialog rejects the same input.
    ControllerForm dialog;

    check(&dialog, "zpkRadio");
    type(&dialog, "numeratorEdit", "1*/");
    type(&dialog, "denominatorEdit", "100");
    type(&dialog, "gainStart", "1");
    type(&dialog, "gainEnd", "1000");

    verifyAndApply(&dialog);

    EXPECT_FALSE(dialog.wasAccepted())
        << "a malformed numerator was accepted";
    EXPECT_EQ(dialog.takeControllerStructure(), nullptr);
    EXPECT_FALSE(complaintOf(&dialog).isEmpty()) << "the rejection must be said";
    EXPECT_TRUE(isMarked(&dialog, "numeratorEdit"));
}

TEST_F(GuiSmoke, SpecificationsDialogNeedsTheFrequenciesFirst)
{
    //The main window gates the step order, but the dialog used to reach
    //first()/last() on a null frequency vector and take the application
    //down; it says so now.
    EXPECT_THROW(SpecificationsForm dialog(nullptr), qftbx::InvalidInput);

    const std::vector<double> empty;
    EXPECT_THROW(SpecificationsForm dialog(&empty), qftbx::InvalidInput);
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
    Omega omega(-2.0, 2.0, 100, qftbx::logspace(-2.0, 2.0, 100), Omega::LogSpace);

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
    BoundaryGridForm dialog;

    //The DEFAULT window must be the full [-360, 0]: loop shaping refuses a
    //narrower one, and the reader of the phase buckets is scaled by it.
    ASSERT_NE(child<QPushButton>(&dialog, "okButton"), nullptr);

    type(&dialog, "phasePoints", "361");
    type(&dialog, "magnitudePoints", "121");

    press(&dialog, "okButton");

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
    BoundaryGridForm dialog;

    type(&dialog, "phaseStart", "0");
    type(&dialog, "phaseEnd", "-360");

    press(&dialog, "okButton");

    EXPECT_FALSE(dialog.wasAccepted()) << "an inverted phase range was accepted";
}

TEST_F(GuiSmoke, TemplatesDialogBuildsOneEpsilonPerFrequency)
{
    TemplatesForm dialog;

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

    ASSERT_TRUE(dialog.wasAccepted()) << "the form rejected valid data";

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

TEST_F(GuiSmoke, TemplatesDialogOpensWithTheProposedEpsilon)
{
    //The field opens filled with the least epsilon each template asks for,
    //computed from the grids as the dialog holds them on launch, so that OK
    //alone is a complete answer, shown as the engine gives it; the Propose
    //button asks again with the plane as chosen.
    TemplatesForm dialog;

    std::vector<Parameter> numerator{Parameter(1.0)};
    std::vector<Parameter> denominator{
        Parameter("a", qftbx::Range(1.0, 5.0), 5.0)};
    PolynomialForm plant("templates", numerator, denominator,
                         Parameter(1.0), Parameter(0.0));

    int calls = 0;
    qftbx::EpsilonMetric lastMetric;
    qftbx::ParameterGrids lastGrids;
    dialog.setEpsilonProposer([&](const qftbx::ParameterGrids & grids, qftbx::EpsilonMetric metric) {
        ++calls;
        lastGrids = grids;
        lastMetric = metric;
        std::vector<qftbx::TemplateEngine::EpsilonProposal> proposals(2);
        proposals[0].connected = 2.7731;
        proposals[0].epsilon = 2.78;
        proposals[0].diameter = 100.0;
        proposals[0].closes = true;
        proposals[1].connected = 0.012345;
        proposals[1].epsilon = 0.0124;
        proposals[1].diameter = 1.0;
        proposals[1].closes = true;
        return proposals;
    });
    dialog.setDefaultPointCount(3);
    dialog.setEpsilonMetric(qftbx::EpsilonMetric{});

    dialog.launch(&plant, 2);

    EXPECT_EQ(calls, 1) << "launching did not ask for a proposal";
    EXPECT_EQ(lastGrids.count("a"), 1u) << "the proposal was not made over the dialog's grids";
    EXPECT_EQ(lastGrids.at("a").size(), 3u) << "the default point count was not used";
    EXPECT_EQ(lastMetric.metric, qftbx::HullMetric::ComplexPlane);
    EXPECT_EQ(child<QLineEdit>(&dialog, "epsilonEdit")->text(), QString("2.78 0.0124"));

    //Propose again in the other plane.
    child<QComboBox>(&dialog, "metricCombo")->setCurrentIndex(0);
    press(&dialog, "proposeButton");
    EXPECT_EQ(calls, 2);
    EXPECT_EQ(lastMetric.metric, qftbx::HullMetric::Nichols);

    //The border sweep needs two uncertain parameters; this plant has one,
    //so the box is offered disabled and reads as off whatever it holds.
    EXPECT_FALSE(child<QCheckBox>(&dialog, "borderSweepCheck")->isEnabled());
    child<QCheckBox>(&dialog, "borderSweepCheck")->setChecked(true);
    EXPECT_FALSE(dialog.borderSweep());

    //The contour is the walk by default; the alpha-shape on request.
    EXPECT_FALSE(dialog.alphaShapeContour());
    child<QComboBox>(&dialog, "contourCombo")->setCurrentIndex(1);
    EXPECT_TRUE(dialog.alphaShapeContour());

    //Where a contour does not close, the whole template stands in by default;
    //the user can ask for an error instead.
    EXPECT_TRUE(dialog.wholeTemplateIfNoContour());
    child<QCheckBox>(&dialog, "wholeTemplateCheck")->setChecked(false);
    EXPECT_FALSE(dialog.wholeTemplateIfNoContour());

    //And OK accepts the filled-in field as it stands.
    check(&dialog, "nicholsRadio");
    press(&dialog, "okButton");
    ASSERT_TRUE(dialog.wasAccepted()) << "the dialog rejected its own proposal";
    const std::vector<double> epsilon = dialog.takeEpsilon();
    ASSERT_EQ(epsilon.size(), 2u);
    EXPECT_DOUBLE_EQ(epsilon[0], 2.78);
    EXPECT_DOUBLE_EQ(epsilon[1], 0.0124);
}

TEST_F(GuiSmoke, TemplatesDialogWithoutAProposerOpensEmpty)
{
    TemplatesForm dialog;

    std::vector<Parameter> numerator{Parameter(1.0)};
    std::vector<Parameter> denominator{
        Parameter("a", qftbx::Range(1.0, 5.0), 5.0)};
    PolynomialForm plant("templates", numerator, denominator,
                         Parameter(1.0), Parameter(0.0));

    dialog.launch(&plant, 2);

    EXPECT_TRUE(child<QLineEdit>(&dialog, "epsilonEdit")->text().isEmpty());
    EXPECT_TRUE(dialog.proposals().empty());
}

TEST_F(GuiSmoke, LoopShapingDialogCarriesTheChosenAlgorithm)
{
    LoopShapingForm dialog;

    type(&dialog, "epsilonEdit", "0.01");
    type(&dialog, "startEdit", "0.1");
    type(&dialog, "endEdit", "100");
    type(&dialog, "pointCountEdit", "200");
    check(&dialog, "mrRadio");

    press(&dialog, "okButton");

    ASSERT_TRUE(dialog.wasAccepted()) << "the form rejected valid data";

    EXPECT_EQ(dialog.algorithmValue(), qftbx::mr);
    EXPECT_DOUBLE_EQ(dialog.epsilonValue(), 0.01);
    EXPECT_DOUBLE_EQ(dialog.range().min, 0.1);
    EXPECT_DOUBLE_EQ(dialog.range().max, 100.0);
    EXPECT_DOUBLE_EQ(dialog.pointCountValue(), 200.0);
}

TEST_F(GuiSmoke, UncertaintyPanelBuildsAnUncertainParameter)
{
    //This is what turns a named coefficient into an uncertain Parameter, so
    //it feeds the uncertainty of every plant and controller. It is driven
    //the way the plant dialog drives it: the three parallel tables, one row
    //per uncertain name.
    UncertaintyPanel dialog;

    const CoefficientTable valueTable{{"1"}, {"a"}};
    const CoefficientTable expressionTable{{"1"}, {"a"}};
    const UncertainTable uncertainTable{{false}, {true}};

    ASSERT_TRUE(dialog.launch(valueTable, expressionTable, uncertainTable, false));

    //One uncertain name, so one generated row: minimum, nominal, maximum.
    type(&dialog, "rangeMinimum", "1");
    type(&dialog, "rangeMaximum", "5");
    type(&dialog, "rangeNominal", "3");

    type(&dialog, "rangeGainStart", "2");
    type(&dialog, "rangeGainEnd", "8");
    type(&dialog, "rangeDelayStart", "0");
    type(&dialog, "rangeDelayEnd", "0");

    press(&dialog, "applyButton");

    ASSERT_TRUE(dialog.wasAccepted()) << "the panel rejected valid data";

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

TEST_F(GuiSmoke, UncertaintyPanelRejectsAnEmptyRange)
{
    //An empty range used to be read as a null sentinel; it must be reported
    //and refused, not turned into a parameter.
    UncertaintyPanel dialog;

    const CoefficientTable valueTable{{"1"}, {"a"}};
    const CoefficientTable expressionTable{{"1"}, {"a"}};
    const UncertainTable uncertainTable{{false}, {true}};

    ASSERT_TRUE(dialog.launch(valueTable, expressionTable, uncertainTable, false));

    //The row is left blank on purpose.
    type(&dialog, "rangeGainStart", "2");
    type(&dialog, "rangeGainEnd", "8");
    type(&dialog, "rangeDelayStart", "0");
    type(&dialog, "rangeDelayEnd", "0");

    press(&dialog, "applyButton");

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

//A viewer has to grow with its window: a form whose widgets are placed at
//absolute coordinates keeps them where they were put, whatever the user does
//to the window, and the plot then stays the size it was drawn at in
//Designer. The check is the behaviour and not the mechanism - resize the
//viewer and see whether the canvas followed - so it holds however the form
//is laid out.
template <typename Viewer>
void expectThePlotGrowsWithTheWindow(const char * what)
{
    Viewer viewer;
    viewer.resize(700, 500);
    viewer.show();
    QCoreApplication::processEvents();

    QCustomPlot * plot = viewer.template findChild<QCustomPlot *>("plot");
    ASSERT_NE(plot, nullptr) << what << ": no plot named \"plot\"";
    const QSize before = plot->size();

    viewer.resize(1300, 900);
    QCoreApplication::processEvents();

    EXPECT_GT(plot->width(), before.width())
        << what << ": the window grew by 600 px and the plot stayed at "
        << plot->width() << " (the form has no layout)";
    EXPECT_GT(plot->height(), before.height())
        << what << ": the window grew by 400 px and the plot stayed at " << plot->height();
}

TEST_F(GuiSmoke, EveryViewerGrowsWithItsWindow)
{
    expectThePlotGrowsWithTheWindow<TemplateViewer>("template viewer");
    expectThePlotGrowsWithTheWindow<BoundaryViewer>("boundary viewer");
    expectThePlotGrowsWithTheWindow<BoundaryUnionViewer>("boundary union viewer");
    expectThePlotGrowsWithTheWindow<LoopBoundariesViewer>("loop boundaries viewer");
    expectThePlotGrowsWithTheWindow<LoopShapingViewer>("loop shaping viewer");

    //Bode draws two canvases, one over the other, and both have to follow.
    BodeViewer bode;
    bode.resize(700, 500);
    bode.show();
    QCoreApplication::processEvents();

    QCustomPlot * magnitude = bode.findChild<QCustomPlot *>("magnitudePlot");
    QCustomPlot * phase = bode.findChild<QCustomPlot *>("phasePlot");
    ASSERT_NE(magnitude, nullptr);
    ASSERT_NE(phase, nullptr);
    const int wasWide = magnitude->width();
    const int wasTall = magnitude->height() + phase->height();

    bode.resize(1300, 900);
    QCoreApplication::processEvents();

    EXPECT_GT(magnitude->width(), wasWide) << "the Bode magnitude did not widen";
    EXPECT_GT(magnitude->height() + phase->height(), wasTall) << "the Bode canvases did not grow";
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
// The steps used to be modal dialogs, so a test that pressed a step button
// hung there for ever and the window could only be built, never driven. They
// are panels in docks now: pressing the button brings the phase up, the test
// fills the panel it finds in the window and presses its button, and the
// window does what a user's acceptance makes it do.
//
// This is the net the rest of the window work needs: the seven bool flags,
// the repeated teardown and the invalidation ladder are about to be replaced,
// and until now there was nothing watching.

//The panel of a phase, as the window built it.
template <typename Panel>
Panel * panelIn(QWidget * window)
{
    return window->findChild<Panel *>();
}

//The user of the plant panel: a plant that needs no uncertainty.
void fillPlant(PlantForm * plant, const QString & name)
{
    type(plant, "nameEdit", name);
    check(plant, "transferFunctionRadio");
    check(plant, "zerosPolesRadio");
    check(plant, "zpkRadio");
    type(plant, "numeratorEdit", "2");
    type(plant, "denominatorEdit", "5 30");
    type(plant, "gainEdit", "3");
    type(plant, "delayEdit", "0");
    verifyAndApply(plant);
}

TEST_F(GuiSmoke, PressingThePlantStepPublishesAPlantAndOpensTheNextSteps)
{
    MainWindow window;

    QPushButton * plantButton = child<QPushButton>(&window, "plantButton");
    ASSERT_NE(plantButton, nullptr);
    plantButton->click();

    PlantForm * plant = panelIn<PlantForm>(&window);
    ASSERT_NE(plant, nullptr) << "pressing the step must bring its panel up";
    fillPlant(plant, "driven");

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

TEST_F(GuiSmoke, APanelNobodyAcceptsLeavesTheStepUndone)
{
    MainWindow window;

    //A user who brings the phase up, looks at it and does nothing: the step
    //is not done until its button is pressed.
    QPushButton * plantButton = child<QPushButton>(&window, "plantButton");
    ASSERT_NE(plantButton, nullptr);
    plantButton->click();

    ASSERT_NE(panelIn<PlantForm>(&window), nullptr);

    QProgressBar * progress = child<QProgressBar>(&window, "progressBar");
    ASSERT_NE(progress, nullptr);
    EXPECT_EQ(progress->value(), 0)
        << "a step nobody accepted must not count as done";
}


//The user of the frequency panel: four frequencies, typed.
void fillFrequencies(FrequenciesForm * frequencies)
{
    QComboBox * mode = child<QComboBox>(frequencies, "modeStack");
    if (mode != nullptr) {
        mode->setCurrentIndex(0);
    }
    type(frequencies, "manualValues", "0.1 1 10 100");
    press(frequencies, "okButton");
}

TEST_F(GuiSmoke, WalkingTwoStepsOpensTheThirdAndNoFurther)
{
    MainWindow window;

    QPushButton * plantButton = child<QPushButton>(&window, "plantButton");
    QPushButton * frequenciesButton = child<QPushButton>(&window, "frequenciesButton");
    ASSERT_NE(plantButton, nullptr);
    ASSERT_NE(frequenciesButton, nullptr);

    plantButton->click();
    fillPlant(panelIn<PlantForm>(&window), "walked");

    frequenciesButton->click();
    fillFrequencies(panelIn<FrequenciesForm>(&window));

    QProgressBar * progress = child<QProgressBar>(&window, "progressBar");
    ASSERT_NE(progress, nullptr);
    EXPECT_EQ(progress->value(), 2);

    //The templates need both, and now have both.
    QPushButton * templates = child<QPushButton>(&window, "templatesButton");
    ASSERT_NE(templates, nullptr);
    EXPECT_TRUE(templates->isEnabled());

    //The specifications need the frequencies, and the Bode view needs both.
    EXPECT_TRUE(child<QPushButton>(&window, "specificationsButton")->isEnabled());

    //And the boundaries need the templates, which nobody has computed.
    EXPECT_FALSE(child<QPushButton>(&window, "boundariesButton")->isEnabled());
}

TEST_F(GuiSmoke, APanelRefusesToPublishWhatTheProjectHasTakenAwayFromIt)
{
    //The panels stay open now, so what they were handed can be destroyed
    //under them: the specifications read the frequency values of the
    //project's Omega, and entering a new set frees the old one. The window
    //hands them the new one, and a panel with none refuses rather than
    //reading a vector that is gone.
    const std::vector<double> frequencies{0.1, 1.0, 10.0};
    SpecificationsForm specifications(&frequencies);

    specifications.setFrequencies(nullptr);
    child<QComboBox>(&specifications, "typeCombo")
        ->setCurrentIndex(int(qftbx::SpecificationType::Stability));
    check(&specifications, "constantRadio");
    type(&specifications, "magnitudeEdit", "1.2");
    press(&specifications, "addButton");
    press(&specifications, "okButton");

    EXPECT_FALSE(specifications.wasAccepted())
        << "a panel with no frequencies must not publish a band read from them";

    //The same for the grids of the templates, which describe the parameters
    //of a plant the project owns.
    TemplatesForm templates;

    std::vector<Parameter> numerator{Parameter(1.0)};
    std::vector<Parameter> denominator{Parameter("a", qftbx::Range(1.0, 5.0), 5.0)};
    PolynomialForm plant("templates", numerator, denominator, Parameter(1.0), Parameter(0.0));

    templates.launch(&plant, 3);
    EXPECT_EQ(templates.shownPlant(), &plant);

    templates.forgetPlant();
    type(&templates, "epsilonEdit", "0.05");
    check(&templates, "linspaceRadio");
    type(&templates, "globalPointCount", "3");
    check(&templates, "allVariablesRadio");
    press(&templates, "okButton");

    EXPECT_FALSE(templates.wasAccepted())
        << "grids with no plant behind them must not be published";
}

TEST_F(GuiSmoke, OpeningAProjectPutsItsCardsOnTheCanvasFolded)
{
    //What the toolbox is for: the results of a project, all of them, on the
    //screen at once. Every phase is a card on the canvas - what it was
    //asked for and what came out of it, together - and a project that is
    //opened shows them with their forms folded away, so that what is on
    //screen is the diagrams.
    MainWindow window;
    window.setFileChooser([](bool) {
        return QString(QFTBX_TEST_DATA_DIR "/planta1.qft");
    });
    window.findChild<QAction *>("actionOpen")->trigger();

    for (const char * name : {"plantCard", "templatesCard", "boundariesCard", "loopShapingCard"}) {
        PhaseCard * card = window.findChild<PhaseCard *>(name);
        ASSERT_NE(card, nullptr) << name << " is not on the canvas";
        EXPECT_FALSE(card->isFormShown()) << name << " opened with its form in front of its diagram";
    }

    //They are laid out one after another on the canvas, not on top of each
    //other and not in tabs.
    QWidget * canvas = window.findChild<QWidget *>("canvasContent");
    ASSERT_NE(canvas, nullptr);
    EXPECT_GE(canvas->layout()->count(), 4) << "the cards are not on the canvas";

    //And pressing a step unfolds the form of that phase.
    child<QPushButton>(&window, "plantButton")->click();
    EXPECT_TRUE(window.findChild<PhaseCard *>("plantCard")->isFormShown());
}

TEST_F(GuiSmoke, TheCanvasWrapsAndScrollsInsteadOfSqueezing)
{
    //The rule of the canvas: a card keeps the size it asks for. What does
    //not fit in a row goes to the row below, and what does not fit
    //downwards is scrolled to - nothing is ever shrunk to make room.
    MainWindow window;
    window.setFileChooser([](bool) {
        return QString(QFTBX_TEST_DATA_DIR "/planta1.qft");
    });
    window.findChild<QAction *>("actionOpen")->trigger();

    window.resize(1280, 860);
    window.show();
    QCoreApplication::processEvents();

    QScrollArea * canvas = window.findChild<QScrollArea *>("canvas");
    ASSERT_NE(canvas, nullptr);
    QWidget * content = window.findChild<QWidget *>("canvasContent");
    ASSERT_NE(content, nullptr);

    //Seven phases of 560 by 360 do not fit on one screen, so the canvas is
    //taller than what is on show and there is somewhere to scroll to.
    EXPECT_GT(content->height(), canvas->viewport()->height())
        << "the cards were squeezed into the viewport instead of wrapping below it";

    //And every card is at least as wide as a diagram worth looking at.
    for (const char * name : {"plantCard", "templatesCard", "boundariesCard", "loopShapingCard"}) {
        PhaseCard * card = window.findChild<PhaseCard *>(name);
        ASSERT_NE(card, nullptr);
        EXPECT_GE(card->width(), 500) << name << " came out too narrow to read";
    }
}

TEST_F(GuiSmoke, ACardGrowsWhenItsFormIsUnfolded)
{
    //The point of the canvas: unfolding the form of a phase makes its card
    //bigger, and the canvas moves the ones beside it out of the way instead
    //of the card taking its space from its neighbours.
    MainWindow window;
    window.setFileChooser([](bool) {
        return QString(QFTBX_TEST_DATA_DIR "/planta1.qft");
    });
    window.findChild<QAction *>("actionOpen")->trigger();

    PhaseCard * card = window.findChild<PhaseCard *>("templatesCard");
    ASSERT_NE(card, nullptr);

    const QSize folded = card->sizeHint();
    card->showForm(true);
    const QSize open = card->sizeHint();

    EXPECT_GT(open.height(), folded.height()) << "the form has to make the card taller";
    EXPECT_EQ(open.width(), folded.width())
        << "unfolding a form must not widen the card: the row would break up every time";

    //And every card is one of the two sizes, never its own: a wall of
    //panels of one height reads as a wall, each at the size of its own form
    //reads as a pile.
    for (const char * name : {"plantCard", "templatesCard", "boundariesCard", "loopShapingCard"}) {
        PhaseCard * other = window.findChild<PhaseCard *>(name);
        ASSERT_NE(other, nullptr);
        EXPECT_EQ(other->sizeHint(), other->isFormShown() ? open : folded)
            << name << " asks for a size of its own";
    }

    //The form-only phases are as tall as a folded card, and one column
    //wide - two only for the specifications, whose form was drawn wider
    //than a column and asks for the same width always, folded or not.
    for (const char * name : {"frequenciesCard", "controllerCard"}) {
        PhaseCard * other = window.findChild<PhaseCard *>(name);
        ASSERT_NE(other, nullptr);
        EXPECT_EQ(other->sizeHint().height(), folded.height()) << name;
        EXPECT_EQ(other->sizeHint().width(), folded.width()) << name;
    }

    PhaseCard * specifications = window.findChild<PhaseCard *>("specificationsCard");
    ASSERT_NE(specifications, nullptr);
    EXPECT_EQ(specifications->sizeHint().height(), folded.height());
    EXPECT_GT(specifications->sizeHint().width(), folded.width());
}

TEST_F(GuiSmoke, ACardDraggedByItsBarChangesPlaces)
{
    //The order of the canvas is the user's: a card taken by its bar and
    //dragged over another changes places with it, and the layout opens the
    //hole by itself while the drag is still going on.
    MainWindow window;
    window.setFileChooser([](bool) {
        return QString(QFTBX_TEST_DATA_DIR "/planta1.qft");
    });
    window.findChild<QAction *>("actionOpen")->trigger();
    window.resize(1280, 860);
    window.show();
    QCoreApplication::processEvents();

    QWidget * canvas = window.findChild<QWidget *>("canvasContent");
    ASSERT_NE(canvas, nullptr);
    QLayout * layout = canvas->layout();
    ASSERT_NE(layout, nullptr);

    PhaseCard * first = window.findChild<PhaseCard *>("plantCard");
    PhaseCard * last = window.findChild<PhaseCard *>("loopShapingCard");
    ASSERT_NE(first, nullptr);
    ASSERT_NE(last, nullptr);
    ASSERT_EQ(layout->indexOf(first), 0);

    const int wasLast = layout->indexOf(last);
    ASSERT_GT(wasLast, 0);

    //Taken by its bar and dropped on the first card.
    QWidget * bar = last->findChild<QWidget *>("loopShapingCardBar");
    ASSERT_NE(bar, nullptr);

    const QPoint onTheFirst = canvas->mapToGlobal(first->geometry().topLeft() + QPoint(10, 10));
    QMouseEvent press(QEvent::MouseButtonPress, QPointF(5, 5), QPointF(5, 5),
                      onTheFirst, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QMouseEvent move(QEvent::MouseMove, QPointF(5, 5), QPointF(5, 5),
                     onTheFirst, Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
    QMouseEvent release(QEvent::MouseButtonRelease, QPointF(5, 5), QPointF(5, 5),
                        onTheFirst, Qt::LeftButton, Qt::NoButton, Qt::NoModifier);

    QCoreApplication::sendEvent(bar, &press);
    QCoreApplication::sendEvent(bar, &move);
    QCoreApplication::sendEvent(bar, &release);

    EXPECT_EQ(layout->indexOf(last), 0) << "the card did not move to where it was dropped";
    EXPECT_EQ(layout->indexOf(first), 1) << "the one it was dropped on did not move over";

    //The bar is the handle and says so with the hand; the buttons on it are
    //not, and a hand over something you only press says the wrong thing.
    EXPECT_EQ(bar->cursor().shape(), Qt::OpenHandCursor);
    for (const char * button : {"loopShapingCardFold", "loopShapingCardNarrower",
                                "loopShapingCardWider"}) {
        QWidget * pressed = last->findChild<QWidget *>(button);
        ASSERT_NE(pressed, nullptr) << button;
        EXPECT_EQ(pressed->cursor().shape(), Qt::ArrowCursor) << button;
    }
}

TEST_F(GuiSmoke, TheCanvasComesBackAsTheLastSessionLeftIt)
{
    //The order and the sizes are the user's, so they outlive the session:
    //the window writes them into the settings on the way out and puts every
    //card back where it was on the way in.
    qftbx::Settings settings;
    settings.interface.canvas = "loopShapingCard:2 plantCard:1";

    MainWindow window(settings);
    window.setFileChooser([](bool) {
        return QString(QFTBX_TEST_DATA_DIR "/planta1.qft");
    });
    window.findChild<QAction *>("actionOpen")->trigger();

    QWidget * canvas = window.findChild<QWidget *>("canvasContent");
    ASSERT_NE(canvas, nullptr);

    PhaseCard * loop = window.findChild<PhaseCard *>("loopShapingCard");
    PhaseCard * plant = window.findChild<PhaseCard *>("plantCard");
    ASSERT_NE(loop, nullptr);
    ASSERT_NE(plant, nullptr);

    EXPECT_EQ(canvas->layout()->indexOf(loop), 0) << "the design was not put back in front";
    EXPECT_EQ(canvas->layout()->indexOf(plant), 1);
    EXPECT_EQ(loop->span(), 2) << "the size it was given was not put back";
    EXPECT_EQ(plant->span(), 1);
}

TEST_F(GuiSmoke, TheWindowOpensAtTheSizeItWasLeft)
{
    qftbx::Settings settings;
    settings.interface.window = "1100 700";

    MainWindow window(settings);
    EXPECT_EQ(window.size(), QSize(1100, 700));

    //And a line this build cannot read is no reason to refuse to start.
    qftbx::Settings broken;
    broken.interface.window = "as wide as you like";
    MainWindow other(broken);
    EXPECT_GT(other.width(), 0);
}

TEST_F(GuiSmoke, TheThemeDressesTheWindowAndItsDiagrams)
{
    //The look is flat and square, and it is one style sheet over three
    //palettes: the light one, the dark one and the machine's own. What has
    //to be checked is that choosing one reaches everything - including the
    //diagrams, which are drawn by QCustomPlot and not by the style sheet.
    qftbx::Settings settings;
    settings.interface.theme = "light";

    MainWindow window(settings);
    window.setFileChooser([](bool) {
        return QString(QFTBX_TEST_DATA_DIR "/planta1.qft");
    });
    window.findChild<QAction *>("actionOpen")->trigger();

    QCustomPlot * plot = window.findChild<QCustomPlot *>();
    ASSERT_NE(plot, nullptr);

    QAction * dark = window.findChild<QAction *>("actionTheme_dark");
    ASSERT_NE(dark, nullptr) << "the View menu has no dark theme";
    dark->trigger();

    EXPECT_LT(QApplication::palette().color(QPalette::Window).lightness(), 100)
        << "the dark theme did not reach the application";
    EXPECT_GT(plot->xAxis->tickLabelColor().lightness(), 150)
        << "the diagram kept its black axes in the middle of a dark window";

    QAction * light = window.findChild<QAction *>("actionTheme_light");
    ASSERT_NE(light, nullptr);
    light->trigger();

    EXPECT_GT(QApplication::palette().color(QPalette::Window).lightness(), 200);
    EXPECT_LT(plot->xAxis->tickLabelColor().lightness(), 100);

    //And the accent is the blue of the icon, in both.
    EXPECT_NEAR(QApplication::palette().color(QPalette::Highlight).hue(), 204, 12);

    qftbx::applyTheme(qftbx::kSystemTheme);
}

TEST_F(GuiSmoke, TheFiguresAndTheIconAreInTheBuild)
{
    //Resources compiled into a STATIC library are dropped by the linker
    //unless the target owns them: the figures of the plant and the
    //specifications came up blank and the application had no icon, and
    //nothing said so - a missing resource is an empty pixmap.
    for (const char * size : {"16", "32", "64", "128", "256"}) {
        const QPixmap icon(QString(":/icons/qftbx_%1.png").arg(size));
        EXPECT_FALSE(icon.isNull()) << "the icon of " << size << " pixels is not in the build";
    }

    //The three that are still shown: the families a plant or a controller
    //can be written in. The six of the specifications are gone - what a
    //specification requires is DRAWN now, from the definition the program
    //computes, so it says the bound that was typed and not a W with a
    //subscript.
    for (const char * figure : {"copol", "kgan", "knogan"}) {
        const QPixmap picture(QString(":/figures/%1.png").arg(figure));
        EXPECT_FALSE(picture.isNull()) << "the figure " << figure << " is not in the build";
    }

    //And a form shows the one of the family chosen.
    PlantForm plant;
    check(&plant, "transferFunctionRadio");
    check(&plant, "zerosPolesRadio");
    check(&plant, "zpkRadio");
    QLabel * image = child<QLabel>(&plant, "familyImage");
    ASSERT_NE(image, nullptr);
    EXPECT_FALSE(image->pixmap().isNull()) << "the plant form has no figure in it";
}

//Waits for the phase that is computing, spinning the loop as the
//application does: the result arrives through a queued call.
void waitForComputation(MainWindow & window)
{
    for (int spin = 0; spin < 2000 && window.isComputing(); ++spin) {
        QCoreApplication::processEvents();
        QThread::msleep(5);
    }
    QCoreApplication::processEvents();
}

TEST_F(GuiSmoke, AComputationRunsOnAWorkerAndTheWindowStaysAlive)
{
    //The window used to freeze for as long as a computation took - tens of
    //minutes on a real problem - with an hourglass over it and no way out
    //but killing the process. Now the phase says it is working, its form is
    //not to be touched while its numbers are in use, and the rest of the
    //window answers.
    MainWindow window;

    child<QPushButton>(&window, "plantButton")->click();
    fillPlant(panelIn<PlantForm>(&window), "worked");
    child<QPushButton>(&window, "frequenciesButton")->click();
    fillFrequencies(panelIn<FrequenciesForm>(&window));

    child<QPushButton>(&window, "templatesButton")->click();
    TemplatesForm * templates = panelIn<TemplatesForm>(&window);
    ASSERT_NE(templates, nullptr);

    type(templates, "epsilonEdit", "0.5");
    check(templates, "linspaceRadio");
    type(templates, "globalPointCount", "3");
    check(templates, "allVariablesRadio");
    check(templates, "nicholsRadio");
    press(templates, "okButton");

    PhaseCard * card = window.findChild<PhaseCard *>("templatesCard");
    ASSERT_NE(card, nullptr);

    //It says so while it runs - and the form is disabled, because its
    //numbers are what is being computed.
    if (window.isComputing()) {
        EXPECT_TRUE(card->isBusy());
        EXPECT_NE(window.findChild<QToolButton *>("templatesCardCancel"), nullptr);
    }

    waitForComputation(window);

    EXPECT_FALSE(card->isBusy()) << "the card stayed busy after the run finished";

    QProgressBar * progress = child<QProgressBar>(&window, "progressBar");
    ASSERT_NE(progress, nullptr);
    //Plant, frequencies and templates: three of the seven steps.
    EXPECT_EQ(progress->value(), 3) << "the templates did not reach the project: "
                                    << m_reported.join(" | ").toStdString();

    //And what it computed is on the screen.
    TemplateViewer * viewer = window.findChild<TemplateViewer *>();
    ASSERT_NE(viewer, nullptr);
    QCustomPlot * plot = child<QCustomPlot>(viewer, "plot");
    ASSERT_NE(plot, nullptr);
    EXPECT_GT(plot->graphCount(), 0) << "the templates were computed and not drawn";
}

TEST_F(GuiSmoke, ACancelledComputationLeavesTheProjectAsItWas)
{
    //Giving up has to leave nothing behind: half a sweep is not a set of
    //templates, and a project that kept one would carry a result nobody
    //computed.
    MainWindow window;

    child<QPushButton>(&window, "plantButton")->click();
    fillPlant(panelIn<PlantForm>(&window), "cancelled");
    child<QPushButton>(&window, "frequenciesButton")->click();
    fillFrequencies(panelIn<FrequenciesForm>(&window));

    child<QPushButton>(&window, "templatesButton")->click();
    TemplatesForm * templates = panelIn<TemplatesForm>(&window);
    ASSERT_NE(templates, nullptr);

    //A sweep big enough to still be running when the cancel arrives.
    type(templates, "epsilonEdit", "0.5");
    check(templates, "linspaceRadio");
    type(templates, "globalPointCount", "400");
    check(templates, "allVariablesRadio");
    check(templates, "nicholsRadio");
    press(templates, "okButton");

    QToolButton * cancel = window.findChild<QToolButton *>("templatesCardCancel");
    ASSERT_NE(cancel, nullptr);
    cancel->click();

    waitForComputation(window);

    QProgressBar * progress = child<QProgressBar>(&window, "progressBar");
    ASSERT_NE(progress, nullptr);
    EXPECT_EQ(progress->value(), 2) << "a cancelled sweep left templates behind";
    EXPECT_TRUE(m_reported.isEmpty())
        << "giving up is not an error: " << m_reported.join(" | ").toStdString();
}

TEST_F(GuiSmoke, ZZPlantForm)
{
    const QString out = qEnvironmentVariable("QFTBX_RENDER_DIR");
    if (out.isEmpty()) {
        GTEST_SKIP();
    }

    qftbx::applyTheme(qftbx::kLightTheme);

    const QSize card(660, 360);

    PlantForm fresh;
    fresh.resize(card);
    fresh.grab().save(out + "/planta-vacia.png");

    PlantForm typed;
    typed.resize(card);
    type(&typed, "nameEdit", "Motor de continua");
    type(&typed, "descriptionEdit", "El del articulo de 2007, con la inercia incierta");
    check(&typed, "transferFunctionRadio");
    check(&typed, "zerosPolesRadio");
    check(&typed, "zpkRadio");
    type(&typed, "numeratorEdit", "");
    type(&typed, "denominatorEdit", "0 a");
    type(&typed, "gainEdit", "3");
    type(&typed, "delayEdit", "0");
    typed.grab().save(out + "/planta-escrita.png");

    press(&typed, "uncertaintyButton");
    QCoreApplication::processEvents();
    typed.grab().save(out + "/planta-incertidumbre.png");

    type(&typed, "rangeMinimum", "1");
    type(&typed, "rangeNominal", "3");
    type(&typed, "rangeMaximum", "10");
    type(&typed, "rangeGainStart", "1");
    type(&typed, "rangeGainEnd", "20");
    press(&typed, "applyButton");
    QCoreApplication::processEvents();

    press(&typed, "okButton");
    QCoreApplication::processEvents();
    typed.grab().save(out + "/planta-verificada.png");

    //And what a wrong field looks like.
    PlantForm wrong;
    wrong.resize(card);
    type(&wrong, "nameEdit", "rota");
    check(&wrong, "transferFunctionRadio");
    check(&wrong, "polynomialRadio");
    type(&wrong, "numeratorEdit", "1");
    type(&wrong, "denominatorEdit", "1 2*/");
    type(&wrong, "gainEdit", "1");
    press(&wrong, "okButton");
    QCoreApplication::processEvents();
    wrong.grab().save(out + "/planta-error.png");
}

TEST_F(GuiSmoke, ZZControllerFreedom)
{
    const QString out = qEnvironmentVariable("QFTBX_RENDER_DIR");
    if (out.isEmpty()) {
        GTEST_SKIP();
    }

    qftbx::applyTheme(qftbx::kLightTheme);

    ProjectController project;
    project.load((out + "/ejemplo-completo.qft").toStdString());

    ControllerForm form;
    form.resize(660, 360);
    form.setFromProject(project.controllerStructure());
    form.grab().save(out + "/controlador-datos.png");

    press(&form, "uncertaintyButton");
    QCoreApplication::processEvents();
    form.grab().save(out + "/controlador-libertad.png");

    for (QLineEdit * field : form.findChildren<QLineEdit *>()) {
        std::printf("%-16s = %s\n", field->objectName().toStdString().c_str(),
                    field->text().toStdString().c_str());
    }
    std::fflush(stdout);
}

TEST_F(GuiSmoke, ZZFormulas)
{
    const QString out = qEnvironmentVariable("QFTBX_RENDER_DIR");
    if (out.isEmpty()) {
        GTEST_SKIP();
    }

    //One sheet with the shapes that have to come out right: fractions,
    //exponents, roots, tall parentheses, subscripts, and the four families
    //of a real plant.
    struct Case { const char * title; qftbx::Formula formula; };

    ProjectController project;
    project.load(std::string(QFTBX_TEST_DATA_DIR "/qft_toolbox_ex2.qft"));

    std::vector<Case> cases;
    cases.push_back({"plant of ex2", qftbx::formulaOf(*project.plant(), 4)});
    cases.push_back({"typed", qftbx::formulaOfText("k*(s+a1)/(s*(s^2+2*z*w*s+w^2))", 4)});
    cases.push_back({"roots", qftbx::formulaOfText("sqrt(1+(s/10)^2)/abs(s+1)", 4)});
    cases.push_back({"functions", qftbx::formulaOfText("exp(-0.05*s)*sin(pi*s)/ln(1+s)", 4)});
    cases.push_back({"nested", qftbx::formulaOfText("1/(1+1/(1+1/s))", 4)});

    QImage sheet(900, 190 * int(cases.size()), QImage::Format_ARGB32);
    sheet.fill(Qt::white);

    for (std::size_t i = 0; i < cases.size(); ++i) {
        FormulaView view;
        view.resize(880, 170);
        view.setFormula(cases[i].formula);

        QPixmap drawn = view.grab();

        QPainter painter(&sheet);
        painter.drawPixmap(10, 190 * int(i) + 10, drawn);
        painter.setPen(Qt::gray);
        painter.drawText(14, 190 * int(i) + 22, QString::fromLatin1(cases[i].title));
        painter.drawRect(10, 190 * int(i) + 10, 880, 170);
        painter.end();

        std::printf("%-12s %s\n", cases[i].title,
                    qftbx::latexOf(cases[i].formula).c_str());
    }

    std::fflush(stdout);
    sheet.save(out + "/formulas.png");
}

//The three probes below are not tests: they are how the forms are LOOKED
//at. Each writes what it makes into QFTBX_RENDER_DIR and is skipped when
//that variable is not set, so an ordinary run never sees them. They are
//kept because a layout is reviewed by looking at it, and looking at it by
//hand means building the whole application, opening a project and pressing
//seven buttons.
TEST_F(GuiSmoke, ZZEjemploCompleto)
{
    const QString out = qEnvironmentVariable("QFTBX_RENDER_DIR");
    if (out.isEmpty()) {
        GTEST_SKIP();
    }

    //A project solved FOR REAL by this binary: the file it saves carries
    //the settings it was computed with and the verdict of the checker,
    //which is what the older .qft files do not have.
    ProjectController project;
    project.load(std::string(QFTBX_TEST_DATA_DIR "/qft_toolbox_ex2.qft"));

    qftbx::Settings settings;
    settings.algorithms.conservativeBoundaryColumns = true;
    project.applySettings(settings);

    const bool designed = project.computeLoopShaping(0.5, qftbx::mc2,
                                                     qftbx::Range(0.01, 1000.0), 500);
    std::printf("designed: %d\n", (int) designed);
    if (project.loopShapingResult() != nullptr && project.loopShapingResult()->check().has_value()) {
        std::printf("verdict: %s by %.4f dB\n",
                    project.loopShapingResult()->check()->satisfied() ? "satisfied" : "EXCEEDED",
                    project.loopShapingResult()->check()->worstExcessDb);
    }
    std::fflush(stdout);

    project.save((out + "/ejemplo-completo.qft").toStdString());
}

TEST_F(GuiSmoke, ZZGaleria)
{
    const QString out = qEnvironmentVariable("QFTBX_RENDER_DIR");
    if (out.isEmpty()) {
        GTEST_SKIP();
    }

    qftbx::applyTheme(qftbx::kLightTheme);

    MainWindow window;
    window.setFileChooser([&out](bool) { return out + "/ejemplo-completo.qft"; });
    window.findChild<QAction *>("actionOpen")->trigger();
    window.resize(1400, 900);
    window.show();
    QCoreApplication::processEvents();

    //Every card, folded and open, at the size the canvas gives it.
    for (const char * nombre : {"plantCard", "frequenciesCard", "specificationsCard",
                                "templatesCard", "boundariesCard", "controllerCard",
                                "loopShapingCard"}) {
        PhaseCard * card = window.findChild<PhaseCard *>(nombre);
        if (card == nullptr) { continue; }

        card->showForm(false);
        QCoreApplication::processEvents();
        card->grab().save(out + "/" + nombre + "-plegada.png");

        card->showForm(true);
        QCoreApplication::processEvents();
        card->grab().save(out + "/" + nombre + "-abierta.png");
        card->showForm(false);
    }

    window.grab().save(out + "/ventana.png");
}

TEST_F(GuiSmoke, TheSquareOfTheCanvasFollowsTheScreenItIsGiven)
{
    //The cards fill the width they are given instead of being a fixed size:
    //a column of 560 leaves 144 pixels of nothing on a 1280 screen, and
    //makes six tiny cards on a 4K one where three big ones were wanted.
    const QSize narrow = PhaseCard::unitFor(700);
    const QSize normal = PhaseCard::unitFor(1264);
    const QSize wide = PhaseCard::unitFor(3800);

    EXPECT_EQ(PhaseCard::columnsFor(700), 1);
    EXPECT_EQ(PhaseCard::columnsFor(1264), 2);
    EXPECT_EQ(PhaseCard::columnsFor(1900), 3);

    EXPECT_EQ(narrow.width(), 700) << "one card across takes the whole width";
    EXPECT_EQ(normal.width() * 2 + 8, 1264) << "two across share it exactly";
    EXPECT_GT(wide.width(), normal.width()) << "a wider screen gives bigger cards, not more";

    //And a square is a square-ish: a card is never a strip.
    for (const QSize & unit : {narrow, normal, wide}) {
        EXPECT_GT(unit.height(), unit.width() / 2);
        EXPECT_LT(unit.height(), unit.width());
    }
}

TEST_F(GuiSmoke, TheWidthOfACardIsTheUsersAndTheFoldDoesNotTouchIt)
{
    //Two sizes that answer to two different things: the user says how much
    //of the canvas a phase is worth, with its own buttons, and folding the
    //form only adds or removes the band below the diagram. A card made wide
    //stays wide with its form open or closed.
    MainWindow window;
    window.setFileChooser([](bool) {
        return QString(QFTBX_TEST_DATA_DIR "/planta1.qft");
    });
    window.findChild<QAction *>("actionOpen")->trigger();

    PhaseCard * card = window.findChild<PhaseCard *>("loopShapingCard");
    ASSERT_NE(card, nullptr);
    ASSERT_EQ(card->span(), 1);

    const QSize one = card->sizeHint();

    card->findChild<QToolButton *>("loopShapingCardWider")->click();
    EXPECT_EQ(card->span(), 2);
    //Bigger, not longer: a card of two takes two columns across AND two
    //rows down, so the diagram in it is seen better and not just wider.
    EXPECT_GT(card->sizeHint().width(), one.width());
    EXPECT_GT(card->sizeHint().height(), one.height());

    const QSize wide = card->sizeHint();
    card->showForm(true);
    EXPECT_EQ(card->sizeHint().width(), wide.width()) << "the fold moved the width the user set";
    EXPECT_GT(card->sizeHint().height(), wide.height());

    card->showForm(false);
    EXPECT_EQ(card->sizeHint(), wide);

    //And it stops where the canvas does, instead of growing for ever.
    card->findChild<QToolButton *>("loopShapingCardWider")->click();
    card->findChild<QToolButton *>("loopShapingCardWider")->click();
    card->findChild<QToolButton *>("loopShapingCardWider")->click();
    EXPECT_EQ(card->span(), 3);

    card->findChild<QToolButton *>("loopShapingCardNarrower")->click();
    card->findChild<QToolButton *>("loopShapingCardNarrower")->click();
    card->findChild<QToolButton *>("loopShapingCardNarrower")->click();
    EXPECT_EQ(card->span(), 1);
}

TEST_F(GuiSmoke, EachPhaseIsOneCardWithItsFormAndItsDiagramsInside)
{
    //What the cards are for: the form that describes a step and the
    //diagrams of that step in one thing on the screen, instead of a modal
    //dialog that vanished and a viewer in a window of its own.
    MainWindow window;

    child<QPushButton>(&window, "plantButton")->click();

    PhaseCard * plant = window.findChild<PhaseCard *>("plantCard");
    ASSERT_NE(plant, nullptr) << "the plant phase has no card";
    EXPECT_NE(plant->findChild<PlantForm *>(), nullptr);
    EXPECT_NE(plant->findChild<BodeViewer *>(), nullptr)
        << "the Bode diagram belongs in the card of the plant it draws";
    EXPECT_TRUE(plant->isFormShown()) << "pressing the step opens the form";

    //A phase that is only a form is a card too, and has nothing to fold.
    fillPlant(panelIn<PlantForm>(&window), "carded");
    child<QPushButton>(&window, "frequenciesButton")->click();

    PhaseCard * frequencies = window.findChild<PhaseCard *>("frequenciesCard");
    ASSERT_NE(frequencies, nullptr);
    EXPECT_NE(frequencies->findChild<FrequenciesForm *>(), nullptr);
    EXPECT_TRUE(frequencies->isFormShown());
}

TEST_F(GuiSmoke, TheBodeDiagramIsDrawnAsSoonAsThereIsSomethingToDraw)
{
    //It used to be a window of its own behind a menu entry, asked for and
    //gone stale in silence. It is in the plant's dock now, and it is redrawn
    //whenever the plant or the frequencies change.
    MainWindow window;

    child<QPushButton>(&window, "plantButton")->click();
    fillPlant(panelIn<PlantForm>(&window), "drawn");

    BodeViewer * bode = panelIn<BodeViewer>(&window);
    ASSERT_NE(bode, nullptr);
    QCustomPlot * magnitude = child<QCustomPlot>(bode, "magnitudePlot");
    ASSERT_NE(magnitude, nullptr);
    EXPECT_EQ(magnitude->plottableCount(), 0)
        << "a plant with no design frequencies has no Bode diagram yet";

    child<QPushButton>(&window, "frequenciesButton")->click();
    fillFrequencies(panelIn<FrequenciesForm>(&window));

    EXPECT_GT(magnitude->plottableCount(), 0)
        << "with a plant and its frequencies the diagram draws itself";
}

TEST_F(GuiSmoke, LookingAgainAtAStepAlreadyDoneLeavesItDone)
{
    //The defect that deriving the state fixed, pinned so it cannot come back.
    //Cancelling the dialog of a step that was already finished used to delete
    //its widgets and walk the progress bar backwards, while the project still
    //held the artefact - the window said the step was undone and the project
    //said it was done.
    MainWindow window;

    QPushButton * plantButton = child<QPushButton>(&window, "plantButton");
    ASSERT_NE(plantButton, nullptr);
    plantButton->click();
    fillPlant(panelIn<PlantForm>(&window), "walked");

    QProgressBar * progress = child<QProgressBar>(&window, "progressBar");
    ASSERT_NE(progress, nullptr);
    ASSERT_EQ(progress->value(), 1);

    //Now a user who brings the phase up again and accepts nothing.
    plantButton->click();

    EXPECT_EQ(progress->value(), 1)
        << "the plant is still in the project, so the step is still done";
}

TEST_F(GuiSmoke, OpeningAProjectRebuildsTheStepsItCarried)
{
    //The fifty lines that used to be ninety-six: after a load, the window's
    //state is the project's state, and the widgets of the steps the file
    //carried are the ones that exist.
    MainWindow window;

    window.setFileChooser([](bool forSaving) {
        EXPECT_FALSE(forSaving);
        return QString(QFTBX_TEST_DATA_DIR "/planta1.qft");
    });

    QAction * open = window.findChild<QAction *>("actionOpen");
    ASSERT_NE(open, nullptr) << "the open action is what the test drives";
    open->trigger();

    //planta1.qft is a finished design: every step done.
    QProgressBar * progress = child<QProgressBar>(&window, "progressBar");
    ASSERT_NE(progress, nullptr);
    EXPECT_EQ(progress->value(), static_cast<int>(qftbx::kStepCount));

    //And every button that a finished design unlocks.
    EXPECT_TRUE(child<QPushButton>(&window, "templatesButton")->isEnabled());
    EXPECT_TRUE(child<QPushButton>(&window, "boundariesButton")->isEnabled());
    EXPECT_TRUE(child<QPushButton>(&window, "loopButton")->isEnabled());
}


TEST_F(GuiSmoke, ThePlantFormShowsThePlantOfTheProject)
{
    //Opening a project used to leave the plant form empty: the project
    //carries a system, not the text it was typed as, so the form is written
    //back from it. Accepting without editing must give back the same plant,
    //uncertainty included - the intervals come with the parameters and not
    //from the fields.
    ProjectController project;
    project.load(std::string(QFTBX_TEST_DATA_DIR "/planta1.qft"));
    ASSERT_NE(project.plant(), nullptr);

    PlantForm dialog;
    dialog.setFromProject(project.plant());

    EXPECT_EQ(child<QLineEdit>(&dialog, "nameEdit")->text(), QString("aa"));
    EXPECT_TRUE(child<QRadioButton>(&dialog, "zpkRadio")->isChecked());
    EXPECT_EQ(child<QLineEdit>(&dialog, "denominatorEdit")->text(), QString("a b"))
        << "an uncertain coefficient shows its name, which is what the field holds";

    press(&dialog, "okButton");
    ASSERT_TRUE(dialog.wasAccepted()) << "the form rejected the plant it was given";

    const std::unique_ptr<LtiSystem> described = dialog.takePlant();
    ASSERT_NE(described, nullptr);
    EXPECT_TRUE(described->sameAs(*project.plant()))
        << "the form gave back a different plant from the one it was shown";
}

TEST_F(GuiSmoke, EditingThePlantFormLeavesTheProjectsParametersBehind)
{
    //The other half: the parameters of the loaded plant describe the text
    //the form was filled with and nothing else, so an edited field is read
    //as what it says.
    ProjectController project;
    project.load(std::string(QFTBX_TEST_DATA_DIR "/planta1.qft"));
    ASSERT_NE(project.plant(), nullptr);

    PlantForm dialog;
    dialog.setFromProject(project.plant());
    type(&dialog, "denominatorEdit", "2 7");

    //An edit undoes the verification the file came with: what is on screen
    //is no longer what was checked.
    EXPECT_EQ(child<QPushButton>(&dialog, "okButton")->text(), QString("Verify"));
    verifyAndApply(&dialog);
    ASSERT_TRUE(dialog.wasAccepted());

    const std::unique_ptr<LtiSystem> described = dialog.takePlant();
    ASSERT_NE(described, nullptr);
    EXPECT_FALSE(described->sameAs(*project.plant()));
    ASSERT_EQ(described->denominator().size(), 2u);
    EXPECT_FALSE(described->denominator().at(0).isUncertain())
        << "the edited coefficient is the number typed, not the old interval";
    EXPECT_DOUBLE_EQ(described->denominator().at(0).nominal(), 2.0);

    //And only that: the gain field was not touched, so the gain is still
    //the plant's own, name included - a form has no field for that name.
    EXPECT_EQ(described->gain().name(), project.plant()->gain().name());
    EXPECT_DOUBLE_EQ(described->gain().nominal(), project.plant()->gain().nominal());
    EXPECT_DOUBLE_EQ(described->gain().range().min, project.plant()->gain().range().min);
    EXPECT_DOUBLE_EQ(described->gain().range().max, project.plant()->gain().range().max);
}

TEST_F(GuiSmoke, TheControllerFormShowsTheStructureOfTheProject)
{
    ProjectController project;
    project.load(std::string(QFTBX_TEST_DATA_DIR "/planta1.qft"));
    ASSERT_NE(project.controllerStructure(), nullptr);

    ControllerForm dialog;
    dialog.setFromProject(project.controllerStructure());

    press(&dialog, "okButton");
    ASSERT_TRUE(dialog.wasAccepted()) << "the form rejected the structure it was given";

    const std::unique_ptr<LtiSystem> described = dialog.takeControllerStructure();
    ASSERT_NE(described, nullptr);
    EXPECT_TRUE(described->sameAs(*project.controllerStructure()))
        << "the form gave back a different structure from the one it was shown";
}

TEST_F(GuiSmoke, TheLoopFormShowsWhatProducedTheDesign)
{
    //A design is a number that depends on the algorithm, the tolerance and
    //the reading of the phase grid; the form used to open on the defaults
    //over a project that had been solved with something else.
    ProjectController project;
    project.load(std::string(QFTBX_TEST_DATA_DIR "/planta1.qft"));
    ASSERT_NE(project.loopShapingResult(), nullptr);
    project.loopShapingResult()->setRun({qftbx::mc2, 0.02, true});

    LoopShapingForm dialog;
    dialog.setFromProject(project.loopShapingResult());

    EXPECT_TRUE(child<QRadioButton>(&dialog, "mc2Radio")->isChecked());
    EXPECT_EQ(child<QLineEdit>(&dialog, "epsilonEdit")->text(), QString("0.02"));
    EXPECT_TRUE(child<QCheckBox>(&dialog, "conservativeColumnsCheck")->isChecked());
}

TEST_F(GuiSmoke, OpeningAProjectDrawsWhatItCarries)
{
    //The diagram sits beside the data that produced it, so a file with
    //results has them on screen the moment it is opened. It used to be that
    //the numbers came back and every plot stayed empty until the user found
    //the right menu entry.
    MainWindow window;

    window.setFileChooser([](bool) {
        return QString(QFTBX_TEST_DATA_DIR "/planta1.qft");
    });

    window.findChild<QAction *>("actionOpen")->trigger();

    const auto drawn = [&](auto * viewer, const char * name) {
        QCustomPlot * plot = child<QCustomPlot>(viewer, name);
        return plot != nullptr && plot->plottableCount() > 0;
    };

    TemplateViewer * templates = window.findChild<TemplateViewer *>();
    ASSERT_NE(templates, nullptr);
    EXPECT_TRUE(drawn(templates, "plot")) << "the templates of the file are not on screen";

    BoundaryUnionViewer * boundaries = window.findChild<BoundaryUnionViewer *>();
    ASSERT_NE(boundaries, nullptr);
    EXPECT_TRUE(drawn(boundaries, "plot")) << "the boundaries of the file are not on screen";

    LoopShapingViewer * loop = window.findChild<LoopShapingViewer *>();
    ASSERT_NE(loop, nullptr);
    EXPECT_TRUE(drawn(loop, "plot")) << "the design of the file is not on screen";

    BodeViewer * bode = window.findChild<BodeViewer *>();
    ASSERT_NE(bode, nullptr);
    EXPECT_TRUE(drawn(bode, "magnitudePlot")) << "the plant of the file has no Bode diagram";
}

TEST_F(GuiSmoke, OpeningAProjectOverPhasesAlreadyOpenSurvives)
{
    MainWindow window;
    window.show();

    child<QPushButton>(&window, "plantButton")->click();
    fillPlant(panelIn<PlantForm>(&window), "before");
    child<QPushButton>(&window, "frequenciesButton")->click();
    QCoreApplication::processEvents();

    window.setFileChooser([](bool) {
        return QString(QFTBX_TEST_DATA_DIR "/planta1.qft");
    });
    window.findChild<QAction *>("actionOpen")->trigger();

    //The widgets of the previous project are freed through deleteLater, so
    //they die here and not before: whatever the window kept pointing at
    //them shows up now and not in a test without an event loop.
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();

    QProgressBar * progress = child<QProgressBar>(&window, "progressBar");
    ASSERT_NE(progress, nullptr);
    EXPECT_EQ(progress->value(), static_cast<int>(qftbx::kStepCount));
}

//Every project of the fixtures, opened through the window: the phases it
//carries are built, what it carries is drawn, and the window is still
//standing afterwards. The drawing runs by itself now, so a file that the
//reader accepts and the diagram cannot draw is a crash on opening, which is
//exactly what this is here to stop.
class OpenedProject : public GuiSmoke, public ::testing::WithParamInterface<const char *>
{
};

TEST_P(OpenedProject, OpensWithoutTakingTheWindowDown)
{
    MainWindow window;

    window.setFileChooser([](bool) {
        return QString(QFTBX_TEST_DATA_DIR "/") + QString::fromUtf8(GetParam());
    });

    window.findChild<QAction *>("actionOpen")->trigger();

    QProgressBar * progress = child<QProgressBar>(&window, "progressBar");
    ASSERT_NE(progress, nullptr);
    EXPECT_GT(progress->value(), 0) << "the file carried nothing the window could use";

    //And again over the phases the first one left open, which is what a
    //user does all day.
    window.findChild<QAction *>("actionOpen")->trigger();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();

    EXPECT_GT(progress->value(), 0);
}

INSTANTIATE_TEST_SUITE_P(Fixtures, OpenedProject,
                         ::testing::Values("planta1.qft", "planta2.qft", "acc90.qft",
                                           "multivaluados.qft", "cervera.qft",
                                           "qft_toolbox_ex2.qft"));

TEST_F(GuiSmoke, TheTemplateViewerDrawsAProjectThatBroughtNoEpsilon)
{
    //A project can hold the clouds and not the tolerance they were walked
    //with, and the viewer used to ask its epsilon vector for an element it
    //did not have: the toolbox died with a message about vector ranges over
    //a diagram it could perfectly well draw.
    TemplateViewer viewer;

    qftbx::CloudSet templates{{{1.0, 2.0}, {2.0, 3.0}, {3.0, 1.0}}};
    qftbx::CloudSet contour = templates;
    std::vector<double> omega{1.0};

    viewer.setData(templates, contour, &omega, nullptr);
    viewer.plotDiagram(true);

    QCustomPlot * plot = child<QCustomPlot>(&viewer, "plot");
    ASSERT_NE(plot, nullptr);
    EXPECT_GT(plot->graphCount(), 0) << "the templates are there to be drawn";
}

TEST_F(GuiSmoke, AReusedDialogForgetsItsPreviousAcceptance)
{
    //StepDialog directly: the seven dialogs used to declare this flag each
    //for themselves, set it on OK and never clear it, so a reused one
    //reported an acceptance whose payload it had already handed over.
    PlantForm dialog;

    EXPECT_FALSE(dialog.wasAccepted()) << "a fresh dialog has accepted nothing";

    type(&dialog, "nameEdit", "reused");
    check(&dialog, "zpkRadio");
    type(&dialog, "numeratorEdit", "2");
    type(&dialog, "denominatorEdit", "5 30");
    type(&dialog, "gainEdit", "3");
    type(&dialog, "delayEdit", "0");
    verifyAndApply(&dialog);

    ASSERT_TRUE(dialog.wasAccepted());

    //Which is what the window does before showing it again.
    dialog.clearAcceptance();

    EXPECT_FALSE(dialog.wasAccepted())
        << "an acceptance must not outlive the showing it belongs to";
}

} // namespace

TEST_F(GuiSmoke, TheHelpMenuSaysWhatTheToolboxIsAndWhoWroteIt)
{
    MainWindow window;
    auto * about = window.findChild<QAction *>("actionAbout");
    ASSERT_NE(about, nullptr);
    EXPECT_EQ(about->text(), QStringLiteral("&About QFTbx..."));
    ASSERT_NE(window.findChild<QAction *>("actionAboutQt"), nullptr);

    applyLanguage(kSourceLanguage);
    const QString text = aboutText();
    EXPECT_TRUE(text.contains("isaac.martinez@upct.es"));
    EXPECT_TRUE(text.contains("jcervera@um.es"));
    EXPECT_TRUE(text.contains("Quantitative Feedback Theory"));
    EXPECT_TRUE(text.contains("under development"));
    EXPECT_TRUE(text.contains("https://github.com/Isaac-Martinez-Forte/QFTbx"));
    EXPECT_TRUE(text.contains("GNU General Public License"));

    //In Spanish too: the texts are filed under the context the box looks in.
    applyLanguage("es");
    EXPECT_TRUE(aboutText().contains("Autores")) << aboutText().toStdString();
    EXPECT_TRUE(aboutText().contains(QString::fromUtf8("en desarrollo")));
    applyLanguage(kSourceLanguage);
}

//The colour of a curve says which frequency it is, so it has to be distinct
//for as many frequencies as a problem has. The named-colour palette this
//replaces ran out at fourteen and painted every one after that the same
//cyan, which on the twenty-three frequencies of the Horowitz-Sidi motor was
//eleven curves nobody could tell apart.
TEST_F(GuiSmoke, TheFrequencyColoursDoNotRepeatOrFadeOut)
{
    for (const int count : {5, 14, 23, 40}) {
        QSet<QRgb> seen;
        for (int i = 0; i < count; ++i) {
            const QColor colour = qftbx::frequencyColour(i, count);
            EXPECT_TRUE(colour.isValid()) << count << " frequencies, index " << i;
            seen.insert(colour.rgb());

            //Readable on white: the top of viridis is a light yellow and the
            //walk has to stop short of it.
            EXPECT_LT(colour.lightness(), 225)
                << count << " frequencies, index " << i << " is too light to see on white";
        }
        EXPECT_EQ(seen.size(), count) << count << " frequencies gave " << seen.size() << " colours";
    }

    //Ordered: the map walks from dark blue to green, so the first and the
    //last of a sweep are never neighbours.
    const QColor first = qftbx::frequencyColour(0, 20);
    const QColor last = qftbx::frequencyColour(19, 20);
    const int distance = std::abs(first.red() - last.red())
                       + std::abs(first.green() - last.green())
                       + std::abs(first.blue() - last.blue());
    EXPECT_GT(distance, 150) << "the ends of the sweep are too close to tell apart";
}

//Every canvas is set up once, when its viewer is built, and not from the
//drawing routine: a plot configured while drawing has no interactions until
//it has data. So an untouched viewer already answers for its axes and its
//mouse.
TEST_F(GuiSmoke, EveryCanvasIsSetUpBeforeItHasData)
{
    const auto expectReady = [](QCustomPlot * plot, const char * what) {
        ASSERT_NE(plot, nullptr) << what;
        EXPECT_FALSE(plot->xAxis->label().isEmpty()) << what << ": the x axis has no label";
        EXPECT_FALSE(plot->yAxis->label().isEmpty()) << what << ": the y axis has no label";
        EXPECT_TRUE(plot->interactions().testFlag(QCP::iRangeDrag)) << what << ": cannot be dragged";
        EXPECT_TRUE(plot->interactions().testFlag(QCP::iRangeZoom)) << what << ": cannot be zoomed";
    };

    TemplateViewer templates;
    expectReady(templates.findChild<QCustomPlot *>("plot"), "template viewer");

    BoundaryViewer boundaries;
    expectReady(boundaries.findChild<QCustomPlot *>("plot"), "boundary viewer");

    BoundaryUnionViewer unionViewer;
    expectReady(unionViewer.findChild<QCustomPlot *>("plot"), "boundary union viewer");

    LoopBoundariesViewer loopBoundaries;
    expectReady(loopBoundaries.findChild<QCustomPlot *>("plot"), "loop boundaries viewer");

    LoopShapingViewer loop;
    expectReady(loop.findChild<QCustomPlot *>("plot"), "loop shaping viewer");

    //Bode had no interactions at all: its canvases were never set up.
    BodeViewer bode;
    expectReady(bode.findChild<QCustomPlot *>("magnitudePlot"), "Bode magnitude");
    expectReady(bode.findChild<QCustomPlot *>("phasePlot"), "Bode phase");
}

//A legend has one row per design frequency, and a problem has as many as it
//has frequencies: twenty-three on the Horowitz-Sidi motor, twice that in the
//loop viewer's both-diagrams mode. Without a scroll area the last rows
//cannot be reached at all, and twenty checkboxes are not worked one at a
//time either.
TEST_F(GuiSmoke, TheLegendScrollsAndCanBeWorkedInOneGo)
{
    qftbx::FrequencyLegend legend;
    legend.resize(160, 200);

    for (int i = 0; i < 23; ++i) {
        legend.addRow(QString::number(i < 10 ? 0.1 * (i + 1) : i), qftbx::frequencyColour(i, 23));
    }
    ASSERT_EQ(legend.rowCount(), 23);

    //The rows are inside a scroll area, so the box being shorter than they
    //are does not put any of them out of reach.
    QScrollArea * scroll = legend.findChild<QScrollArea *>("legendScroll");
    ASSERT_NE(scroll, nullptr) << "the rows are not in a scroll area";
    EXPECT_TRUE(scroll->widgetResizable());

    QPushButton * none = child<QPushButton>(&legend, "legendNone");
    QPushButton * all = child<QPushButton>(&legend, "legendAll");
    ASSERT_NE(none, nullptr);
    ASSERT_NE(all, nullptr);

    int toggles = 0;
    QObject::connect(&legend, &qftbx::FrequencyLegend::rowToggled, [&toggles]() { ++toggles; });

    none->click();
    for (int i = 0; i < legend.rowCount(); ++i) {
        EXPECT_FALSE(legend.isRowChecked(i)) << "row " << i << " survived None";
    }
    EXPECT_EQ(toggles, 1) << "the owner is told once, not once per row";

    all->click();
    for (int i = 0; i < legend.rowCount(); ++i) {
        EXPECT_TRUE(legend.isRowChecked(i)) << "row " << i << " missed All";
    }

    //The filter hides rows, and All then means all of what is in front of
    //the user: that is what makes the filter worth having.
    legend.show();
    QCoreApplication::processEvents();
    QLineEdit * filter = child<QLineEdit>(&legend, "legendFilter");
    ASSERT_NE(filter, nullptr);
    filter->setText("0.1");
    QCoreApplication::processEvents();

    none->click();
    int hiddenAndStillChecked = 0;
    for (int i = 0; i < legend.rowCount(); ++i) {
        if (legend.isRowChecked(i)) {
            ++hiddenAndStillChecked;
        }
    }
    EXPECT_GT(hiddenAndStillChecked, 0) << "None cleared rows the filter was hiding";
}

//A figure for a paper has to be vector, has to be the size it was asked for
//and not the size the window happened to be, and has to be on white however
//the interface is themed.
TEST_F(GuiSmoke, AFigureIsExportedAsVectorAtTheSizeAskedFor)
{
    QTemporaryDir directory;
    ASSERT_TRUE(directory.isValid());

    BoundaryUnionViewer viewer;
    const qftbx::UnionTraces traces{{qftbx::NicholsPoint(-270.0, 10.0), qftbx::NicholsPoint(-180.0, 0.0),
                                     qftbx::NicholsPoint(-90.0, 10.0)}};
    std::vector<double> omega{1.0};
    viewer.setData(traces, &omega);
    viewer.showDiagram();

    QCustomPlot * plot = child<QCustomPlot>(&viewer, "plot");
    ASSERT_NE(plot, nullptr);

    //SVG: vector, and it carries the curve as a path rather than as pixels.
    qftbx::ExportRequest svg;
    svg.fileName = directory.filePath("figure.svg");
    svg.size = QSize(1200, 900);
    svg.profile = qftbx::ExportProfile::ForPublishing;
    EXPECT_TRUE(qftbx::savePlot(*plot, svg)) << "the SVG was not written";

    QFile written(svg.fileName);
    ASSERT_TRUE(written.open(QIODevice::ReadOnly));
    const QString content = QString::fromUtf8(written.readAll());
    EXPECT_TRUE(content.contains("<svg")) << "what came out is not an SVG";
    EXPECT_TRUE(content.contains("width=\"1200\"")) << "the size asked for was ignored";
    EXPECT_TRUE(content.contains("<path") || content.contains("<polyline"))
        << "the curve is not in the file as a path";

    //PDF, the other vector format.
    qftbx::ExportRequest pdf;
    pdf.fileName = directory.filePath("figure.pdf");
    pdf.size = QSize(1200, 900);
    EXPECT_TRUE(qftbx::savePlot(*plot, pdf));
    EXPECT_GT(QFileInfo(pdf.fileName).size(), 0);

    //PNG at the size asked for, not at the window's.
    qftbx::ExportRequest png;
    png.fileName = directory.filePath("figure.png");
    png.size = QSize(1600, 1200);
    EXPECT_TRUE(qftbx::savePlot(*plot, png));
    const QImage image(png.fileName);
    EXPECT_EQ(image.width(), 1600) << "the PNG came out at the window's size";

    //And the publishing profile leaves the viewer as it found it: it is a
    //way of writing the figure, not a change to what is on screen.
    const QColor axisBefore = plot->xAxis->labelColor();
    const double penBefore = plot->plottable(0)->pen().widthF();
    qftbx::ExportRequest again = svg;
    again.fileName = directory.filePath("twice.svg");
    EXPECT_TRUE(qftbx::savePlot(*plot, again));
    EXPECT_EQ(plot->xAxis->labelColor(), axisBefore) << "the export changed the screen";
    EXPECT_DOUBLE_EQ(plot->plottable(0)->pen().widthF(), penBefore) << "the export changed the screen";
}

//Opening a project has to SHOW it. The dialogs kept their own copy of what
//the user had typed and nothing ever flowed the other way, so a design read
//from a file left every form empty: the plant was in the project and the
//dialog did not know it.
TEST_F(GuiSmoke, OpeningAProjectFillsTheFormsWithWhatItHolds)
{
    MainWindow window;
    window.setFileChooser([](bool) {
        return QString(QFTBX_TEST_DATA_DIR "/qft_toolbox_ex2.qft");
    });

    QAction * open = child<QAction>(&window, "actionOpen");
    ASSERT_NE(open, nullptr);
    open->trigger();

    //The frequencies: the fixture carries twenty of them, and the manual
    //page lists the values whatever the mode, since that is what the project
    //has rather than a rule that regenerates them.
    FrequenciesForm * frequencies = window.findChild<FrequenciesForm *>();
    ASSERT_NE(frequencies, nullptr) << "the frequencies dialog was not built";
    QLineEdit * values = child<QLineEdit>(frequencies, "manualValues");
    ASSERT_NE(values, nullptr);
    EXPECT_FALSE(values->text().isEmpty()) << "the frequencies came out empty";
    EXPECT_GE(values->text().split(' ', Qt::SkipEmptyParts).size(), 2)
        << "only one frequency was filled in";

    //The boundary grid: the one the boundaries in the file were computed on,
    //not the default the dialog opens with.
    BoundaryGridForm * grid = window.findChild<BoundaryGridForm *>();
    ASSERT_NE(grid, nullptr) << "the boundary dialog was not built";
    QLineEdit * phasePoints = child<QLineEdit>(grid, "phasePoints");
    ASSERT_NE(phasePoints, nullptr);
    EXPECT_FALSE(phasePoints->text().isEmpty()) << "the phase count came out empty";
    EXPECT_GT(phasePoints->text().toInt(), 1) << "the phase count is not a grid";
}

//The window destroys a viewer from refreshAvailability(), which the project
//calls when it changes - and a project changes from inside a widget's own
//slot: the template viewer's Recompute button asks for a contour and the
//window computes it. Freeing the widget whose slot is still on the stack is
//a use after free, so they go through deleteLater() and Qt destroys them
//when control is back at the event loop.
TEST_F(GuiSmoke, RecomputingFromTheViewerDoesNotFreeItUnderItsOwnSlot)
{
    MainWindow window;
    window.setFileChooser([](bool) {
        return QString(QFTBX_TEST_DATA_DIR "/qft_toolbox_ex2.qft");
    });

    QAction * open = child<QAction>(&window, "actionOpen");
    ASSERT_NE(open, nullptr);
    open->trigger();

    TemplateViewer * viewer = window.findChild<TemplateViewer *>();
    ASSERT_NE(viewer, nullptr) << "the template viewer was not built";

    //Its own button, as the user presses it: the handler reaches the project,
    //the project announces, and the window re-derives its widgets while this
    //slot is still running.
    QPushButton * recompute = child<QPushButton>(viewer, "recomputeButton");
    ASSERT_NE(recompute, nullptr);
    recompute->click();

    //Still here, and still answering: nothing was freed underneath it.
    EXPECT_NE(window.findChild<TemplateViewer *>(), nullptr)
        << "the viewer was destroyed while its own slot was on the stack";

    //And the event loop can run without tripping over what was queued.
    QCoreApplication::processEvents();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
}
