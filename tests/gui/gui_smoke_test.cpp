/**
 * @file
 * @brief Smoke tests over the Qt interface, run headless.
 *
 * The forms are where the user's numbers become model objects, and the backend
 * suite cannot reach them. Each test drives a form the way a user does, filling
 * its fields by object name and pressing its button, and reads the form's own
 * answer; the modal error box is redirected so a rejection is asserted rather
 * than waited on. The main window is driven through its panels and cards: what
 * a step publishes and unlocks, what every fixture project puts on the canvas
 * when opened, how cards fold, grow, wrap and change places, what the theme,
 * tooltips, legends and figure export must do, and that a computation runs off
 * the GUI thread and cancels leaving the project as it was. The probes that
 * render forms into `QFTBX_RENDER_DIR` are skipped unless that variable is set.
 */

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
#include <QSlider>
#include <QSplitter>
#include <QMouseEvent>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QIcon>
#include <QDockWidget>
#include <limits>
#include <QToolButton>
#include <QStackedWidget>
#include <QTableWidget>
#include <QString>
#include <QStringList>

#include "src/app/project_controller.h"
#include "src/gui/common/formula_delegate.h"
#include "src/gui/common/field_mark.h"
#include "src/gui/common/trace_segments.h"
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
#include "src/core/boundaries/boundary_data.h"
#include "src/core/loopshaping/loop_shaping_result.h"
#include "src/gui/boundaries/boundary_grid_form.h"
#include "src/gui/templates/templates_form.h"
#include "src/gui/loopshaping/loop_shaping_form.h"
#include "src/gui/application/error_message.h"
#include "src/core/common/exception.h"

using namespace qftbx;

namespace {

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

void verifyAndApply(QWidget * form)
{
    press(form, "okButton");
    press(form, "okButton");
}

BoundaryData oneBoundary()
{
    const qftbx::Trace curve{qftbx::NicholsPoint(-270.0, 10.0), qftbx::NicholsPoint(-180.0, 4.0),
                             qftbx::NicholsPoint(-90.0, 10.0)};

    return BoundaryData({{{"Stability", {curve}}}},
                        {false}, {true}, 361, qftbx::Range(-360.0, 0.0),
                        {curve}, {qftbx::TraceSet(361)},
                        121, qftbx::Range(-60.0, 60.0));
}

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

    press(&form, "okButton");
    EXPECT_FALSE(form.wasAccepted()) << "verifying is not applying";
    EXPECT_EQ(child<QPushButton>(&form, "okButton")->text(), QString("Apply"));

    press(&form, "okButton");

    ASSERT_TRUE(form.wasAccepted()) << "the form rejected valid data: "
                                    << complaintOf(&form).toStdString();

    std::unique_ptr<LtiSystem> plant(form.takePlant());
    ASSERT_NE(plant, nullptr);
    EXPECT_EQ(plant->type(), LtiSystem::SystemType::ZeroPoleGain);
    EXPECT_EQ(plant->name(), "smoke");
    EXPECT_EQ(plant->description(), "the one of the smoke test");

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
    PlantForm form;

    type(&form, "nameEdit", "path");
    check(&form, "transferFunctionRadio");
    check(&form, "zerosPolesRadio");
    check(&form, "tcgRadio");

    EXPECT_TRUE(child<QRadioButton>(&form, "transferFunctionRadio")->isChecked());
    EXPECT_TRUE(child<QRadioButton>(&form, "zerosPolesRadio")->isChecked());
    EXPECT_TRUE(child<QRadioButton>(&form, "tcgRadio")->isChecked());

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

    check(&form, "zpkRadio");
    EXPECT_EQ(figures->currentIndex(), 0);
    EXPECT_EQ(child<QPushButton>(&form, "okButton")->text(), QString("Verify"));
}

TEST_F(GuiSmoke, PlantFormRejectsAnInvalidExpression)
{
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
    ControllerForm form;

    check(&form, "zpkRadio");
    type(&form, "numeratorEdit", "z1");
    type(&form, "denominatorEdit", "p1 p2");
    type(&form, "gainStart", "1");
    type(&form, "gainEnd", "1000");

    press(&form, "uncertaintyButton");

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

    EXPECT_EQ(child<QLineEdit>(&form, "magnitudeEdit")->text(), "1.2*/");
}

TEST_F(GuiSmoke, TheSpecificationsEnteredAreListedWithTheirBound)
{
    const std::vector<double> frequencies{0.1, 1.0, 10.0, 100.0};
    SpecificationsForm form(&frequencies);

    QComboBox * typeCombo = child<QComboBox>(&form, "typeCombo");
    QTableWidget * table = child<QTableWidget>(&form, "specificationsTable");
    ASSERT_NE(typeCombo, nullptr);
    ASSERT_NE(table, nullptr);

    typeCombo->setCurrentIndex(int(qftbx::SpecificationType::Stability));
    check(&form, "constantRadio");
    check(&form, "decibelsRadio");
    type(&form, "magnitudeEdit", "3.5");

    press(&form, "addButton");
    EXPECT_EQ(child<QPushButton>(&form, "addButton")->text(), QString("Add"))
        << "verifying offers to add: " << complaintOf(&form).toStdString();
    press(&form, "addButton");

    ASSERT_EQ(table->rowCount(), 1);

    typeCombo->setCurrentIndex(int(qftbx::SpecificationType::TrackingUpper));
    check(&form, "systemRadio");
    check(&form, "polynomialRadio");
    type(&form, "numeratorEdit", "0.6584");
    type(&form, "denominatorEdit", "1 4 19.752");
    type(&form, "k", "1");

    press(&form, "addButton");
    press(&form, "addButton");

    ASSERT_EQ(table->rowCount(), 2);

    const QVariant stability = table->item(1, 2)->data(qftbx::FormulaDelegate::formulaRole);
    ASSERT_TRUE(stability.canConvert<qftbx::Formula>());
    EXPECT_EQ(qftbx::latexOf(stability.value<qftbx::Formula>()),
              std::string("\\left|\\frac{L}{1 + L}\\right| \\leq 3.5 dB"));

    EXPECT_EQ(table->item(0, 0)->text(), QString("Tracking, upper bound"));
    EXPECT_EQ(table->item(1, 0)->text(), QString("Stability"));

    const QVariant held = table->item(0, 2)->data(qftbx::FormulaDelegate::formulaRole);
    ASSERT_TRUE(held.canConvert<qftbx::Formula>());
    EXPECT_EQ(qftbx::latexOf(held.value<qftbx::Formula>()),
              std::string("\\left|F\\frac{L}{1 + L}\\right| \\leq "
                          "\\frac{0.6584}{s^{2} + 4s + 19.75}"));

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

    press(&form, "okButton");
    ASSERT_TRUE(form.wasAccepted());

    const std::optional<qftbx::SpecificationRecords> records = form.takeSpecifications();
    ASSERT_TRUE(records.has_value());
    EXPECT_TRUE(records->at(int(qftbx::SpecificationType::Stability)).used);
    EXPECT_TRUE(records->at(int(qftbx::SpecificationType::TrackingUpper)).used);
    EXPECT_FALSE(records->at(int(qftbx::SpecificationType::SensorNoise)).used);

    EXPECT_NEAR(records->at(int(qftbx::SpecificationType::Stability)).height,
                qftbx::dbToLinear(3.5), 1e-12);
}

