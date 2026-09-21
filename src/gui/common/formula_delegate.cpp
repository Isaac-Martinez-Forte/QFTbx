/**
 * @file
 * @brief Painting and sizing of a formula inside a table cell.
 *
 * The cell's ground and selection are drawn by the style with the text
 * cleared, since the formula is the text and drawing both would print it
 * twice. A formula wider or taller than its cell is drawn smaller, down to
 * six tenths of the font and no further: past that the answer is to widen
 * the column. The size hint is the natural size plus a margin of air.
 */

#include "src/gui/common/formula_delegate.h"

#include <algorithm>

#include <QPainter>

#include "src/gui/common/formula_view.h"

namespace qftbx {

namespace {

const int kMargin = 6;

const double kSmallest = 0.6;

}

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

}
