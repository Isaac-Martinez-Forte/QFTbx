/**
 * @file
 * @brief A widget that draws a formula as it is written, and the drawing routines behind it.
 *
 * The forms take a transfer function as a line of text, which is what the
 * user cannot check; once read, the function that was understood is shown
 * back with the fraction under its rule, the exponents raised and the
 * parentheses as tall as what they hold, so the user can answer whether
 * that is the plant they meant. Measuring and drawing are exposed apart
 * from the widget so that anything with a painter can hold a formula, a
 * table cell above all, and the metrics give ascent and descent separately
 * because a fraction hangs below the baseline. The widget draws at the size
 * that fits and offers the LaTeX of the drawing on a right-click.
 */

#ifndef QFTBX_GUI_FORMULA_VIEW_H
#define QFTBX_GUI_FORMULA_VIEW_H

#include <QWidget>

class QPainter;

#include "src/core/math/formula.h"

namespace qftbx {

/**
 * @brief What a formula measures at a given font: its width and how far it
 * reaches above and below the baseline.
 *
 * A formula is not a line of text - a fraction hangs below the baseline and
 * an exponent rises above it - so a caller placing one needs the two halves
 * and not just a height.
 */
struct FormulaMetrics
{
    qreal width = 0.0;
    qreal ascent = 0.0;
    qreal descent = 0.0;

    qreal height() const { return ascent + descent; }
};

/// What the formula measures at this font, and the same formula drawn with
/// its baseline at (x, baseline). Available on their own so that anything
/// with a QPainter can hold a formula: a table cell, above all.
FormulaMetrics formulaMetrics(const Formula & formula, const QFont & font);
void drawFormula(QPainter & painter, const Formula & formula, const QFont & font,
                 qreal x, qreal baseline);

/**
 * @brief Draws a formula the way it is written: the fraction under its
 * rule, the exponents raised, the parentheses as tall as what they hold.
 *
 * What it is for: the forms ask for a transfer function as a line of text,
 * and a line of text is exactly what the user cannot check. Once the text
 * has been read, this shows back the function that was understood, and the
 * user answers the only question that matters - is that the plant I meant.
 *
 * It draws at the size that fits, between half and twice the font it is
 * given, so the same widget serves a card in the canvas and a figure of
 * its own. The LaTeX of what is drawn is one right-click
 * away, for a paper.
 */
class FormulaView : public QWidget
{
    Q_OBJECT

public:
    explicit FormulaView(QWidget * parent = nullptr);

    void setFormula(Formula formula);
    void clear();

    bool isEmpty() const;

    /// The formula in LaTeX, as the body of a math environment.
    QString latex() const;

    /// The text shown when there is no formula (an explanation, a hint).
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

/// So that a formula travels in a QVariant, which is how a model hands one to
/// the delegate that draws it.
Q_DECLARE_METATYPE(qftbx::Formula)

#endif