TEST_F(GuiSmoke, ASpecificationAppliesAtTheFrequenciesThatAreTicked)
{
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

    EXPECT_DOUBLE_EQ(effort.omegaStart, 0.1);
    EXPECT_DOUBLE_EQ(effort.omegaEnd, 10.0);
    ASSERT_EQ(effort.skipped.size(), 1u);
    EXPECT_DOUBLE_EQ(effort.skipped.front(), 1.0);

    const qftbx::Specification specification =
            qftbx::toSpecification(effort, qftbx::SpecificationType::ControlEffort);
    EXPECT_TRUE(specification.appliesAt(0.1));
    EXPECT_FALSE(specification.appliesAt(1.0)) << "the frequency taken out is out";
    EXPECT_TRUE(specification.appliesAt(10.0));
    EXPECT_FALSE(specification.appliesAt(100.0)) << "and so is everything past the band";
}

TEST_F(GuiSmoke, ABoundWhoseNumeratorIsOneSaysSoWhenItIsOpenedAgain)
{
    const std::vector<double> frequencies{0.1, 1.0, 10.0};
    SpecificationsForm form(&frequencies);

    QComboBox * typeCombo = child<QComboBox>(&form, "typeCombo");
    typeCombo->setCurrentIndex(int(qftbx::SpecificationType::TrackingLower));

    check(&form, "systemRadio");
    check(&form, "polynomialRadio");
    type(&form, "numeratorEdit", "");
    type(&form, "denominatorEdit", "1 17 82 120");
    type(&form, "k", "120");
    type(&form, "delayEdit", "0");
    press(&form, "addButton");
    press(&form, "addButton");

    ASSERT_EQ(child<QTableWidget>(&form, "specificationsTable")->rowCount(), 1)
        << complaintOf(&form).toStdString();

    typeCombo->setCurrentIndex(int(qftbx::SpecificationType::ControlEffort));
    typeCombo->setCurrentIndex(int(qftbx::SpecificationType::TrackingLower));

    EXPECT_EQ(child<QLineEdit>(&form, "numeratorEdit")->text(), QString("1"))
        << "the numerator the formula draws as 1 came back as an empty field";
    EXPECT_EQ(child<QLineEdit>(&form, "denominatorEdit")->text(), QString("1 17 82 120"));
    EXPECT_EQ(child<QLineEdit>(&form, "k")->text(), QString("120"));
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
    FrequenciesForm dialog;

    child<QComboBox>(&dialog, "modeStack")->setCurrentIndex(0);

    press(&dialog, "okButton");

    EXPECT_FALSE(dialog.wasAccepted());
    EXPECT_FALSE(m_reported.empty()) << "the rejection must be reported";
    EXPECT_EQ(dialog.takeOmega(), nullptr);
}

TEST_F(GuiSmoke, FrequenciesDialogRefusesNonPositiveFrequencies)
{
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
    FrequenciesForm dialog;

    child<QComboBox>(&dialog, "modeStack")->setCurrentIndex(2);
    type(&dialog, "linStart", "1");
    type(&dialog, "linEnd", "10");

    press(&dialog, "okButton");

    EXPECT_FALSE(dialog.wasAccepted());
    EXPECT_FALSE(m_reported.empty()) << "the rejection must be reported";
}

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
    EXPECT_THROW(SpecificationsForm dialog(nullptr), qftbx::InvalidInput);

    const std::vector<double> empty;
    EXPECT_THROW(SpecificationsForm dialog(&empty), qftbx::InvalidInput);
}

TEST_F(GuiSmoke, TemplateViewerAsksItsHandlerToRecomputeTheContour)
{
    TemplateViewer viewer;

    const qftbx::CloudSet contour{{{1.0, 0.0}, {0.0, 1.0}}};
    const qftbx::CloudSet templates{{{2.0, 0.0}, {0.0, 2.0}}};
    std::vector<double> omega{1.0};
    std::vector<double> epsilon{0.05};

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

    QCheckBox * row = child<QCheckBox>(&viewer, "check");
    ASSERT_NE(row, nullptr);
    EXPECT_TRUE(row->styleSheet().startsWith("color"))
        << "the frequency row lost its colour: "
        << row->styleSheet().toStdString();
    EXPECT_FALSE(row->styleSheet().contains("colorsCreated"));
}

TEST_F(GuiSmoke, TemplateViewerWithNothingPlottedIgnoresTheRecomputeButton)
{
    TemplateViewer viewer;

    press(&viewer, "recomputeButton");
}

TEST_F(GuiSmoke, BodeViewerDrawsBothAxesOfTheDiagram)
{
    BodeViewer viewer;

    std::vector<Parameter> numerator{Parameter(1.0)};
    std::vector<Parameter> denominator{Parameter(1.0), Parameter(1.0)};
    PolynomialForm plant("bode", numerator, denominator,
                         Parameter(1.0), Parameter(0.0));

    Omega omega(-2.0, 2.0, 100, qftbx::logspace(-2.0, 2.0, 100), Omega::LogSpace);

    viewer.drawBode(&plant, &omega);

    QCustomPlot * magnitude = child<QCustomPlot>(&viewer, "magnitudePlot");
    QCustomPlot * phase = child<QCustomPlot>(&viewer, "phasePlot");
    ASSERT_NE(magnitude, nullptr);
    ASSERT_NE(phase, nullptr);

    EXPECT_EQ(magnitude->plottableCount(), 1) << "the magnitude curve is missing";
    EXPECT_EQ(phase->plottableCount(), 1) << "the phase curve is missing";

    EXPECT_GT(magnitude->xAxis->range().lower, 0.0);
    EXPECT_NEAR(magnitude->xAxis->range().lower, 0.01, 1e-9);
    EXPECT_NEAR(magnitude->xAxis->range().upper, 100.0, 1e-6);

    EXPECT_LT(phase->yAxis->range().lower, -80.0);
    EXPECT_GT(phase->yAxis->range().lower, -90.5);

    viewer.drawBode(&plant, &omega);
    EXPECT_EQ(magnitude->plottableCount(), 1) << "a redraw piled up curves";
}

