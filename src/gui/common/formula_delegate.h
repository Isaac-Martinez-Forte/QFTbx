/**
 * @file
 * @brief An item delegate that draws the formula a table cell holds.
 *
 * A table of specifications is a table of bounds, and a bound is a transfer
 * function: as two lists of coefficients it says nothing, drawn as the
 * quotient it is, it says everything. The model keeps the Formula of a
 * cell in FormulaDelegate::formulaRole; a cell without one is drawn as the
 * ordinary text it holds.
 */

#ifndef QFTBX_GUI_FORMULA_DELEGATE_H
#define QFTBX_GUI_FORMULA_DELEGATE_H

#include <QStyledItemDelegate>

namespace qftbx {

class FormulaDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    static constexpr int formulaRole = Qt::UserRole + 1;

    explicit FormulaDelegate(QObject * parent = nullptr);

    void paint(QPainter * painter, const QStyleOptionViewItem & option,
               const QModelIndex & index) const override;

    QSize sizeHint(const QStyleOptionViewItem & option,
                   const QModelIndex & index) const override;
};

}

#endif
