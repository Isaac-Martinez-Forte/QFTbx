#include "src/gui/application/language.h"

#include <QCoreApplication>
#include <QDir>
#include <QEvent>
#include <QLibraryInfo>
#include <QLocale>
#include <QTranslator>

#include "src/core/project/settings.h"

namespace qftbx {

namespace {

const QString kResourceDirectory = QStringLiteral(":/i18n");
//The name of the source language, which every translation translates into
//the name of its own: "Español", "Français"...
const char * const kSourceLanguageName = QT_TRANSLATE_NOOP("Language", "English");
const QString kFilePrefix = QStringLiteral("qftbx_");

//Owned by the application object; replaced when the language changes.
QTranslator * g_application = nullptr;
QTranslator * g_qt = nullptr;
QString g_current = kSourceLanguage;

void dropTranslators()
{
    for (QTranslator ** translator : {&g_application, &g_qt}) {
        if (*translator != nullptr) {
            QCoreApplication::removeTranslator(*translator);
            delete *translator;
            *translator = nullptr;
        }
    }
}

//The code "system" resolved against what is available: the machine's
//language with its region ("es_ES"), then without it ("es"), then the
//source language.
QString resolved(const QString & code)
{
    const QStringList available = availableLanguages();
    if (code != kSystemLanguage && available.contains(code)) {
        return code;
    }
    const QString system = QLocale::system().name();
    if (available.contains(system)) {
        return system;
    }
    const QString language = system.section('_', 0, 0);
    if (available.contains(language)) {
        return language;
    }
    return kSourceLanguage;
}

} // namespace

QStringList availableLanguages()
{
    QStringList codes{kSystemLanguage, kSourceLanguage};
    const QDir resources(kResourceDirectory);
    for (const QString & file : resources.entryList({kFilePrefix + "*.qm"}, QDir::Files, QDir::Name)) {
        const QString code = file.mid(kFilePrefix.size(), file.size() - kFilePrefix.size() - 3);
        if (!codes.contains(code)) {
            codes << code;
        }
    }
    return codes;
}

bool isAvailableLanguage(const QString & code)
{
    return availableLanguages().contains(code);
}

QString languageName(const QString & code)
{
    if (code == kSystemLanguage) {
        return QCoreApplication::translate("Language", "System language");
    }
    if (code == kSourceLanguage) {
        return QString::fromLatin1(kSourceLanguageName);
    }
    //Each translation names its own language: the text below, translated.
    //A translation that has not done so is named by Qt, region and all.
    QTranslator translation;
    if (translation.load(kResourceDirectory + "/" + kFilePrefix + code + ".qm")) {
        const QString name = translation.translate("Language", kSourceLanguageName);
        if (!name.isEmpty()) {
            return name;
        }
    }
    QString name = QLocale(code).nativeLanguageName();
    if (name.isEmpty()) {
        return code;
    }
    name[0] = name[0].toUpper();
    return name;
}

QString currentLanguage()
{
    return g_current;
}

QString applyLanguage(const QString & code)
{
    const QString shown = resolved(code);
    dropTranslators();

    if (shown != kSourceLanguage) {
        //Qt's own first, so the application's takes precedence where both
        //have a text (the last installed is consulted first).
        auto * qt = new QTranslator(QCoreApplication::instance());
        if (qt->load(QStringLiteral("qtbase_") + shown, QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
            QCoreApplication::installTranslator(qt);
            g_qt = qt;
        } else {
            delete qt;
        }
        auto * application = new QTranslator(QCoreApplication::instance());
        if (application->load(kResourceDirectory + "/" + kFilePrefix + shown + ".qm")) {
            QCoreApplication::installTranslator(application);
            g_application = application;
        } else {
            delete application;
        }
    }

    g_current = shown;

    //Installing a translator posts a LanguageChange to every window; it is
    //delivered here and now, so the caller sees the interface in the new
    //language when this returns.
    QCoreApplication::sendPostedEvents(nullptr, QEvent::LanguageChange);
    return shown;
}

void storeLanguage(const QString & code, const std::string & settingsPath)
{
    writeSetting(settingsPath.empty() ? userSettingsPath() : settingsPath, "interface.language", code.toStdString());
}

} // namespace qftbx