TEST_F(GuiSmoke, BoundaryGridDialogBuildsTheNicholsGrid)
{
    BoundaryGridForm dialog;

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

    check(&dialog, "linspaceRadio");
    type(&dialog, "globalPointCount", "3");
    check(&dialog, "allVariablesRadio");
    check(&dialog, "nicholsRadio");

    press(&dialog, "okButton");

    ASSERT_TRUE(dialog.wasAccepted()) << "the form rejected valid data";

    const std::vector<double> epsilon = dialog.takeEpsilon();
    EXPECT_EQ(epsilon.size(), 4);
    for (double value : epsilon) {
        EXPECT_DOUBLE_EQ(value, 0.05);
    }

    const qftbx::ParameterGrids grids = dialog.grids();
    EXPECT_EQ(grids.count("a"), 1u)
        << "the uncertain parameter got no grid";
}

TEST_F(GuiSmoke, TemplatesDialogOpensWithTheProposedEpsilon)
{
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

    child<QComboBox>(&dialog, "metricCombo")->setCurrentIndex(0);
    press(&dialog, "proposeButton");
    EXPECT_EQ(calls, 2);
    EXPECT_EQ(lastMetric.metric, qftbx::HullMetric::Nichols);

    EXPECT_FALSE(child<QCheckBox>(&dialog, "borderSweepCheck")->isEnabled());
    child<QCheckBox>(&dialog, "borderSweepCheck")->setChecked(true);
    EXPECT_FALSE(dialog.borderSweep());

    EXPECT_FALSE(dialog.alphaShapeContour());
    child<QComboBox>(&dialog, "contourCombo")->setCurrentIndex(1);
    EXPECT_TRUE(dialog.alphaShapeContour());

    EXPECT_TRUE(dialog.wholeTemplateIfNoContour());
    child<QCheckBox>(&dialog, "wholeTemplateCheck")->setChecked(false);
    EXPECT_FALSE(dialog.wholeTemplateIfNoContour());

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
    UncertaintyPanel dialog;

    const CoefficientTable valueTable{{"1"}, {"a"}};
    const CoefficientTable expressionTable{{"1"}, {"a"}};
    const UncertainTable uncertainTable{{false}, {true}};

    ASSERT_TRUE(dialog.launch(valueTable, expressionTable, uncertainTable, false));

    type(&dialog, "rangeMinimum", "1");
    type(&dialog, "rangeMaximum", "5");
    type(&dialog, "rangeNominal", "3");

    type(&dialog, "rangeGainStart", "2");
    type(&dialog, "rangeGainEnd", "8");
    type(&dialog, "rangeDelayStart", "0");
    type(&dialog, "rangeDelayEnd", "0");

    press(&dialog, "applyButton");

    ASSERT_TRUE(dialog.wasAccepted()) << "the panel rejected valid data";

    ASSERT_EQ(dialog.numerator().size(), 1u);
    EXPECT_FALSE(dialog.numerator()[0].isUncertain());

    ASSERT_EQ(dialog.denominator().size(), 1u);
    Parameter & uncertain = dialog.denominator()[0];
    EXPECT_TRUE(uncertain.isUncertain());
    EXPECT_EQ(uncertain.name(), "a");
    EXPECT_DOUBLE_EQ(uncertain.rawRange().min, 1.0);
    EXPECT_DOUBLE_EQ(uncertain.rawRange().max, 5.0);
    EXPECT_DOUBLE_EQ(uncertain.rawNominal(), 3.0);

    EXPECT_DOUBLE_EQ(dialog.gain().min, 2.0);
    EXPECT_DOUBLE_EQ(dialog.gain().max, 8.0);
    EXPECT_DOUBLE_EQ(dialog.delay().min, 0.0);
    EXPECT_DOUBLE_EQ(dialog.delay().max, 0.0);
}

TEST_F(GuiSmoke, UncertaintyPanelRejectsAnEmptyRange)
{
    UncertaintyPanel dialog;

    const CoefficientTable valueTable{{"1"}, {"a"}};
    const CoefficientTable expressionTable{{"1"}, {"a"}};
    const UncertainTable uncertainTable{{false}, {true}};

    ASSERT_TRUE(dialog.launch(valueTable, expressionTable, uncertainTable, false));

    type(&dialog, "rangeGainStart", "2");
    type(&dialog, "rangeGainEnd", "8");
    type(&dialog, "rangeDelayStart", "0");
    type(&dialog, "rangeDelayEnd", "0");

    press(&dialog, "applyButton");

    EXPECT_FALSE(dialog.wasAccepted()) << "a blank range was accepted";
}

