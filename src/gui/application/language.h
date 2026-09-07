#ifndef QFTBX_GUI_LANGUAGE_H
#define QFTBX_GUI_LANGUAGE_H

#include <string>

#include <QString>
#include <QStringList>

/**
 * @brief The language of the interface: a setting, chosen in the View menu,
 * applied with QTranslator.
 *
 * A language is its code: "system" for the machine's, "en" for the
 * language the interface is written in, and the code of every translation
 * compiled into the application (src/gui/translations/qftbx_<code>.ts, in
 * the resources as :/i18n/qftbx_<code>.qm). Nothing here names a language:
 * adding one is adding its file, see src/gui/translations/README.md.
 *
 * Two translators are installed for a language: the application's own and
 * Qt's, for its standard dialogs and buttons. Installing them makes Qt send
 * a LanguageChange event to every window; the main window retranslates
 * itself on it, and the dialogs are built when they open. The choice is
 * kept in the settings file (interface.language), so the next start reads
 * it.
 */
namespace qftbx {

/// The code of the machine's language.
inline const QString kSystemLanguage = QStringLiteral("system");
/// The language the interface is written in: always available, no
/// translation needed.
inline const QString kSourceLanguage = QStringLiteral("en");

/// "system", "en", and one code per translation compiled in, in that order.
QStringList availableLanguages();

/// Whether a code names a language the interface can show.
bool isAvailableLanguage(const QString & code);

/// The name of a language in that language, for the menu; the system
/// language in the language of the moment.
QString languageName(const QString & code);

/// Installs the translators of the language (removing those of the
/// previous one) and returns the code effectively shown: "system"
/// resolves to the machine's language when a translation for it exists and
/// to "en" otherwise; an unknown code is treated as "system".
QString applyLanguage(const QString & code);

/// The code shown now ("en", "es", ...).
QString currentLanguage();

/// Writes the choice into a settings file as interface.language: the file
/// the application read its settings from, or the user's own when it read
/// none (qftbx::userSettingsPath()).
void storeLanguage(const QString & code, const std::string & settingsPath);

} // namespace qftbx

#endif // QFTBX_GUI_LANGUAGE_H
