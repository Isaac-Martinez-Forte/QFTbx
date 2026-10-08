/**
 * @file
 * @brief A widget that draws a formula as it is written, and the drawing
 * routines behind it.
 *
 * The forms take a transfer function as a line of text, which is what the
 * user cannot check; once read, the function that was understood is shown
 * back with the fraction under its rule, the exponents raised and the
 * parentheses as tall as what they hold, so the user can answer whether
 * that is the plant they meant. Measuring and drawing are exposed apart
 * from the widget so that anything with a painter can hold a formula, a
 * table cell above all, and the metrics give ascent and descent separately
 * because a fraction hangs below the baseline. drawFormula() puts the
 * baseline at the y it is given.
 *
 * The widget draws at the size that fits, between half and twice its font,
 * shows the placeholder when it holds no formula, and offers the LaTeX of
 * the drawing, the body of a math environment, on a right-click. Formula is
 * declared a metatype so that a model can hand one to its delegate in a
 * QVariant.
 */

#ifndef QFTBX_GUI_FORMULA_VIEW_H
#define QFTBX_GUI_FORMULA_VIEW_H

#include <QWidget>

class QPainter;

#include "src/core/math/formula.h"

namespace qftbx {

struct FormulaMetrics
{
    qreal width = 0.0;
    qreal ascent = 0.0;
    qreal descent = 0.0;

    qreal height() const { return ascent + descent; }
};

FormulaMetrics formulaMetrics(const Formula & formula, const QFont & font);
void drawFormula(QPainter & painter, const Formula & formula, const QFont & font,
                 qreal x, qreal baseline);

class FormulaView : public QWidget
{
    Q_OBJECT

public:
    explicit FormulaView(QWidget * parent = nullptr);

    void setFormula(Formula formula);
    void clear();

    bool isEmpty() const;

    QString latex() const;

    void setPlaceholder(const QString & text);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent * event) override;
    void contextMenuEvent(QContextMenuEvent * event) override;

private:
    Formula m_formula;
    QString m_placeholder;
};

}

Q_DECLARE_METATYPE(qftbx::Formula)

#endif