TEST_F(GuiSmoke, BoundaryViewerDrawsTheBoundariesItIsGiven)
{
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
    expectThePlotGrowsWithTheWindow<LoopShapingViewer>("loop shaping viewer");

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

TEST_F(GuiSmoke, TheFrequencyPanelGrowsWithTheTemplateViewer)
{
    TemplateViewer viewer;
    viewer.resize(700, 500);
    viewer.show();
    QCoreApplication::processEvents();

    QWidget * panel = viewer.findChild<QWidget *>("sidePanel");
    QCustomPlot * plot = viewer.findChild<QCustomPlot *>("plot");
    ASSERT_NE(panel, nullptr) << "the side column is not a widget that can be resized";
    ASSERT_NE(plot, nullptr);

    const int panelBefore = panel->width();
    const int plotBefore = plot->width();

    viewer.resize(1300, 500);
    QCoreApplication::processEvents();

    EXPECT_GT(panel->width(), panelBefore)
        << "the viewer grew by 600 px and the frequencies stayed at " << panel->width();
    EXPECT_GT(plot->width(), plotBefore) << "and the diagram has to grow as well";

    EXPECT_GT(plot->width() - plotBefore, panel->width() - panelBefore);

    QSplitter * splitter = viewer.findChild<QSplitter *>();
    ASSERT_NE(splitter, nullptr) << "the two sides cannot be traded by hand";
    EXPECT_FALSE(splitter->childrenCollapsible()) << "either side can be dragged out of sight";
}

TEST_F(GuiSmoke, ProposingAnEpsilonFillsTheFieldsAndComputesNothing)
{
    TemplateViewer viewer;

    const qftbx::CloudSet contour{{{1.0, 0.0}, {0.0, 1.0}}, {{1.0, 0.0}, {0.0, 1.0}}};
    const qftbx::CloudSet templates{{{2.0, 0.0}, {0.0, 2.0}}, {{2.0, 0.0}, {0.0, 2.0}}};
    std::vector<double> omega{1.0, 10.0};
    std::vector<double> epsilon{0.05, 0.05};

    int walked = 0;
    std::vector<double> walkedWith;
    viewer.setContourRecomputer([&](std::vector<double> eps){ ++walked; walkedWith = eps; });
    viewer.setEpsilonProposer([]{
        std::vector<qftbx::TemplateEngine::EpsilonProposal> proposals(2);
        proposals[0].epsilon = 0.25;
        proposals[0].closes = true;
        proposals[1].epsilon = 0.75;
        proposals[1].closes = true;
        return proposals;
    });

    viewer.setData(templates, contour, &omega, &epsilon);
    viewer.plotDiagram(true);

    press(&viewer, "proposeButton");

    const QList<QLineEdit *> fields = viewer.findChildren<QLineEdit *>("field");
    ASSERT_EQ(fields.size(), 2);
    EXPECT_EQ(fields.at(0)->text().toDouble(), 0.25) << "the proposal did not reach the field";
    EXPECT_EQ(fields.at(1)->text().toDouble(), 0.75);
    EXPECT_EQ(walked, 0) << "proposing walked the contours again by itself";

    press(&viewer, "recomputeButton");
    EXPECT_EQ(walked, 1);
    ASSERT_EQ(walkedWith.size(), 2u);
    EXPECT_EQ(walkedWith.at(0), 0.25);
    EXPECT_EQ(walkedWith.at(1), 0.75);
}

TEST_F(GuiSmoke, TheEpsilonOfAFrequencyIsAFieldAndNothingElse)
{
    TemplateViewer viewer;

    const qftbx::CloudSet contour{{{1.0, 0.0}, {0.0, 1.0}}};
    const qftbx::CloudSet templates{{{2.0, 0.0}, {0.0, 2.0}}};
    std::vector<double> omega{1.0};
    std::vector<double> epsilon{0.05};

    viewer.setData(templates, contour, &omega, &epsilon);
    viewer.plotDiagram(true);

    QLineEdit * field = child<QLineEdit>(&viewer, "field");
    ASSERT_NE(field, nullptr) << "the row lost the field with the epsilon";
    EXPECT_EQ(field->text().toDouble(), 0.05);
    EXPECT_EQ(viewer.findChild<QSlider *>(), nullptr) << "the row still carries a slider";
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

TEST_F(GuiSmoke, TheMainWindowBuildsItsWholeWidgetTree)
{
    MainWindow window;
    EXPECT_FALSE(window.windowTitle().isEmpty());
}

template <typename Panel>
Panel * panelIn(QWidget * window)
{
    return window->findChild<Panel *>();
}

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

    QProgressBar * progress = child<QProgressBar>(&window, "progressBar");
    ASSERT_NE(progress, nullptr);
    EXPECT_EQ(progress->value(), 1);

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

    QPushButton * plantButton = child<QPushButton>(&window, "plantButton");
    ASSERT_NE(plantButton, nullptr);
    plantButton->click();

    ASSERT_NE(panelIn<PlantForm>(&window), nullptr);

    QProgressBar * progress = child<QProgressBar>(&window, "progressBar");
    ASSERT_NE(progress, nullptr);
    EXPECT_EQ(progress->value(), 0)
        << "a step nobody accepted must not count as done";
}

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

    QPushButton * templates = child<QPushButton>(&window, "templatesButton");
    ASSERT_NE(templates, nullptr);
    EXPECT_TRUE(templates->isEnabled());

    EXPECT_TRUE(child<QPushButton>(&window, "specificationsButton")->isEnabled());

    EXPECT_FALSE(child<QPushButton>(&window, "boundariesButton")->isEnabled());
}

