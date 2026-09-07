// The interface language: the preference, the translators and the
// retranslation of the main window, headless.
#include <gtest/gtest.h>

#include <QAction>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QMenu>
#include <QTranslator>

#include "src/core/common/exception.h"
#include "src/gui/application/error_message.h"
#include "src/core/project/settings.h"
#include "src/gui/application/language.h"
#include "src/gui/application/main_window.h"

using namespace qftbx;

TEST(Language, TheAvailableLanguagesComeFromTheCompiledTranslations)
{
    const QStringList languages = availableLanguages();
    EXPECT_EQ(languages.first(), kSystemLanguage);
    EXPECT_TRUE(languages.contains(kSourceLanguage));
    EXPECT_TRUE(languages.contains("es")) << "the Spanish translation is compiled in";
    EXPECT_TRUE(isAvailableLanguage("es"));
    EXPECT_FALSE(isAvailableLanguage("fr")) << "no French translation yet";
    EXPECT_EQ(languageName("es"), QString::fromUtf8("Español")) << "the translation names its own language";
    EXPECT_EQ(languageName("en"), QStringLiteral("English"));
    EXPECT_FALSE(languageName("fr").isEmpty()) << "a code is named even before it is translated";
}

TEST(Language, EveryTranslationIsCompleteAndLoads)
{
    QTranslator translator;
    ASSERT_TRUE(translator.load(QStringLiteral(":/i18n/qftbx_es.qm"))) << "the .qm is in the resources";
    EXPECT_FALSE(translator.isEmpty());
    EXPECT_EQ(translator.translate("qftbx::MainWindow", "&Language"), QString::fromUtf8("&Idioma"));

    //Every string the sources carry has a text in every language: lupdate
    //marks the ones that do not as unfinished, and none may be.
    QDir folder(QStringLiteral(QFTBX_SOURCE_DIR "/src/gui/translations"));
    const QStringList files = folder.entryList({"*.ts"}, QDir::Files);
    ASSERT_FALSE(files.isEmpty());
    for (const QString & name : files) {
        QFile source(folder.filePath(name));
        ASSERT_TRUE(source.open(QIODevice::ReadOnly));
        const QByteArray text = source.readAll();
        EXPECT_FALSE(text.contains("type=\"unfinished\"")) << name.toStdString() << ": a string was added to the interface and not translated";
        EXPECT_FALSE(text.contains("type=\"obsolete\"")) << name.toStdString() << ": a string left the interface and stayed in the translation";
    }
}

TEST(Language, TheChoiceIsWrittenToTheSettingsFile)
{
    QTemporaryDir directory;
    const std::string path = directory.filePath("qftbx.conf").toStdString();
    storeLanguage("es", path);
    EXPECT_EQ(readSettings(path).interface.language, "es");
    storeLanguage(kSystemLanguage, path);
    EXPECT_EQ(readSettings(path).interface.language, "system");
}

TEST(Language, TheMenuStartsOnTheLanguageOfTheSettings)
{
    Settings settings;
    settings.interface.language = "es";
    MainWindow window(settings);
    auto * spanish = window.findChild<QAction *>("actionLanguage_es");
    auto * system = window.findChild<QAction *>("actionLanguage_system");
    ASSERT_NE(spanish, nullptr);
    ASSERT_NE(system, nullptr);
    EXPECT_TRUE(spanish->isChecked());
    EXPECT_FALSE(system->isChecked());

    Settings unknown;
    unknown.interface.language = "fr";
    MainWindow other(unknown);
    EXPECT_TRUE(other.findChild<QAction *>("actionLanguage_system")->isChecked()) << "a language not compiled in falls back to the system's";
}

TEST(Language, TheMainWindowRetranslatesWhenTheLanguageChanges)
{
    MainWindow window;
    QMenu * file = nullptr;
    for (QMenu * menu : window.findChildren<QMenu *>()) {
        if (menu->objectName() == QLatin1String("menuFile")) {
            file = menu;
        }
    }
    ASSERT_NE(file, nullptr);

    applyLanguage(kSourceLanguage);
    EXPECT_EQ(file->title(), QStringLiteral("&File"));

    EXPECT_EQ(applyLanguage("es"), QStringLiteral("es"));
    EXPECT_EQ(currentLanguage(), QStringLiteral("es"));
    EXPECT_EQ(file->title(), QStringLiteral("Ar&chivo")) << "the form retranslated itself";
    EXPECT_EQ(window.windowTitle(), QString::fromUtf8("QFT: teoría de la realimentación cuantitativa"));

    applyLanguage(kSourceLanguage);
    EXPECT_EQ(file->title(), QStringLiteral("&File"));
    EXPECT_EQ(window.windowTitle(), QStringLiteral("QFT: Quantitative feedback theory"));
}

TEST(Language, TheCoreMessagesAreTranslatedAtTheBoundary)
{
    //A message thrown by the core in English, with its arguments apart,
    //reaches the user in the language of the interface with the same
    //arguments in place; a parse error keeps its file and line around it.
    const InvalidInput cells(QFTBX_TR("Core", "The contours need one epsilon per design frequency: %1 given for %2 frequencies.").arg(3).arg(5));
    const ParseError malformed(QFTBX_TR("Core", "missing <%1> element").arg("plant"), 12, "project.qft");
    const FileError plain("a text nobody translates");

    applyLanguage(kSourceLanguage);
    EXPECT_EQ(translated(cells), QStringLiteral("The contours need one epsilon per design frequency: 3 given for 5 frequencies."));
    EXPECT_EQ(translated(malformed), QStringLiteral("project.qft: missing <plant> element (line 12)"));

    applyLanguage("es");
    EXPECT_EQ(translated(cells), QString::fromUtf8("Los contornos necesitan un épsilon por frecuencia de diseño: se dieron 3 para 5 frecuencias."));
    EXPECT_EQ(translated(malformed), QString::fromUtf8("project.qft: falta el elemento <plant> (línea 12)"));
    EXPECT_EQ(translated(plain), QStringLiteral("a text nobody translates")) << "a plain text has no translation to give";
    EXPECT_EQ(translated(std::runtime_error("elsewhere")), QStringLiteral("elsewhere"));

    applyLanguage(kSourceLanguage);
}
