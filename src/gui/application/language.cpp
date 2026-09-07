#include "src/gui/application/language.h"

#include <QCoreApplication>
#include <QEvent>
#include <QLibraryInfo>
#include <QLocale>
#include <QSettings>
#include <QTranslator>

namespace qftbx {

namespace {

const char * kSettingsKey = "interface/language";

//Owned by the application object; replaced when the language changes.
QTranslator * g_application = nullptr;
QTranslator * g_qt = nullptr;
Language g_current = Language::English;

Language resolved(Language language)
{
    if (language != Language::System) {
        return language;
    }
    return QLocale::system().language() == QLocale::Spanish ? Language::Spanish : Language::English;
}

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

} // namespace

QString languageCode(Language language)
{
    switch (language) {
    case Language::System: return QStringLiteral("system");
    case Language::English: return QStringLiteral("en");
    case Language::Spanish: return QStringLiteral("es");
    }
    return QStringLiteral("system");
}

Language languageFromCode(const QString & code)
{
    if (code == QLatin1String("en")) {
        return Language::English;
    }
    if (code == QLatin1String("es")) {
        return Language::Spanish;
    }
    return Language::System;
}

QString languageName(Language language)
{
    switch (language) {
    case Language::System: return QCoreApplication::translate("Language", "System language");
    case Language::English: return QStringLiteral("English");
    case Language::Spanish: return QStringLiteral("Español");
    }
    return QString();
}

Language storedLanguage()
{
    QSettings settings(QStringLiteral("QFTbx"), QStringLiteral("QFTbx"));
    return languageFromCode(settings.value(kSettingsKey, QStringLiteral("system")).toString());
}

void storeLanguage(Language language)
{
    QSettings settings(QStringLiteral("QFTbx"), QStringLiteral("QFTbx"));
    settings.setValue(kSettingsKey, languageCode(language));
}

Language currentLanguage()
{
    return g_current;
}

Language applyLanguage(Language language)
{
    const Language shown = resolved(language);
    dropTranslators();

    if (shown == Language::Spanish) {
        //Qt's own first, so the application's takes precedence where both
        //have a text (the last installed is consulted first).
        auto * qt = new QTranslator(QCoreApplication::instance());
        if (qt->load(QStringLiteral("qtbase_es"), QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
            QCoreApplication::installTranslator(qt);
            g_qt = qt;
        } else {
            delete qt;
        }
        auto * application = new QTranslator(QCoreApplication::instance());
        if (application->load(QStringLiteral(":/i18n/qftbx_es.qm"))) {
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

} // namespace qftbx