TEST_F(GuiSmoke, APanelRefusesToPublishWhatTheProjectHasTakenAwayFromIt)
{
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

TEST_F(GuiSmoke, TheStepButtonsAreAllTheSameHeight)
{
    MainWindow window;
    window.show();
    QCoreApplication::processEvents();

    int tallest = 0;
    int shortest = std::numeric_limits<int>::max();
    for (const char * name : {"plantButton", "frequenciesButton", "specificationsButton",
                              "templatesButton", "boundariesButton", "controllerButton",
                              "loopButton"}) {
        QPushButton * step = child<QPushButton>(&window, name);
        ASSERT_NE(step, nullptr) << name;
        tallest = std::max(tallest, step->height());
        shortest = std::min(shortest, step->height());
    }

    EXPECT_EQ(tallest, shortest)
        << "a caption on two lines makes its button taller than the rest";
}

TEST_F(GuiSmoke, EveryFormFitsTheBandItsCardGivesIt)
{
    MainWindow window;
    window.setFileChooser([](bool) {
        return QString(QFTBX_TEST_DATA_DIR "/planta1.qft");
    });
    window.findChild<QAction *>("actionOpen")->trigger();
    window.resize(1280, 900);
    window.show();
    QCoreApplication::processEvents();

    for (const char * step : {"plantButton", "frequenciesButton", "specificationsButton",
                              "templatesButton", "boundariesButton", "controllerButton",
                              "loopButton"}) {
        child<QPushButton>(&window, step)->click();
    }
    QCoreApplication::processEvents();

    for (const char * name : {"plantCard", "frequenciesCard", "templatesCard",
                              "boundariesCard", "controllerCard", "loopShapingCard"}) {
        QScrollArea * area = window.findChild<QScrollArea *>(QString(name) + "FormArea");
        ASSERT_NE(area, nullptr) << name;
        QWidget * form = area->widget();
        ASSERT_NE(form, nullptr) << name;

        EXPECT_LE(form->sizeHint().height(), area->viewport()->height())
            << name << ": the form asks for " << form->sizeHint().height()
            << " px in a band of " << area->viewport()->height()
            << ", so its button has to be scrolled to";
    }
}

TEST_F(GuiSmoke, ClosingAPhaseDoesNotThrowAwayWhatWasTypedIntoIt)
{
    MainWindow window;
    window.setFileChooser([](bool) {
        return QString(QFTBX_TEST_DATA_DIR "/planta1.qft");
    });
    window.findChild<QAction *>("actionOpen")->trigger();

    child<QPushButton>(&window, "boundariesButton")->click();

    QLineEdit * points = window.findChild<QLineEdit *>("phasePoints");
    ASSERT_NE(points, nullptr);
    const QString asStored = points->text();

    points->setText("37");

    window.findChild<QToolButton *>("boundariesCardClose")->click();
    ASSERT_TRUE(window.findChild<PhaseCard *>("boundariesCard")->isHidden());

    child<QPushButton>(&window, "boundariesButton")->click();

    EXPECT_EQ(window.findChild<QLineEdit *>("phasePoints")->text(), QString("37"))
        << "tidying the screen threw away what was written and had not been applied";
    EXPECT_NE(asStored, QString("37")) << "the test types what the project already held";
}

TEST_F(GuiSmoke, APhaseIsClosedFromItsBarAndOpenedFromItsStepButton)
{
    MainWindow window;
    window.setFileChooser([](bool) {
        return QString(QFTBX_TEST_DATA_DIR "/planta1.qft");
    });
    window.findChild<QAction *>("actionOpen")->trigger();
    window.resize(1280, 900);
    QCoreApplication::processEvents();

    QWidget * canvas = window.findChild<QWidget *>("canvasContent");
    ASSERT_NE(canvas, nullptr);

    for (const char * name : {"plantCard", "specificationsCard", "templatesCard",
                              "boundariesCard", "loopShapingCard"}) {
        PhaseCard * card = window.findChild<PhaseCard *>(name);
        ASSERT_NE(card, nullptr) << name;
        QToolButton * close = window.findChild<QToolButton *>(QString(name) + "Close");
        ASSERT_NE(close, nullptr) << name << " cannot be closed";
    }

    const int wholeCanvas = canvas->layout()->heightForWidth(canvas->width());

    for (const char * name : {"plantCard", "specificationsCard", "boundariesCard",
                              "loopShapingCard"}) {
        window.findChild<QToolButton *>(QString(name) + "Close")->click();
        EXPECT_TRUE(window.findChild<PhaseCard *>(name)->isHidden()) << name << " did not close";
    }
    QCoreApplication::processEvents();

    EXPECT_LT(canvas->layout()->heightForWidth(canvas->width()), wholeCanvas)
        << "the cards that were closed still take their room on the canvas";

    child<QPushButton>(&window, "plantButton")->click();
    PhaseCard * plant = window.findChild<PhaseCard *>("plantCard");
    EXPECT_FALSE(plant->isHidden()) << "its step button did not bring it back";
    EXPECT_TRUE(plant->isFormShown()) << "it came back folded";
    EXPECT_TRUE(window.findChild<PhaseCard *>("boundariesCard")->isHidden())
        << "opening one phase brought back the rest";
}

TEST_F(GuiSmoke, OpeningAProjectPutsItsCardsOnTheCanvasFolded)
{
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

    QWidget * canvas = window.findChild<QWidget *>("canvasContent");
    ASSERT_NE(canvas, nullptr);
    EXPECT_GE(canvas->layout()->count(), 4) << "the cards are not on the canvas";

    child<QPushButton>(&window, "plantButton")->click();
    EXPECT_TRUE(window.findChild<PhaseCard *>("plantCard")->isFormShown());
}

TEST_F(GuiSmoke, TheCanvasWrapsAndScrollsInsteadOfSqueezing)
{
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

    EXPECT_GT(content->height(), canvas->viewport()->height())
        << "the cards were squeezed into the viewport instead of wrapping below it";

    for (const char * name : {"plantCard", "templatesCard", "boundariesCard", "loopShapingCard"}) {
        PhaseCard * card = window.findChild<PhaseCard *>(name);
        ASSERT_NE(card, nullptr);
        EXPECT_GE(card->width(), 500) << name << " came out too narrow to read";
    }
}

TEST_F(GuiSmoke, ACardGrowsWhenItsFormIsUnfolded)
{
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

    for (const char * name : {"plantCard", "templatesCard", "boundariesCard", "loopShapingCard"}) {
        PhaseCard * other = window.findChild<PhaseCard *>(name);
        ASSERT_NE(other, nullptr);
        EXPECT_EQ(other->sizeHint(), other->isFormShown() ? open : folded)
            << name << " asks for a size of its own";
    }

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

TEST_F(GuiSmoke, TheTwoPhasesThatMayChangePlacesDoSoAndNoOthers)
{
    MainWindow window;
    window.setFileChooser([](bool) {
        return QString(QFTBX_TEST_DATA_DIR "/planta1.qft");
    });
    window.findChild<QAction *>("actionOpen")->trigger();
    QCoreApplication::processEvents();

    const auto orderOf = [&window]() {
        QStringList names;
        for (PhaseCard * card : window.findChildren<PhaseCard *>()) {
            names << card->objectName();
        }
        return names;
    };

    const auto placeOf = [&window](const char * name) {
        PhaseCard * card = window.findChild<PhaseCard *>(name);
        return card == nullptr ? QPoint() : card->geometry().topLeft();
    };

    window.resize(PhaseCard::unitFor(1280).width() * 2 + 64, 900);
    window.show();
    QCoreApplication::processEvents();

    ASSERT_EQ(PhaseCard::columnsFor(window.width() - 64), 2);
    EXPECT_LT(placeOf("specificationsCard").y(), placeOf("templatesCard").y())
        << "on two columns the specifications go first, whole";

    window.resize(PhaseCard::unitFor(1900).width() * 3 + 64, 900);
    QCoreApplication::processEvents();

    ASSERT_EQ(PhaseCard::columnsFor(window.width() - 64), 3);
    EXPECT_LT(placeOf("templatesCard").y(), placeOf("specificationsCard").y())
        << "on three columns the templates fill the square that was left";

    const QStringList names = orderOf();
    EXPECT_LT(names.indexOf("plantCard"), names.indexOf("frequenciesCard"));
    EXPECT_LT(names.indexOf("specificationsCard"), names.indexOf("boundariesCard"))
        << "the boundaries are computed from the specifications and never come first";
    EXPECT_LT(names.indexOf("boundariesCard"), names.indexOf("controllerCard"));
}

TEST_F(GuiSmoke, ACardDraggedByItsBarChangesPlaces)
{
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

    qftbx::Settings broken;
    broken.interface.window = "as wide as you like";
    MainWindow other(broken);
    EXPECT_GT(other.width(), 0);
}

TEST_F(GuiSmoke, TheThemeDressesTheWindowAndItsDiagrams)
{
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

    EXPECT_NEAR(QApplication::palette().color(QPalette::Highlight).hue(), 204, 12);

    qftbx::applyTheme(qftbx::kSystemTheme);
}

TEST_F(GuiSmoke, ATraceIsCutWhereItJumpsAndNotWhereItStandsUp)
{
    const auto at = [](double phase, double magnitude) {
        return qftbx::NicholsPoint(phase, magnitude);
    };

    qftbx::Trace one{at(-10, 0), at(-9, 1), at(-8, 2), at(-8, 30), at(-8, 60), at(-7, 61)};
    EXPECT_EQ(qftbx::continuousSegments(one).size(), 1u)
        << "a steep boundary was cut in two";

    qftbx::Trace two{at(-10, 0), at(-9, 0), at(-8, 0), at(-100, 5), at(-99, 5)};
    const std::vector<qftbx::Trace> cut = qftbx::continuousSegments(two);
    ASSERT_EQ(cut.size(), 2u);
    EXPECT_EQ(cut.front().size(), 3u);
    EXPECT_EQ(cut.back().size(), 2u);

    qftbx::Trace orphan{at(-10, 0), at(-9, 0), at(-8, 0), at(-125, -16.5)};
    const std::vector<qftbx::Trace> alone = qftbx::continuousSegments(orphan);
    ASSERT_EQ(alone.size(), 2u);
    EXPECT_EQ(alone.back().size(), 1u);

    const std::vector<std::size_t> ends = qftbx::segmentEnds(orphan);
    ASSERT_EQ(ends.size(), 2u);
    EXPECT_EQ(ends.front(), 3u);
    EXPECT_EQ(ends.back(), 4u);
}

TEST_F(GuiSmoke, EveryFieldSaysWhatItIsFor)
{
    MainWindow window;
    window.setFileChooser([](bool) {
        return QString(QFTBX_TEST_DATA_DIR "/planta1.qft");
    });
    window.findChild<QAction *>("actionOpen")->trigger();
    QCoreApplication::processEvents();

    const QStringList explained{"numeratorEdit", "denominatorEdit", "descriptionEdit", "check"};

    QStringList silent;

    for (QWidget * form : window.findChildren<QWidget *>()) {
        const QString kind = QString::fromLatin1(form->metaObject()->className());
        if (!kind.startsWith("qftbx::") || !kind.endsWith("Form")) {
            continue;
        }

        for (QWidget * field : form->findChildren<QWidget *>()) {
            const QString widget = QString::fromLatin1(field->metaObject()->className());
            const bool asks = widget == "QLineEdit" || widget == "QComboBox"
                    || widget == "QRadioButton" || widget == "QCheckBox"
                    || widget == "QPushButton";

            if (!asks || field->objectName().isEmpty()
                    || field->objectName().startsWith("qt_")
                    || explained.contains(field->objectName())) {
                continue;
            }

            if (field->toolTip().isEmpty()) {
                silent << kind + "::" + field->objectName();
            }
        }
    }

    EXPECT_TRUE(silent.isEmpty()) << "fields with nothing to say: "
                                  << silent.join(", ").toStdString();

    PlantForm plant;
    QLineEdit * gain = child<QLineEdit>(&plant, "gainEdit");
    ASSERT_NE(gain, nullptr);

    const QString own = gain->toolTip();
    ASSERT_FALSE(own.isEmpty());

    qftbx::markWrong(gain, true, QStringLiteral("this is the mistake"));
    EXPECT_EQ(gain->toolTip(), QStringLiteral("this is the mistake"));

    qftbx::markWrong(gain, false);
    EXPECT_EQ(gain->toolTip(), own) << "the field forgot what it is for";
}

TEST_F(GuiSmoke, TheFiguresAndTheIconAreInTheBuild)
{
    for (const char * size : {"16", "32", "64", "128", "256"}) {
        const QPixmap icon(QString(":/icons/qftbx_%1.png").arg(size));
        EXPECT_FALSE(icon.isNull()) << "the icon of " << size << " pixels is not in the build";
    }

    for (const char * figure : {"copol", "kgan", "knogan"}) {
        const QPixmap picture(QString(":/figures/%1.png").arg(figure));
        EXPECT_FALSE(picture.isNull()) << "the figure " << figure << " is not in the build";
    }

    PlantForm plant;
    check(&plant, "transferFunctionRadio");
    check(&plant, "zerosPolesRadio");
    check(&plant, "zpkRadio");
    QLabel * image = child<QLabel>(&plant, "familyImage");
    ASSERT_NE(image, nullptr);
    EXPECT_FALSE(image->pixmap().isNull()) << "the plant form has no figure in it";
}

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

    if (window.isComputing()) {
        EXPECT_TRUE(card->isBusy());
        EXPECT_NE(window.findChild<QToolButton *>("templatesCardCancel"), nullptr);
    }

    waitForComputation(window);

    EXPECT_FALSE(card->isBusy()) << "the card stayed busy after the run finished";

    QProgressBar * progress = child<QProgressBar>(&window, "progressBar");
    ASSERT_NE(progress, nullptr);
    EXPECT_EQ(progress->value(), 3) << "the templates did not reach the project: "
                                    << m_reported.join(" | ").toStdString();

    TemplateViewer * viewer = window.findChild<TemplateViewer *>();
    ASSERT_NE(viewer, nullptr);
    QCustomPlot * plot = child<QCustomPlot>(viewer, "plot");
    ASSERT_NE(plot, nullptr);
    EXPECT_GT(plot->graphCount(), 0) << "the templates were computed and not drawn";
}

TEST_F(GuiSmoke, ACancelledComputationLeavesTheProjectAsItWas)
{
    MainWindow window;

    child<QPushButton>(&window, "plantButton")->click();
    fillPlant(panelIn<PlantForm>(&window), "cancelled");
    child<QPushButton>(&window, "frequenciesButton")->click();
    fillFrequencies(panelIn<FrequenciesForm>(&window));

    child<QPushButton>(&window, "templatesButton")->click();
    TemplatesForm * templates = panelIn<TemplatesForm>(&window);
    ASSERT_NE(templates, nullptr);

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

TEST_F(GuiSmoke, ZZContours)
{
    if (qEnvironmentVariable("QFTBX_RENDER_DIR").isEmpty()) {
        GTEST_SKIP();
    }

    ProjectController project;
    project.load(std::string(QFTBX_TEST_DATA_DIR "/qft_toolbox_ex2.qft"));

    project.recomputeContour(*project.epsilon());

    const std::vector<qftbx::TemplateEngine::ContourReport> & reports = project.contourReports();
    const qftbx::CloudSet & contour = project.contour();

    for (std::size_t i = 0; i < reports.size(); ++i) {
        const std::complex<double> first = contour.at(i).front();
        const std::complex<double> last = contour.at(i).back();

        std::printf("w[%zu]: %zu points, relaxed %d, truncated %d, wholeCloud %d, "
                    "components %zu, closed %d\n",
                    i, reports[i].contourPoints, (int) reports[i].relaxed,
                    (int) reports[i].truncated, (int) reports[i].wholeCloud,
                    reports[i].components, (int) (std::abs(first - last) < 1e-12));
    }

    const std::vector<qftbx::TemplateEngine::EpsilonProposal> asked = project.proposeEpsilon();
    std::vector<double> proposed;
    for (std::size_t i = 0; i < asked.size(); ++i) {
        std::printf("   w[%zu] needs epsilon %g (connected from %g, diameter %g), closes %d\n",
                    i, asked[i].epsilon, asked[i].connected, asked[i].diameter,
                    (int) asked[i].closes);
        proposed.push_back(asked[i].epsilon);
    }

    project.recomputeContour(proposed);

    const std::vector<qftbx::TemplateEngine::ContourReport> & after = project.contourReports();
    for (std::size_t i = 0; i < after.size(); ++i) {
        const std::complex<double> first = project.contour().at(i).front();
        const std::complex<double> last = project.contour().at(i).back();
        std::printf("   after: w[%zu] %zu of %zu points, relaxed %d, components %zu, closed %d\n",
                    i, after[i].contourPoints, after[i].cloudPoints, (int) after[i].relaxed,
                    after[i].components, (int) (std::abs(first - last) < 1e-12));
    }
    std::fflush(stdout);

    project.setAlphaShapeContour(true);

    const std::vector<qftbx::TemplateEngine::EpsilonProposal> alpha = project.proposeEpsilon();
    std::vector<double> alphaEpsilon;
    for (const qftbx::TemplateEngine::EpsilonProposal & one : alpha) {
        alphaEpsilon.push_back(one.epsilon);
    }

    project.recomputeContour(alphaEpsilon);

    const std::vector<qftbx::TemplateEngine::ContourReport> & shapes = project.contourReports();
    for (std::size_t i = 0; i < shapes.size(); ++i) {
        std::printf("   alpha: w[%zu] epsilon %g, %zu of %zu points, components %zu\n",
                    i, alphaEpsilon[i], shapes[i].contourPoints, shapes[i].cloudPoints,
                    shapes[i].components);
    }

    for (const double radius : {1.0, 10.0, 100.0}) {
        const std::vector<double> same(alphaEpsilon.size(), radius);
        project.recomputeContour(same);

        const std::vector<qftbx::TemplateEngine::ContourReport> & at = project.contourReports();
        for (std::size_t i = 0; i < at.size(); ++i) {
            std::printf("   alpha e=%g: w[%zu] %zu of %zu points, components %zu\n",
                        radius, i, at[i].contourPoints, at[i].cloudPoints, at[i].components);
        }
    }
    std::fflush(stdout);

    TemplateViewer viewer;
    viewer.resize(900, 700);
    viewer.setData(project.templates(), project.contour(),
                   project.omega()->values(), project.epsilon());
    viewer.plotDiagram(true);
    viewer.grab().save(qEnvironmentVariable("QFTBX_RENDER_DIR") + "/contornos-alpha.png");
}

TEST_F(GuiSmoke, ZZFormulas)
{
    const QString out = qEnvironmentVariable("QFTBX_RENDER_DIR");
    if (out.isEmpty()) {
        GTEST_SKIP();
    }

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

TEST_F(GuiSmoke, ZZEjemploCompleto)
{
    const QString out = qEnvironmentVariable("QFTBX_RENDER_DIR");
    if (out.isEmpty()) {
        GTEST_SKIP();
    }

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

    qftbx::applyTheme(qEnvironmentVariable("QFTBX_RENDER_THEME", qftbx::kLightTheme));
    qftbx::applyLanguage(qEnvironmentVariable("QFTBX_RENDER_LANG", "en"));

    MainWindow window;
    window.setFileChooser([&out](bool) { return out + "/ejemplo-completo.qft"; });
    window.findChild<QAction *>("actionOpen")->trigger();
    window.resize(1400, 900);
    window.show();
    QCoreApplication::processEvents();

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

    window.resize(1980, 1100);
    QCoreApplication::processEvents();
    window.grab().save(out + "/ventana-tres-columnas.png");
}

TEST_F(GuiSmoke, TheSquareOfTheCanvasFollowsTheScreenItIsGiven)
{
    const QSize narrow = PhaseCard::unitFor(700);
    const QSize normal = PhaseCard::unitFor(1264);
    const QSize wide = PhaseCard::unitFor(3800);

    EXPECT_EQ(PhaseCard::columnsFor(700), 1);
    EXPECT_EQ(PhaseCard::columnsFor(1264), 2);
    EXPECT_EQ(PhaseCard::columnsFor(1900), 3);

    EXPECT_EQ(narrow.width(), 700) << "one card across takes the whole width";
    EXPECT_EQ(normal.width() * 2 + 8, 1264) << "two across share it exactly";
    EXPECT_GT(wide.width(), normal.width()) << "a wider screen gives bigger cards, not more";

    for (const QSize & unit : {narrow, normal, wide}) {
        EXPECT_GT(unit.height(), unit.width() / 2);
        EXPECT_LT(unit.height(), unit.width());
    }
}

TEST_F(GuiSmoke, TheWidthOfACardIsTheUsersAndTheFoldDoesNotTouchIt)
{
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
    EXPECT_GT(card->sizeHint().width(), one.width());
    EXPECT_GT(card->sizeHint().height(), one.height());

    const QSize wide = card->sizeHint();
    card->showForm(true);
    EXPECT_EQ(card->sizeHint().width(), wide.width()) << "the fold moved the width the user set";
    EXPECT_GT(card->sizeHint().height(), wide.height());

    card->showForm(false);
    EXPECT_EQ(card->sizeHint(), wide);

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
    MainWindow window;

    child<QPushButton>(&window, "plantButton")->click();

    PhaseCard * plant = window.findChild<PhaseCard *>("plantCard");
    ASSERT_NE(plant, nullptr) << "the plant phase has no card";
    EXPECT_NE(plant->findChild<PlantForm *>(), nullptr);
    EXPECT_NE(plant->findChild<BodeViewer *>(), nullptr)
        << "the Bode diagram belongs in the card of the plant it draws";
    EXPECT_TRUE(plant->isFormShown()) << "pressing the step opens the form";

    fillPlant(panelIn<PlantForm>(&window), "carded");
    child<QPushButton>(&window, "frequenciesButton")->click();

    PhaseCard * frequencies = window.findChild<PhaseCard *>("frequenciesCard");
    ASSERT_NE(frequencies, nullptr);
    EXPECT_NE(frequencies->findChild<FrequenciesForm *>(), nullptr);
    EXPECT_TRUE(frequencies->isFormShown());
}

TEST_F(GuiSmoke, TheBodeDiagramIsDrawnAsSoonAsThereIsSomethingToDraw)
{
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
    MainWindow window;

    QPushButton * plantButton = child<QPushButton>(&window, "plantButton");
    ASSERT_NE(plantButton, nullptr);
    plantButton->click();
    fillPlant(panelIn<PlantForm>(&window), "walked");

    QProgressBar * progress = child<QProgressBar>(&window, "progressBar");
    ASSERT_NE(progress, nullptr);
    ASSERT_EQ(progress->value(), 1);

    plantButton->click();

    EXPECT_EQ(progress->value(), 1)
        << "the plant is still in the project, so the step is still done";
}

TEST_F(GuiSmoke, OpeningAProjectRebuildsTheStepsItCarried)
{
    MainWindow window;

    window.setFileChooser([](bool forSaving) {
        EXPECT_FALSE(forSaving);
        return QString(QFTBX_TEST_DATA_DIR "/planta1.qft");
    });

    QAction * open = window.findChild<QAction *>("actionOpen");
    ASSERT_NE(open, nullptr) << "the open action is what the test drives";
    open->trigger();

    QProgressBar * progress = child<QProgressBar>(&window, "progressBar");
    ASSERT_NE(progress, nullptr);
    EXPECT_EQ(progress->value(), static_cast<int>(qftbx::kStepCount));

    EXPECT_TRUE(child<QPushButton>(&window, "templatesButton")->isEnabled());
    EXPECT_TRUE(child<QPushButton>(&window, "boundariesButton")->isEnabled());
    EXPECT_TRUE(child<QPushButton>(&window, "loopButton")->isEnabled());
}

TEST_F(GuiSmoke, ThePlantFormShowsThePlantOfTheProject)
{
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
    ProjectController project;
    project.load(std::string(QFTBX_TEST_DATA_DIR "/planta1.qft"));
    ASSERT_NE(project.plant(), nullptr);

    PlantForm dialog;
    dialog.setFromProject(project.plant());
    type(&dialog, "denominatorEdit", "2 7");

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

    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();

    QProgressBar * progress = child<QProgressBar>(&window, "progressBar");
    ASSERT_NE(progress, nullptr);
    EXPECT_EQ(progress->value(), static_cast<int>(qftbx::kStepCount));
}

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

    dialog.clearAcceptance();

    EXPECT_FALSE(dialog.wasAccepted())
        << "an acceptance must not outlive the showing it belongs to";
}

}

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

    applyLanguage("es");
    EXPECT_TRUE(aboutText().contains("Autores")) << aboutText().toStdString();
    EXPECT_TRUE(aboutText().contains(QString::fromUtf8("en desarrollo")));
    applyLanguage(kSourceLanguage);
}

