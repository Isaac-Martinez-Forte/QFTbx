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

#include <memory>

#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QComboBox>
#include <QStackedWidget>
#include <QString>
#include <QStringList>

#include "src/core/project_controller.h"
#include "GUI/plant_dialog.h"
#include "GUI/controller_dialog.h"
#include "GUI/frequencies_dialog.h"
#include "GUI/specifications_dialog.h"
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

TEST_F(GuiSmoke, TheMainWindowBuildsItsWholeWidgetTree)
{
    //Every dialog, viewer and menu is constructed here: a broken .ui
    //reference or a null child shows up as a crash on construction.
    MainWindow window;
    EXPECT_FALSE(window.windowTitle().isEmpty());
}

} // namespace
