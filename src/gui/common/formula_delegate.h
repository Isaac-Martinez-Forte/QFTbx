/**
 * @file
 * @brief An item delegate that draws the formula a table cell holds.
 *
 * A table of specifications is a table of bounds, and a bound is a transfer
 * function: as two lists of coefficients it says nothing, drawn as the
 * quotient it is, it says everything. The model keeps the formula in a user
 * role of the cell; a cell without one is drawn as the ordinary text it
 * holds.
 */

#ifndef QFTBX_GUI_FORMULA_DELEGATE_H
#define QFTBX_GUI_FORMULA_DELEGATE_H

#include <QStyledItemDelegate>

namespace qftbx {

/**
 * @brief Draws the formula a cell holds, instead of the text of one.
 *
 * A table of specifications is a table of bounds, and a bound is a transfer
 * function: read as "1 4 19.752" over "1 30" it says nothing, and drawn as
 * the quotient it is, it says everything. The cell carries the Formula in
 * FormulaDelegate::formulaRole and the delegate draws it; a cell without
 * one is drawn as the ordinary text it holds.
 */
class FormulaDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    /// Where the model keeps the formula of a cell.
    static constexpr int formulaRole = Qt::UserRole + 1;

    explicit FormulaDelegate(QObject * parent = nullptr);

    void paint(QPainter * painter, const QStyleOptionViewItem & option,
               const QModelIndex & index) const override;

    QSize sizeHint(const QStyleOptionViewItem & option,
                   const QModelIndex & index) const override;
};

}

#endif