TEST_F(GuiSmoke, TheFrequencyColoursDoNotRepeatOrFadeOut)
{
    for (const int count : {5, 14, 23, 40}) {
        QSet<QRgb> seen;
        for (int i = 0; i < count; ++i) {
            const QColor colour = qftbx::frequencyColour(i, count);
            EXPECT_TRUE(colour.isValid()) << count << " frequencies, index " << i;
            seen.insert(colour.rgb());

            EXPECT_LT(colour.lightness(), 225)
                << count << " frequencies, index " << i << " is too light to see on white";
        }
        EXPECT_EQ(seen.size(), count) << count << " frequencies gave " << seen.size() << " colours";
    }

    const QColor first = qftbx::frequencyColour(0, 20);
    const QColor last = qftbx::frequencyColour(19, 20);
    const int distance = std::abs(first.red() - last.red())
                       + std::abs(first.green() - last.green())
                       + std::abs(first.blue() - last.blue());
    EXPECT_GT(distance, 150) << "the ends of the sweep are too close to tell apart";
}

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

    LoopShapingViewer loop;
    expectReady(loop.findChild<QCustomPlot *>("plot"), "loop shaping viewer");

    BodeViewer bode;
    expectReady(bode.findChild<QCustomPlot *>("magnitudePlot"), "Bode magnitude");
    expectReady(bode.findChild<QCustomPlot *>("phasePlot"), "Bode phase");
}

