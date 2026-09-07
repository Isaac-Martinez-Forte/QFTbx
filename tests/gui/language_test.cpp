// The interface language: the preference, the translators and the
// retranslation of the main window, headless.
#include <gtest/gtest.h>

#include <QFile>
#include <QMenu>
#include <QTranslator>

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
