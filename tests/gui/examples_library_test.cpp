/**
 * @file
 * @brief The shipped examples as the interface sees them.
 *
 * The directory has to be found wherever the packaging left it, every example
 * has to say what it is without the file being read whole, and a file of that
 * directory has to be recognised as one so the saving path never writes over
 * it. The build under test resolves the directory to the tree's own
 * `examples/`, and the tests that need another one set the environment
 * variable the resolution honours first. The dialog is driven the way a
 * person drives it, by picking a row and pressing the button. The probe that
 * renders the window into `QFTBX_RENDER_DIR` is skipped unless that variable
 * is set.
 */

#include <gtest/gtest.h>

#include <algorithm>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QElapsedTimer>
#include <QTemporaryDir>

#include <QLabel>
#include <QListWidget>
#include <QPushButton>

#include <QAction>
#include <QMenu>

#include "src/gui/application/examples_dialog.h"
#include "src/gui/application/main_window.h"
#include "src/gui/application/theme.h"
#include "src/gui/common/examples_library.h"

using namespace qftbx;

namespace {

class ExamplesLibrary : public ::testing::Test
{
protected:
    void TearDown() override
    {
        qunsetenv("QFTBX_EXAMPLES_DIR");
    }
};

}

TEST_F(ExamplesLibrary, TheDirectoryIsFoundAndHoldsTheProblems)
{
    const QString directory = examplesDirectory();

    ASSERT_FALSE(directory.isEmpty()) << "the examples were not found";
    EXPECT_TRUE(QFileInfo(directory).isDir());

    const std::vector<Example> examples = shippedExamples();
    EXPECT_GE(examples.size(), 12u);
}

TEST_F(ExamplesLibrary, EveryOneSaysWhatItIsAndWhereItIs)
{
    for (const Example & example : shippedExamples()) {
        EXPECT_FALSE(example.name.isEmpty());
        EXPECT_FALSE(example.description.isEmpty())
                << example.name.toStdString() << " carries no description";
        EXPECT_TRUE(QFileInfo(example.path).isFile())
                << example.path.toStdString();
    }
}

TEST_F(ExamplesLibrary, TheyComeOrderedByName)
{
    const std::vector<Example> examples = shippedExamples();

    for (std::size_t i = 1; i < examples.size(); ++i) {
        EXPECT_LE(examples[i - 1].name.localeAwareCompare(examples[i].name), 0)
                << examples[i - 1].name.toStdString() << " before "
                << examples[i].name.toStdString();
    }
}

TEST_F(ExamplesLibrary, AFileOfTheDirectoryIsRecognisedAndAnyOtherIsNot)
{
    const std::vector<Example> examples = shippedExamples();
    ASSERT_FALSE(examples.empty());

    EXPECT_TRUE(isShippedExample(examples.front().path));

    QTemporaryDir elsewhere;
    ASSERT_TRUE(elsewhere.isValid());
    const QString copy = QDir(elsewhere.path()).filePath("mine.qft");
    ASSERT_TRUE(QFile::copy(examples.front().path, copy));

    EXPECT_FALSE(isShippedExample(copy));
    EXPECT_FALSE(isShippedExample(QString()));
}

TEST_F(ExamplesLibrary, TheEnvironmentDecidesWhereToLook)
{
    QTemporaryDir empty;
    ASSERT_TRUE(empty.isValid());

    qputenv("QFTBX_EXAMPLES_DIR", empty.path().toUtf8());

    EXPECT_EQ(QFileInfo(examplesDirectory()).canonicalFilePath(),
              QFileInfo(empty.path()).canonicalFilePath());
    EXPECT_TRUE(shippedExamples().empty());
}

TEST_F(ExamplesLibrary, ADirectoryThatIsNotThereIsIgnored)
{
    qputenv("QFTBX_EXAMPLES_DIR", QByteArray("/nowhere/qftbx/examples"));

    EXPECT_FALSE(examplesDirectory().isEmpty())
            << "a path that does not exist must fall through to the next one";
    EXPECT_FALSE(shippedExamples().empty());
}