TEST_F(GuiSmoke, TheLegendScrollsAndCanBeWorkedInOneGo)
{
    qftbx::FrequencyLegend legend;
    legend.resize(160, 200);

    for (int i = 0; i < 23; ++i) {
        legend.addRow(QString::number(i < 10 ? 0.1 * (i + 1) : i), qftbx::frequencyColour(i, 23));
    }
    ASSERT_EQ(legend.rowCount(), 23);

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

    qftbx::ExportRequest pdf;
    pdf.fileName = directory.filePath("figure.pdf");
    pdf.size = QSize(1200, 900);
    EXPECT_TRUE(qftbx::savePlot(*plot, pdf));
    EXPECT_GT(QFileInfo(pdf.fileName).size(), 0);

    qftbx::ExportRequest png;
    png.fileName = directory.filePath("figure.png");
    png.size = QSize(1600, 1200);
    EXPECT_TRUE(qftbx::savePlot(*plot, png));
    const QImage image(png.fileName);
    EXPECT_EQ(image.width(), 1600) << "the PNG came out at the window's size";

    const QColor axisBefore = plot->xAxis->labelColor();
    const double penBefore = plot->plottable(0)->pen().widthF();
    qftbx::ExportRequest again = svg;
    again.fileName = directory.filePath("twice.svg");
    EXPECT_TRUE(qftbx::savePlot(*plot, again));
    EXPECT_EQ(plot->xAxis->labelColor(), axisBefore) << "the export changed the screen";
    EXPECT_DOUBLE_EQ(plot->plottable(0)->pen().widthF(), penBefore) << "the export changed the screen";
}

