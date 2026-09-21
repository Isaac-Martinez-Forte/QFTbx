/**
 * @file
 * @brief Marks a widget through a style property and has the sheet read again.
 *
 * Declares the two inline helpers by which a form points at the field that
 * is wrong instead of describing the mistake in a message box. The colour
 * is the theme's: the sheet selects on the property and each theme answers
 * with its own red. Qt does not re-read a sheet when a property changes, so
 * the widget is unpolished and polished again, and only when the value is
 * new, since this runs on every keystroke. While a field is wrong its
 * tooltip is the reason; its own tooltip, put aside the first time, comes
 * back when it stops being wrong.
 */

#ifndef QFTBX_GUI_FIELD_MARK_H
#define QFTBX_GUI_FIELD_MARK_H

#include <QString>
#include <QStyle>
#include <QVariant>
#include <QWidget>

namespace qftbx {

/**
 * @brief Sets a style property and has the sheet read again.
 *
 * Qt does not re-apply a style sheet when a property it selects on changes:
 * the widget has to be unpolished and polished again, which is the one line
 * everybody forgets. The colours themselves live in the theme, where the
 * light and the dark one answer differently.
 */
inline void markAs(QWidget * widget, const char * property, const QVariant & value)
{
    if (widget == nullptr || widget->property(property) == value) {
        return;
    }

    widget->setProperty(property, value);

    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
}

/**
 * @brief Marks the field that is wrong, and says why in its tooltip.
 *
 * A form that answers "there is an error in the plant data" leaves the user
 * hunting for it. This puts the answer where the mistake is: the colour is
 * the sheet's (a property the theme styles), so the two themes get a red
 * each and nothing here knows which.
 */
inline void markWrong(QWidget * field, bool wrong, const QString & reason = QString())
{
    if (field == nullptr) {
        return;
    }

    const char * const kept = "fieldTooltip";

    if (!field->property(kept).isValid()) {
        field->setProperty(kept, field->toolTip());
    }

    field->setToolTip(wrong ? reason : field->property(kept).toString());

    markAs(field, "wrong", wrong);
}

}

#endif
