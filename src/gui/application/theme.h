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
 * theme is the machine's own palette under the same shapes. applyTheme
 * sets the style, the palette and the sheet on the application, so every
 * window, present or future, follows; an unknown code is the system theme.
 * availableThemes lists the codes in the order the menu shows them.
 * storeTheme writes the choice into the settings file in use, like the
 * language, and writes nothing given an empty path.
 */

namespace qftbx {

extern const QString kSystemTheme;
extern const QString kLightTheme;
extern const QString kDarkTheme;

QStringList availableThemes();

bool isAvailableTheme(const QString & code);

QString themeName(const QString & code);

void applyTheme(const QString & code);

void storeTheme(const QString & code, const std::string & settingsPath);

}

#endif