TEST_F(GuiSmoke, OpeningAProjectFillsTheFormsWithWhatItHolds)
{
    MainWindow window;
    window.setFileChooser([](bool) {
        return QString(QFTBX_TEST_DATA_DIR "/qft_toolbox_ex2.qft");
    });

    QAction * open = child<QAction>(&window, "actionOpen");
    ASSERT_NE(open, nullptr);
    open->trigger();

    FrequenciesForm * frequencies = window.findChild<FrequenciesForm *>();
    ASSERT_NE(frequencies, nullptr) << "the frequencies dialog was not built";
    QLineEdit * values = child<QLineEdit>(frequencies, "manualValues");
    ASSERT_NE(values, nullptr);
    EXPECT_FALSE(values->text().isEmpty()) << "the frequencies came out empty";
    EXPECT_GE(values->text().split(' ', Qt::SkipEmptyParts).size(), 2)
        << "only one frequency was filled in";

    BoundaryGridForm * grid = window.findChild<BoundaryGridForm *>();
    ASSERT_NE(grid, nullptr) << "the boundary dialog was not built";
    QLineEdit * phasePoints = child<QLineEdit>(grid, "phasePoints");
    ASSERT_NE(phasePoints, nullptr);
    EXPECT_FALSE(phasePoints->text().isEmpty()) << "the phase count came out empty";
    EXPECT_GT(phasePoints->text().toInt(), 1) << "the phase count is not a grid";
}

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

    QPushButton * recompute = child<QPushButton>(viewer, "recomputeButton");
    ASSERT_NE(recompute, nullptr);
    recompute->click();

    EXPECT_NE(window.findChild<TemplateViewer *>(), nullptr)
        << "the viewer was destroyed while its own slot was on the stack";

    QCoreApplication::processEvents();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
}