TEST_F(ExamplesLibrary, TheDialogListsThemAndAnswersWithAPath)
{
    ExamplesDialog dialog;

    QListWidget * list = dialog.findChild<QListWidget *>("examplesList");
    QLabel * description = dialog.findChild<QLabel *>("exampleDescription");
    QPushButton * open = dialog.findChild<QPushButton *>("openExampleButton");
    ASSERT_NE(list, nullptr);
    ASSERT_NE(description, nullptr);
    ASSERT_NE(open, nullptr);

    const std::vector<Example> examples = shippedExamples();
    ASSERT_FALSE(examples.empty());
    EXPECT_EQ(list->count(), static_cast<int>(examples.size()));
    EXPECT_EQ(list->item(0)->text(), examples.front().name);

    QLabel * title = dialog.findChild<QLabel *>("exampleName");
    ASSERT_NE(title, nullptr);
    EXPECT_EQ(title->text(), examples.front().name);
    EXPECT_FALSE(description->text().isEmpty()) << "the first one is described on opening";
    EXPECT_TRUE(open->isEnabled());

    list->setCurrentRow(1);
    EXPECT_EQ(title->text(), examples[1].name);
    EXPECT_EQ(description->text(), examples[1].description);

    open->click();
    EXPECT_EQ(dialog.result(), QDialog::Accepted);
    EXPECT_EQ(dialog.chosenPath(), examples[1].path);
}

TEST_F(ExamplesLibrary, TheDialogSaysWhereItLookedWhenThereIsNothing)
{
    QTemporaryDir empty;
    ASSERT_TRUE(empty.isValid());
    qputenv("QFTBX_EXAMPLES_DIR", empty.path().toUtf8());

    ExamplesDialog dialog;

    QListWidget * list = dialog.findChild<QListWidget *>("examplesList");
    ASSERT_NE(list, nullptr);
    EXPECT_EQ(list->count(), 0);

    QLabel * missing = dialog.findChild<QLabel *>("examplesMissing");
    QLabel * where = dialog.findChild<QLabel *>("examplesDirectory");
    ASSERT_NE(missing, nullptr);
    ASSERT_NE(where, nullptr);
    EXPECT_EQ(where->text(), QFileInfo(empty.path()).canonicalFilePath());

    QPushButton * open = dialog.findChild<QPushButton *>("openExampleButton");
    ASSERT_NE(open, nullptr);
    EXPECT_FALSE(open->isEnabled());

    EXPECT_TRUE(dialog.chosenPath().isEmpty());
}

TEST_F(ExamplesLibrary, TheMenuOffersEveryExampleAndTheWayToBrowseThem)
{
    MainWindow window;

    QMenu * menu = window.findChild<QMenu *>("menuExamples");
    ASSERT_NE(menu, nullptr) << "the window has no Examples menu";
    EXPECT_TRUE(menu->isEnabled());

    QAction * browse = window.findChild<QAction *>("actionBrowseExamples");
    ASSERT_NE(browse, nullptr);
    EXPECT_FALSE(browse->text().isEmpty());

    const std::vector<Example> examples = shippedExamples();
    ASSERT_FALSE(examples.empty());

    QStringList offered;
    for (QAction * action : menu->actions()) {
        if (action != browse && !action->isSeparator()) {
            offered << action->text();
            EXPECT_FALSE(action->toolTip().isEmpty())
                    << action->text().toStdString() << " is offered with no description";
        }
    }

    ASSERT_EQ(offered.size(), static_cast<int>(examples.size()));
    for (std::size_t i = 0; i < examples.size(); ++i) {
        EXPECT_EQ(offered.at(static_cast<int>(i)), examples[i].name);
    }
}

TEST_F(ExamplesLibrary, ADoiBecomesAnAddressAndAnythingElseDoesNot)
{
    EXPECT_EQ(doiAddress("10.1002/rnc.1085"), "https://doi.org/10.1002/rnc.1085");
    EXPECT_EQ(doiAddress("10.1115/1.2802481"), "https://doi.org/10.1115/1.2802481");

    for (const char * refused : {"", "rnc.1085", "10/rnc", "10.1002", "10.1002/",
                                 "https://example.com/", "10.1002/a b",
                                 "javascript:alert(1)", "10.1002/<script>"}) {
        EXPECT_TRUE(doiAddress(refused).isEmpty())
                << refused << " was turned into an address";
    }
}

