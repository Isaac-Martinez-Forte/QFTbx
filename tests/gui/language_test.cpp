// The interface language: the preference, the translators and the
// retranslation of the main window, headless.
#include <gtest/gtest.h>

#include <QFile>
#include <QMenu>
#include <QTranslator>

#include "src/core/common/exception.h"
#include "src/gui/application/error_message.h"
#include "src/gui/application/language.h"
#include "src/gui/application/main_window.h"

using namespace qftbx;

TEST(Language, CodesRoundTrip)
{
    for (const Language language : {Language::System, Language::English, Language::Spanish}) {
        EXPECT_EQ(languageFromCode(languageCode(language)), language);
    }
    EXPECT_EQ(languageFromCode("fr"), Language::System) << "an unknown code falls back to the system";
    EXPECT_EQ(languageName(Language::Spanish), QString::fromUtf8("Español"));
}

TEST(Language, TheSpanishTranslationIsCompleteAndLoads)
{
    QTranslator translator;
    ASSERT_TRUE(translator.load(QStringLiteral(":/i18n/qftbx_es.qm"))) << "the .qm is in the resources";
    EXPECT_FALSE(translator.isEmpty());
    EXPECT_EQ(translator.translate("qftbx::MainWindow", "&Language"), QString::fromUtf8("&Idioma"));

    //Every string the sources carry has a Spanish text: lupdate marks the
    //ones that do not as unfinished, and none may be.
    QFile source(QStringLiteral(QFTBX_SOURCE_DIR "/src/gui/translations/qftbx_es.ts"));
    ASSERT_TRUE(source.open(QIODevice::ReadOnly));
    const QByteArray text = source.readAll();
    EXPECT_FALSE(text.contains("type=\"unfinished\"")) << "a string was added to the interface and not translated";
    EXPECT_FALSE(text.contains("type=\"obsolete\"")) << "a string left the interface and stayed in the translation";
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

    applyLanguage(Language::English);
    EXPECT_EQ(file->title(), QStringLiteral("&File"));

    EXPECT_EQ(applyLanguage(Language::Spanish), Language::Spanish);
    EXPECT_EQ(currentLanguage(), Language::Spanish);
    EXPECT_EQ(file->title(), QStringLiteral("Ar&chivo")) << "the form retranslated itself";
    EXPECT_EQ(window.windowTitle(), QString::fromUtf8("QFT: teoría de la realimentación cuantitativa"));

    applyLanguage(Language::English);
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

    applyLanguage(Language::English);
    EXPECT_EQ(translated(cells), QStringLiteral("The contours need one epsilon per design frequency: 3 given for 5 frequencies."));
    EXPECT_EQ(translated(malformed), QStringLiteral("project.qft: missing <plant> element (line 12)"));

    applyLanguage(Language::Spanish);
    EXPECT_EQ(translated(cells), QString::fromUtf8("Los contornos necesitan un épsilon por frecuencia de diseño: se dieron 3 para 5 frecuencias."));
    EXPECT_EQ(translated(malformed), QString::fromUtf8("project.qft: falta el elemento <plant> (línea 12)"));
    EXPECT_EQ(translated(plain), QStringLiteral("a text nobody translates")) << "a plain text has no translation to give";
    EXPECT_EQ(translated(std::runtime_error("elsewhere")), QStringLiteral("elsewhere"));

    applyLanguage(Language::English);
}
