#ifndef QFTBX_GUI_LANGUAGE_H
#define QFTBX_GUI_LANGUAGE_H

#include <string>

#include <QString>
#include <QStringList>

/**
 * @file
 * @brief The language of the interface: a setting, chosen in the View menu,
 * applied with QTranslator.
 *
 * A language is its code CODE: "system" for the machine's, "en" for the
 * language the interface is written in, and the code of every translation
 * compiled into the application (src/gui/translations/qftbx_CODE.ts, in
 * the resources as :/i18n/qftbx_CODE.qm). Nothing here names a language:
 * adding one is adding its file, see src/gui/translations/README.md.
 * availableLanguages() lists them in that order, and languageName() names
 * each in its own language, the system one in the language of the moment.
 *
 * Two translators are installed for a language: the application's own and
 * Qt's, for its standard dialogs and buttons. applyLanguage() replaces
 * those of the previous language and returns the code effectively shown:
 * "system" resolves to the machine's language when a translation for it
 * exists and to "en" otherwise, and an unknown code counts as "system".
 * Installing them makes Qt send a LanguageChange event to every window; the
 * main window retranslates itself on it, and the dialogs are built when
 * they open. storeLanguage() keeps the choice as interface.language in the
 * settings file the application read, or in the user's own
 * (userSettingsPath()) when it read none, so the next start reads it.
 */
namespace qftbx {

inline const QString kSystemLanguage = QStringLiteral("system");
inline const QString kSourceLanguage = QStringLiteral("en");

QStringList availableLanguages();

bool isAvailableLanguage(const QString & code);

QString languageName(const QString & code);

QString applyLanguage(const QString & code);

QString currentLanguage();

void storeLanguage(const QString & code, const std::string & settingsPath);

}

#endif
