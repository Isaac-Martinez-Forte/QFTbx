#ifndef QFTBX_THEME_H
#define QFTBX_THEME_H

#include <QString>
#include <QStringList>

/**
 * @file
 * @brief The look of the interface: flat, square and quiet.
 *
 * Qt's Fusion style with a palette of ours and a small style sheet: right
 * angles, one-pixel borders, no gradient and no shadow anywhere, and one
 * accent colour, which is the blue of the toolbox's own icon.
 *
 * The style sheet is written in terms of palette roles rather than colours,
 * so the three themes are three palettes and one sheet, and the system
 * theme is the machine's own palette under the same shapes.
 */

namespace qftbx {

/// The themes the interface can wear.
extern const QString kSystemTheme;
extern const QString kLightTheme;
extern const QString kDarkTheme;

/// The codes above, in the order the menu lists them.
QStringList availableThemes();

/// Whether a code names a theme this build has.
bool isAvailableTheme(const QString & code);

/// The name of a theme in the interface's language.
QString themeName(const QString & code);

/**
 * @brief Dresses the whole application in a theme.
 *
 * Sets the style, the palette and the style sheet on the application, so
 * every window that exists and every one built afterwards follows. An
 * unknown code is the system theme.
 */
void applyTheme(const QString & code);

/// Writes the chosen theme into the settings file in use, like the
/// language. An empty path writes nothing: there is nowhere to put it.
void storeTheme(const QString & code, const std::string & settingsPath);

}

#endif
