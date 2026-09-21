/**
 * @file
 * @brief Installs and resolves the translators of the interface language.
 *
 * The code `system` resolves against the translations compiled in: the
 * machine's language with its region first, then without it, then the
 * source language. Qt's own translator is installed before the
 * application's, so the application's wins where both have a text. Both
 * are owned here rather than parented to the application: one that fails
 * to load is dropped at once, and one replaced is removed and freed in the
 * same step. A translation names its own language by translating the name
 * of the source language; one that has not is named by Qt's locale. The
 * change events that installing posts are delivered before returning, so
 * the caller sees the interface already in the new language.
 */

#include "src/gui/application/language.h"

#include <QCoreApplication>
#include <QDir>
#include <QEvent>
#include <QLibraryInfo>
#include <QLocale>
#include <QTranslator>

#include <memory>

#include "src/core/project/settings.h"

namespace qftbx {

namespace {

const QString kResourceDirectory = QStringLiteral(":/i18n");
const char * const kSourceLanguageName = QT_TRANSLATE_NOOP("Language", "English");
const QString kFilePrefix = QStringLiteral("qftbx_");

std::unique_ptr<QTranslator> g_application;
std::unique_ptr<QTranslator> g_qt;
QString g_current = kSourceLanguage;

void dropTranslators()
{
    for (std::unique_ptr<QTranslator> * translator : {&g_application, &g_qt}) {
        if (*translator != nullptr) {
            QCoreApplication::removeTranslator(translator->get());
            translator->reset();
        }
    }
}

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

}

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
        auto qt = std::make_unique<QTranslator>();
        if (qt->load(QStringLiteral("qtbase_") + shown, QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
            QCoreApplication::installTranslator(qt.get());
            g_qt = std::move(qt);
        }

        auto application = std::make_unique<QTranslator>();
        if (application->load(kResourceDirectory + "/" + kFilePrefix + shown + ".qm")) {
            QCoreApplication::installTranslator(application.get());
            g_application = std::move(application);
        }
    }

    g_current = shown;

    QCoreApplication::sendPostedEvents(nullptr, QEvent::LanguageChange);
    return shown;
}

void storeLanguage(const QString & code, const std::string & settingsPath)
{
    writeSetting(settingsPath.empty() ? userSettingsPath() : settingsPath, "interface.language", code.toStdString());
}

}
