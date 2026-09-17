#include "src/gui/common/formula_delegate.h"

#include <algorithm>

#include <QPainter>

#include "src/gui/common/formula_view.h"

namespace qftbx {

namespace {

//Air around the formula inside its cell.
const int kMargin = 6;

//A formula wider than its column is drawn smaller rather than cut off, but
//only down to here: past it the answer is to widen the column.
const double kSmallest = 0.6;

} // namespace

FormulaDelegate::FormulaDelegate(QObject * parent)
    : QStyledItemDelegate(parent)
{
}

void FormulaDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option,
                            const QModelIndex & index) const
{
    const QVariant held = index.data(formulaRole);

    if (!held.canConvert<Formula>()) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    const Formula formula = held.value<Formula>();

    QStyleOptionViewItem style = option;
    initStyleOption(&style, index);
    //The text is the formula; drawing both would print it twice.
    style.text.clear();
    style.widget->style()->drawControl(QStyle::CE_ItemViewItem, &style, painter, style.widget);

    const QRectF room = QRectF(option.rect).adjusted(kMargin, 2, -kMargin, -2);
    const FormulaMetrics natural = formulaMetrics(formula, option.font);

    double factor = 1.0;
    if (natural.width > 0.0 && natural.height() > 0.0) {
        factor = std::min({1.0, room.width() / natural.width, room.height() / natural.height()});
    }
    factor = std::max(kSmallest, factor);

    QFont drawn = option.font;
    if (drawn.pointSizeF() > 0.0) {
        drawn.setPointSizeF(drawn.pointSizeF() * factor);
    }

    const FormulaMetrics metrics = formulaMetrics(formula, drawn);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(option.palette.color(option.state & QStyle::State_Selected
                                         ? QPalette::HighlightedText : QPalette::Text));
    drawFormula(*painter, formula, drawn, room.left(),
                room.top() + std::max(0.0, (room.height() - metrics.height()) / 2.0)
                + metrics.ascent);
    painter->restore();
}

QSize FormulaDelegate::sizeHint(const QStyleOptionViewItem & option,
                                const QModelIndex & index) const
{
    const QVariant held = index.data(formulaRole);

    if (!held.canConvert<Formula>()) {
        return QStyledItemDelegate::sizeHint(option, index);
    }

    const FormulaMetrics metrics = formulaMetrics(held.value<Formula>(), option.font);

    return QSize(int(metrics.width) + 2 * kMargin, int(metrics.height()) + 4);
}

} // namespace qftbx
