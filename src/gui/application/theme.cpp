/**
 * @file
 * @brief The palettes and the style sheet of the three themes.
 *
 * The style is Fusion rather than the platform's, because the native styles
 * of Windows and macOS ignore half of the shapes the sheet asks for. The
 * accent is the blue of the toolbox's icon, hue 204, at the two lightnesses
 * readable on a light and a dark ground. The sheet says only shapes, in
 * palette roles, so one sheet dresses all three themes; a tool button gets
 * no padding of its own, or the icon-only ones of a file dialog come out
 * blank. What no role can say is appended once the theme is known: the
 * drop-down arrow image, the red of a wrong field, the colours of a
 * verdict, and those of the two notices beside a template, which are
 * warnings and not refusals.
 */

#include "src/gui/application/theme.h"

#include <QApplication>
#include <QCoreApplication>
#include <QPalette>
#include <QStyle>
#include <QStyleFactory>

#include "src/core/project/settings.h"

namespace qftbx {

const QString kSystemTheme = QStringLiteral("system");
const QString kLightTheme = QStringLiteral("light");
const QString kDarkTheme = QStringLiteral("dark");

namespace {

const QColor kAccentOnLight(0x2c, 0x7b, 0xb6);
const QColor kAccentOnDark(0x4a, 0x9b, 0xd4);

QPalette lightPalette()
{
    QPalette palette;

    const QColor window(0xf3, 0xf5, 0xf7);
    const QColor base(0xff, 0xff, 0xff);
    const QColor alternate(0xf7, 0xf9, 0xfb);
    const QColor text(0x1d, 0x24, 0x29);
    const QColor border(0xd3, 0xda, 0xe0);
    const QColor faded(0xa7, 0xaf, 0xb5);

    palette.setColor(QPalette::Window, window);
    palette.setColor(QPalette::WindowText, text);
    palette.setColor(QPalette::Base, base);
    palette.setColor(QPalette::AlternateBase, alternate);
    palette.setColor(QPalette::ToolTipBase, base);
    palette.setColor(QPalette::ToolTipText, text);
    palette.setColor(QPalette::Text, text);
    palette.setColor(QPalette::Button, base);
    palette.setColor(QPalette::ButtonText, text);
    palette.setColor(QPalette::BrightText, Qt::white);
    palette.setColor(QPalette::Mid, border);
    palette.setColor(QPalette::Dark, border.darker(115));
    palette.setColor(QPalette::Light, base);
    palette.setColor(QPalette::Shadow, border);
    palette.setColor(QPalette::Highlight, kAccentOnLight);
    palette.setColor(QPalette::HighlightedText, Qt::white);
    palette.setColor(QPalette::Link, kAccentOnLight);

    palette.setColor(QPalette::Disabled, QPalette::Text, faded);
    palette.setColor(QPalette::Disabled, QPalette::WindowText, faded);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, faded);

    return palette;
}

QPalette darkPalette()
{
    QPalette palette;

    const QColor window(0x22, 0x27, 0x2b);
    const QColor base(0x1a, 0x1e, 0x21);
    const QColor alternate(0x26, 0x2c, 0x31);
    const QColor button(0x2a, 0x30, 0x35);
    const QColor text(0xe4, 0xe9, 0xed);
    const QColor border(0x3b, 0x43, 0x4a);
    const QColor faded(0x79, 0x83, 0x8b);

    palette.setColor(QPalette::Window, window);
    palette.setColor(QPalette::WindowText, text);
    palette.setColor(QPalette::Base, base);
    palette.setColor(QPalette::AlternateBase, alternate);
    palette.setColor(QPalette::ToolTipBase, alternate);
    palette.setColor(QPalette::ToolTipText, text);
    palette.setColor(QPalette::Text, text);
    palette.setColor(QPalette::Button, button);
    palette.setColor(QPalette::ButtonText, text);
    palette.setColor(QPalette::BrightText, Qt::white);
    palette.setColor(QPalette::Mid, border);
    palette.setColor(QPalette::Dark, border.lighter(130));
    palette.setColor(QPalette::Light, alternate);
    palette.setColor(QPalette::Shadow, QColor(0x14, 0x17, 0x1a));
    palette.setColor(QPalette::Highlight, kAccentOnDark);
    palette.setColor(QPalette::HighlightedText, QColor(0x10, 0x14, 0x17));
    palette.setColor(QPalette::Link, kAccentOnDark);

    palette.setColor(QPalette::Disabled, QPalette::Text, faded);
    palette.setColor(QPalette::Disabled, QPalette::WindowText, faded);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, faded);

    return palette;
}

const char * const kSheet = R"(
* {
    border-radius: 0px;
}

QPushButton {
    background: palette(button);
    border: 1px solid palette(mid);
    padding: 4px 12px;
}
QPushButton:hover {
    background: palette(alternate-base);
    border-color: palette(highlight);
}
QPushButton:pressed {
    background: palette(mid);
}
QPushButton:disabled {
    border-color: palette(alternate-base);
}

QToolButton {
    background: transparent;
    border: 1px solid transparent;
}
QToolButton:hover {
    background: palette(alternate-base);
    border-color: palette(mid);
}
QToolButton:pressed, QToolButton:checked {
    background: palette(mid);
}

