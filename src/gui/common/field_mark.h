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

    //The reason takes the place of the tooltip while the field is wrong,
    //and the field's OWN tooltip comes back when it stops being wrong: it
    //says what the field is for, and that is not this function's to throw
    //away. Put aside the first time, because from then on the tooltip on
    //the widget may be a reason of ours.
    const char * const kept = "fieldTooltip";

    if (!field->property(kept).isValid()) {
        field->setProperty(kept, field->toolTip());
    }

    field->setToolTip(wrong ? reason : field->property(kept).toString());

    //Repolishing a widget that is already marked repaints it for nothing,
    //and this is called on every keystroke: markAs answers that.
    markAs(field, "wrong", wrong);
}

} // namespace qftbx

#endif // QFTBX_GUI_FIELD_MARK_H
