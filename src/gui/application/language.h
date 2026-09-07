#ifndef QFTBX_GUI_LANGUAGE_H
#define QFTBX_GUI_LANGUAGE_H

#include <QString>

/**
 * @brief The language of the interface: a preference of the GUI, chosen in
 * the View menu, kept with QSettings and applied with QTranslator.
 *
 * Two translators are installed for a language: the application's own,
 * compiled from src/gui/translations into the resources, and Qt's for its
 * standard dialogs and buttons. Installing them makes Qt send a
 * LanguageChange event to every window, and the main window retranslates
 * itself on it; the dialogs are built when they open and come out in the
 * language of the moment. The core speaks English: what an exception
 * says reaches the user as it was thrown.
 */
namespace qftbx {

enum class Language { System, English, Spanish };

/// The preference, "system" when none was ever chosen.
Language storedLanguage();
void storeLanguage(Language language);

/// Installs the translators of the language (removing those of the
/// previous one) and returns the language effectively shown: System
/// resolves to Spanish on a Spanish locale and to English elsewhere.
Language applyLanguage(Language language);

/// The language shown now.
Language currentLanguage();

/// The name of a language in that language, for the menu.
QString languageName(Language language);

/// "system", "en", "es".
QString languageCode(Language language);
Language languageFromCode(const QString & code);

} // namespace qftbx

#endif // QFTBX_GUI_LANGUAGE_H