QWidget[cardBar="true"] QToolButton {
    background: palette(button);
    border: 1px solid palette(mid);
    padding: 3px 10px;
}
QWidget[cardBar="true"] QToolButton:hover {
    background: palette(alternate-base);
    border-color: palette(highlight);
}
QWidget[cardBar="true"] QToolButton:disabled {
    border-color: palette(alternate-base);
}

QLineEdit, QComboBox, QTextBrowser, QPlainTextEdit, QAbstractSpinBox {
    background: palette(base);
    border: 1px solid palette(mid);
    padding: 3px 6px;
    selection-background-color: palette(highlight);
}
QLineEdit:focus, QComboBox:focus, QPlainTextEdit:focus, QAbstractSpinBox:focus {
    border-color: palette(highlight);
}
QComboBox::drop-down {
    border: none;
    width: 18px;
}
QComboBox::down-arrow {
    width: 10px;
    height: 10px;
}

QProgressBar {
    background: palette(base);
    border: 1px solid palette(mid);
    text-align: center;
}
QProgressBar::chunk {
    background: palette(highlight);
}

QTabWidget::pane {
    border: 1px solid palette(mid);
}
QTabBar::tab {
    background: transparent;
    border: none;
    border-bottom: 2px solid transparent;
    padding: 5px 12px;
}
QTabBar::tab:selected {
    border-bottom-color: palette(highlight);
}

QGroupBox {
    border: 1px solid palette(mid);
    margin-top: 14px;
}
QGroupBox[bare="true"] {
    border: none;
    margin-top: 0px;
}
QGroupBox::title {
    subcontrol-origin: margin;
    left: 8px;
    padding: 0px 4px;
}

QMenuBar {
    background: palette(window);
}
QMenuBar::item {
    padding: 4px 8px;
}
QMenuBar::item:selected, QMenu::item:selected {
    background: palette(highlight);
    color: palette(highlighted-text);
}
QMenu {
    background: palette(base);
    border: 1px solid palette(mid);
}
QMenu::item {
    padding: 4px 24px 4px 20px;
}

QScrollBar:vertical {
    background: transparent;
    width: 12px;
    margin: 0px;
}
QScrollBar:horizontal {
    background: transparent;
    height: 12px;
    margin: 0px;
}
QScrollBar::handle {
    background: palette(mid);
    min-height: 24px;
    min-width: 24px;
}
QScrollBar::handle:hover {
    background: palette(dark);
}
QScrollBar::add-line, QScrollBar::sub-line {
    height: 0px;
    width: 0px;
}
QScrollBar::add-page, QScrollBar::sub-page {
    background: transparent;
}

qftbx--PhaseCard {
    background: palette(base);
    border: 1px solid palette(mid);
}
QWidget[cardBar="true"] {
    background: palette(alternate-base);
    border-bottom: 1px solid palette(mid);
}
)";

QString sheetOf(const QPalette & palette)
{
    const bool dark = palette.color(QPalette::Window).lightness() < 128;

    const QString arrow = dark ? ":/icons/arrow-down-light.svg" : ":/icons/arrow-down-dark.svg";
    const QString wrong = dark ? "#e06c6c" : "#c0392b";
    const QString met = dark ? "#5fbf7a" : "#1a7f37";
    const QString coarse = dark ? "#e0a860" : "#b45309";

    return QString::fromUtf8(kSheet) + QStringLiteral(R"(
QComboBox::down-arrow {
    image: url(%1);
}

QLineEdit[wrong="true"], QComboBox[wrong="true"], QPlainTextEdit[wrong="true"],
QAbstractSpinBox[wrong="true"] {
    border: 1px solid %2;
}
QLabel[wrong="true"] {
    color: %2;
}

QLabel[verdict="met"] {
    color: %3;
}
QLabel[verdict="exceeded"] {
    color: %2;
    font-weight: bold;
}

QLabel[notice="closed"] {
    color: %2;
}
QLabel[notice="coarse"] {
    color: %4;
}
)").arg(arrow, wrong, met, coarse);
}

}

QStringList availableThemes()
{
    return {kSystemTheme, kLightTheme, kDarkTheme};
}

bool isAvailableTheme(const QString & code)
{
    return availableThemes().contains(code);
}

QString themeName(const QString & code)
{
    if (code == kLightTheme) {
        return QCoreApplication::translate("qftbx::Theme", "Light");
    }
    if (code == kDarkTheme) {
        return QCoreApplication::translate("qftbx::Theme", "Dark");
    }

    return QCoreApplication::translate("qftbx::Theme", "System theme");
}

void applyTheme(const QString & code)
{
    if (QStyle * fusion = QStyleFactory::create("Fusion")) {
        QApplication::setStyle(fusion);
    }

    if (code == kLightTheme) {
        QApplication::setPalette(lightPalette());
    } else if (code == kDarkTheme) {
        QApplication::setPalette(darkPalette());
    } else {
        QApplication::setPalette(QApplication::style()->standardPalette());
    }

    qApp->setStyleSheet(sheetOf(QApplication::palette()));
}

void storeTheme(const QString & code, const std::string & settingsPath)
{
    if (settingsPath.empty()) {
        return;
    }

    qftbx::writeSetting(settingsPath, "interface.theme", code.toStdString());
}

}