TEST_F(ExamplesLibrary, TheSourceIsOfferedAsALinkWhereThereIsOne)
{
    const std::vector<Example> examples = shippedExamples();
    ASSERT_FALSE(examples.empty());

    const auto withDoi = std::find_if(examples.begin(), examples.end(),
                                      [](const Example & e) { return !e.doi.isEmpty(); });
    const auto without = std::find_if(examples.begin(), examples.end(),
                                      [](const Example & e) { return e.doi.isEmpty(); });
    ASSERT_NE(withDoi, examples.end()) << "no example names its article";
    ASSERT_NE(without, examples.end()) << "every example names one, so the other case is untested";

    ExamplesDialog cited(nullptr, withDoi->path);
    QLabel * source = cited.findChild<QLabel *>("exampleSource");
    ASSERT_NE(source, nullptr);
    EXPECT_TRUE(source->isVisibleTo(&cited));
    EXPECT_TRUE(source->text().contains(withDoi->doi));
    EXPECT_TRUE(source->text().contains("https://doi.org/" + withDoi->doi));
    EXPECT_TRUE(source->openExternalLinks());

    ExamplesDialog uncited(nullptr, without->path);
    QLabel * quiet = uncited.findChild<QLabel *>("exampleSource");
    ASSERT_NE(quiet, nullptr);
    EXPECT_FALSE(quiet->isVisibleTo(&uncited)) << "an example with no article shows an empty line";
}

TEST_F(ExamplesLibrary, TheDialogOpensOnTheOneItIsGiven)
{
    const std::vector<Example> examples = shippedExamples();
    ASSERT_GE(examples.size(), 3u);

    ExamplesDialog dialog(nullptr, examples[2].path);

    QListWidget * list = dialog.findChild<QListWidget *>("examplesList");
    QLabel * title = dialog.findChild<QLabel *>("exampleName");
    ASSERT_NE(list, nullptr);
    ASSERT_NE(title, nullptr);

    EXPECT_EQ(list->currentRow(), 2);
    EXPECT_EQ(title->text(), examples[2].name);
}

TEST_F(ExamplesLibrary, ThePreviewReadsNothingButTheHeadOfEachFile)
{
    const std::vector<Example> examples = shippedExamples();
    ASSERT_FALSE(examples.empty());

    qint64 weight = 0;
    for (const Example & example : examples) {
        weight += QFileInfo(example.path).size();
    }
    ASSERT_GT(weight, 1000000) << "the examples are too light for this to mean anything";

    QElapsedTimer clock;
    clock.start();
    ExamplesDialog dialog;
    const qint64 elapsed = clock.elapsed();

    QListWidget * list = dialog.findChild<QListWidget *>("examplesList");
    ASSERT_NE(list, nullptr);
    EXPECT_EQ(list->count(), static_cast<int>(examples.size()));
    EXPECT_LT(elapsed, 1000) << "listing them took " << elapsed << " ms";
}

TEST_F(ExamplesLibrary, SavingAnExampleAsksWhereInsteadOfWritingOverIt)
{
    const std::vector<Example> examples = shippedExamples();
    ASSERT_FALSE(examples.empty());

    const QString shipped = examples.front().path;
    const QDateTime before = QFileInfo(shipped).lastModified();

    MainWindow window;

    QTemporaryDir mine;
    ASSERT_TRUE(mine.isValid());
    const QString elsewhere = QDir(mine.path()).filePath("mine.qft");

    int asked = 0;
    window.setFileChooser([&asked, shipped, elsewhere](bool forSaving) {
        ++asked;
        return forSaving ? elsewhere : shipped;
    });

    QAction * open = window.findChild<QAction *>("actionOpen");
    ASSERT_NE(open, nullptr);
    open->trigger();
    ASSERT_EQ(asked, 1);

    QAction * save = window.findChild<QAction *>("actionSave");
    ASSERT_NE(save, nullptr);
    save->trigger();

    EXPECT_EQ(asked, 2) << "Save wrote without asking where";
    EXPECT_TRUE(QFileInfo(elsewhere).isFile()) << "the copy was not written";
    EXPECT_EQ(QFileInfo(shipped).lastModified(), before)
            << "the shipped example was written to";
}

TEST_F(ExamplesLibrary, ZZExamplesDialog)
{
    const QString out = qEnvironmentVariable("QFTBX_RENDER_DIR");
    if (out.isEmpty()) {
        GTEST_SKIP();
    }

    const std::vector<Example> examples = shippedExamples();
    ASSERT_FALSE(examples.empty());

    for (const QString & theme : {qftbx::kLightTheme, qftbx::kDarkTheme}) {
        qftbx::applyTheme(theme);

        ExamplesDialog first(nullptr, examples.front().path);
        first.show();
        first.grab().save(out + "/ejemplos-" + theme + "-primero.png");

        const Example & longest = *std::max_element(
                    examples.begin(), examples.end(),
                    [](const Example & a, const Example & b) {
            return a.description.size() < b.description.size();
        });

        ExamplesDialog worst(nullptr, longest.path);
        worst.show();
        worst.grab().save(out + "/ejemplos-" + theme + "-mas-largo.png");
    }
}
