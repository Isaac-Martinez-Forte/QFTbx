/**
 * @file
 * @brief Recursive measuring and drawing of a formula tree.
 *
 * Every dimension is a fraction of the em of the font in use, so the whole
 * drawing scales together, with the usual constants of a maths setting: the
 * fraction rule sits on the axis of the line a little above the baseline,
 * an exponent rises by half the ascent of its base, and exponents and
 * subscripts take seven tenths of the font. Parentheses are enlarged to
 * hold what they enclose and centred on it, not on the baseline; a product
 * is drawn as a centred dot, a function name upright, and the digits a
 * symbol ends in as a subscript. The widget draws the whole formula at one
 * size, the largest between half and twice its font at which it fits, and
 * a pixel font is scaled by its pixel size since it has no point size.
 */

#include "src/gui/common/formula_view.h"

#include <algorithm>
#include <cmath>

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QFontMetricsF>
#include <QMenu>
#include <QPainter>
#include <QPainterPath>

namespace qftbx {

namespace {

const double kSmaller = 0.7;
const double kSmallestPoints = 5.0;
const double kAxis = 0.28;
const double kRule = 0.055;
const double kFractionGap = 0.16;
const double kFractionSide = 0.18;
const double kOperatorAir = 0.28;
const double kProductAir = 0.06;
const double kSubscriptDrop = 0.2;
const double kSuperscriptRise = 0.45;
const double kFenceAir = 0.06;
const double kRadicalWidth = 0.6;
const double kRadicalAir = 0.1;

const double kMinimumScale = 0.5;
const double kMaximumScale = 2.0;

struct Box
{
    qreal width = 0.0;
    qreal ascent = 0.0;
    qreal descent = 0.0;

    qreal height() const { return ascent + descent; }
};

QFont scaled(const QFont & font, double factor)
{
    QFont resized = font;

    if (font.pointSizeF() > 0.0) {
        resized.setPointSizeF(std::max(kSmallestPoints, font.pointSizeF() * factor));
    } else {
        resized.setPixelSize(std::max(6, int(std::lround(font.pixelSize() * factor))));
    }

    return resized;
}

QFont italic(const QFont & font)
{
    QFont slanted = font;
    slanted.setItalic(true);
    return slanted;
}

qreal em(const QFont & font)
{
    return QFontMetricsF(font).height();
}

Box measure(const Formula & formula, const QFont & font);
void draw(QPainter & painter, const Formula & formula, const QFont & font,
          qreal x, qreal baseline);

Box textBox(const QString & text, const QFont & font)
{
    const QFontMetricsF metrics(font);

    Box box;
    box.width = metrics.horizontalAdvance(text);
    box.ascent = metrics.ascent();
    box.descent = metrics.descent();

    return box;
}

Box measureSymbol(const Formula & formula, const QFont & font, QString & stem, QString & subscript)
{
    std::string stemText;
    std::string subscriptText;
    formula::splitName(formula.text, stemText, subscriptText);

    stem = QString::fromStdString(stemText);
    subscript = QString::fromStdString(subscriptText);

    Box box = textBox(stem, italic(font));

    if (!subscript.isEmpty()) {
        const QFont small = scaled(font, kSmaller);
        const Box lowered = textBox(subscript, small);
        box.width += lowered.width;
        box.descent = std::max(box.descent, kSubscriptDrop * em(font) + lowered.descent);
    }

    return box;
}

QFont fenceFont(const QFont & font, const QString & glyph, qreal height)
{
    const QFontMetricsF metrics(font);
    const qreal natural = metrics.boundingRect(glyph).height();

    if (natural <= 0.0) {
        return font;
    }

    const double factor = std::max(1.0, (height + 2.0 * kFenceAir * em(font)) / natural);

    return scaled(font, factor);
}

QString drawn(const Formula & formula)
{
    if (formula.kind == Formula::Kind::Operator && formula.text == "*") {
        return QStringLiteral("\u00b7");
    }
    if (formula.kind == Formula::Kind::Operator && formula.text == "-") {
        return QStringLiteral("\u2212");
    }

    return QString::fromStdString(formula.text);
}

QString openingOf(const Formula & formula)
{
    if (formula.text == "|") {
        return QStringLiteral("|");
    }
    if (formula.text == "[") {
        return QStringLiteral("[");
    }
    return QStringLiteral("(");
}

QString closingOf(const Formula & formula)
{
    if (formula.text == "|") {
        return QStringLiteral("|");
    }
    if (formula.text == "[") {
        return QStringLiteral("]");
    }
    return QStringLiteral(")");
}

Box measure(const Formula & formula, const QFont & font)
{
    switch (formula.kind) {
    case Formula::Kind::Number:
        return textBox(QString::fromStdString(formula.text), font);

    case Formula::Kind::Symbol: {
        QString stem;
        QString subscript;
        return measureSymbol(formula, font, stem, subscript);
    }

    case Formula::Kind::Operator: {
        Box box = textBox(drawn(formula), font);
        box.width += 2.0 * kOperatorAir * em(font);
        return box;
    }

    case Formula::Kind::Juxtaposed: {
        Box box;
        box.width = kProductAir * em(font);
        return box;
    }

    case Formula::Kind::Row: {
        Box box;
        for (const Formula & part : formula.parts) {
            const Box piece = measure(part, font);
            box.width += piece.width;
            box.ascent = std::max(box.ascent, piece.ascent);
            box.descent = std::max(box.descent, piece.descent);
        }
        return box;
    }

    case Formula::Kind::Fraction: {
        const Box above = measure(formula.parts[0], font);
        const Box below = measure(formula.parts[1], font);
        const qreal unit = em(font);

        Box box;
        box.width = std::max(above.width, below.width) + 2.0 * kFractionSide * unit;
        box.ascent = kAxis * unit + 0.5 * kRule * unit + kFractionGap * unit + above.height();
        box.descent = -kAxis * unit + 0.5 * kRule * unit + kFractionGap * unit + below.height();
        return box;
    }

    case Formula::Kind::Power: {
        const Box base = measure(formula.parts[0], font);
        const Box exponent = measure(formula.parts[1], scaled(font, kSmaller));

        Box box;
        box.width = base.width + exponent.width;
        box.ascent = std::max(base.ascent, kSuperscriptRise * base.ascent + exponent.height());
        box.descent = base.descent;
        return box;
    }

    case Formula::Kind::Fenced: {
        const Box inside = measure(formula.parts[0], font);
        const QFont grown = fenceFont(font, openingOf(formula), inside.height());
        const QFontMetricsF metrics(grown);

        Box box = inside;
        box.width += metrics.horizontalAdvance(openingOf(formula))
                + metrics.horizontalAdvance(closingOf(formula));
        box.ascent += kFenceAir * em(font);
        box.descent += kFenceAir * em(font);
        return box;
    }

    case Formula::Kind::Function: {
        Formula fenced = formula::fenced(formula.parts[0]);
        QString stem;
        QString subscript;
        Formula name = formula::symbol(formula.text);
        const Box nameBox = measureSymbol(name, font, stem, subscript);
        const Box argument = measure(fenced, font);

        Box box;
        box.width = nameBox.width + argument.width;
        box.ascent = std::max(nameBox.ascent, argument.ascent);
        box.descent = std::max(nameBox.descent, argument.descent);
        return box;
    }

    case Formula::Kind::Root: {
        const Box inside = measure(formula.parts[0], font);
        const qreal unit = em(font);

        Box box = inside;
        box.width += (kRadicalWidth + 2.0 * kRadicalAir) * unit;
        box.ascent += (kRule + 2.0 * kRadicalAir) * unit;
        return box;
    }
    }

    return Box();
}

void draw(QPainter & painter, const Formula & formula, const QFont & font,
          qreal x, qreal baseline)
{
    const qreal unit = em(font);

    switch (formula.kind) {
    case Formula::Kind::Number: {
        painter.setFont(font);
        painter.drawText(QPointF(x, baseline), QString::fromStdString(formula.text));
        return;
    }

    case Formula::Kind::Symbol: {
        QString stem;
        QString subscript;
        measureSymbol(formula, font, stem, subscript);

        const QFont slanted = italic(font);
        painter.setFont(slanted);
        painter.drawText(QPointF(x, baseline), stem);

        if (!subscript.isEmpty()) {
            painter.setFont(scaled(font, kSmaller));
            painter.drawText(QPointF(x + QFontMetricsF(slanted).horizontalAdvance(stem),
                                     baseline + kSubscriptDrop * unit),
                             subscript);
        }
        return;
    }

    case Formula::Kind::Operator: {
        painter.setFont(font);
        painter.drawText(QPointF(x + kOperatorAir * unit, baseline), drawn(formula));
        return;
    }

    case Formula::Kind::Juxtaposed:
        return;

    case Formula::Kind::Row: {
        qreal cursor = x;
        for (const Formula & part : formula.parts) {
            draw(painter, part, font, cursor, baseline);
            cursor += measure(part, font).width;
        }
        return;
    }

    case Formula::Kind::Fraction: {
        const Box whole = measure(formula, font);
        const Box above = measure(formula.parts[0], font);
        const Box below = measure(formula.parts[1], font);

        const qreal axis = baseline - kAxis * unit;
        const qreal centre = x + whole.width / 2.0;

        draw(painter, formula.parts[0], font, centre - above.width / 2.0,
             axis - 0.5 * kRule * unit - kFractionGap * unit - above.descent);
        draw(painter, formula.parts[1], font, centre - below.width / 2.0,
             axis + 0.5 * kRule * unit + kFractionGap * unit + below.ascent);

        QPen pen = painter.pen();
        pen.setWidthF(std::max(1.0, kRule * unit));
        pen.setCapStyle(Qt::FlatCap);

        painter.save();
        painter.setPen(pen);
        painter.drawLine(QPointF(x + kFractionSide * unit / 2.0, axis),
                         QPointF(x + whole.width - kFractionSide * unit / 2.0, axis));
        painter.restore();
        return;
    }

    case Formula::Kind::Power: {
        const Box base = measure(formula.parts[0], font);
        const QFont small = scaled(font, kSmaller);
        const Box exponent = measure(formula.parts[1], small);

        draw(painter, formula.parts[0], font, x, baseline);
        draw(painter, formula.parts[1], small, x + base.width,
             baseline - kSuperscriptRise * base.ascent - exponent.descent);
        return;
    }

    case Formula::Kind::Fenced: {
        const Box inside = measure(formula.parts[0], font);
        const QFont grown = fenceFont(font, openingOf(formula), inside.height());
        const QFontMetricsF metrics(grown);

        const qreal centre = baseline - inside.ascent + inside.height() / 2.0;
        const QRectF glyph = metrics.boundingRect(openingOf(formula));
        const qreal fenceBaseline = centre - (glyph.top() + glyph.bottom()) / 2.0;

        painter.setFont(grown);
        painter.drawText(QPointF(x, fenceBaseline), openingOf(formula));

        const qreal opening = metrics.horizontalAdvance(openingOf(formula));
        draw(painter, formula.parts[0], font, x + opening, baseline);

        painter.setFont(grown);
        painter.drawText(QPointF(x + opening + inside.width, fenceBaseline), closingOf(formula));
        return;
    }

    case Formula::Kind::Function: {
        QString stem;
        QString subscript;
        Formula name = formula::symbol(formula.text);
        const Box nameBox = measureSymbol(name, font, stem, subscript);

        painter.setFont(font);
        painter.drawText(QPointF(x, baseline), stem);

        if (!subscript.isEmpty()) {
            painter.setFont(scaled(font, kSmaller));
            painter.drawText(QPointF(x + QFontMetricsF(font).horizontalAdvance(stem),
                                     baseline + kSubscriptDrop * unit),
                             subscript);
        }

        draw(painter, formula::fenced(formula.parts[0]), font, x + nameBox.width, baseline);
        return;
    }

    case Formula::Kind::Root: {
        const Box inside = measure(formula.parts[0], font);
        const qreal tick = kRadicalWidth * unit;
        const qreal top = baseline - inside.ascent - (kRule + 2.0 * kRadicalAir) * unit;

        draw(painter, formula.parts[0], font, x + tick + kRadicalAir * unit, baseline);

        QPen pen = painter.pen();
        pen.setWidthF(std::max(1.0, kRule * unit));
        pen.setJoinStyle(Qt::MiterJoin);

        painter.save();
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);

        QPainterPath radical;
        radical.moveTo(x, baseline - inside.height() / 2.0);
        radical.lineTo(x + tick * 0.35, baseline + inside.descent);
        radical.lineTo(x + tick * 0.8, top);
        radical.lineTo(x + inside.width + tick + 2.0 * kRadicalAir * unit, top);
        painter.drawPath(radical);
        painter.restore();
        return;
    }
    }
}

}

FormulaMetrics formulaMetrics(const Formula & formula, const QFont & font)
{
    const Box box = measure(formula, font);

    FormulaMetrics metrics;
    metrics.width = box.width;
    metrics.ascent = box.ascent;
    metrics.descent = box.descent;

    return metrics;
}

void drawFormula(QPainter & painter, const Formula & formula, const QFont & font,
                 qreal x, qreal baseline)
{
    draw(painter, formula, font, x, baseline);
}

FormulaView::FormulaView(QWidget * parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
}

void FormulaView::setFormula(Formula formula)
{
    m_formula = std::move(formula);
    updateGeometry();
    update();
}

void FormulaView::clear()
{
    setFormula(Formula());
}

bool FormulaView::isEmpty() const
{
    return formula::isEmpty(m_formula);
}

QString FormulaView::latex() const
{
    return QString::fromStdString(latexOf(m_formula));
}

void FormulaView::setPlaceholder(const QString & text)
{
    m_placeholder = text;
    update();
}

QSize FormulaView::sizeHint() const
{
    if (isEmpty()) {
        const QFontMetricsF metrics(font());
        return QSize(int(metrics.horizontalAdvance(m_placeholder)) + 16,
                     int(metrics.height() * 2.0));
    }

    const Box box = measure(m_formula, font());

    return QSize(int(std::ceil(box.width)) + 16, int(std::ceil(box.height())) + 12);
}

QSize FormulaView::minimumSizeHint() const
{
    const QFontMetricsF metrics(font());

    return QSize(int(metrics.height() * 3.0), int(metrics.height() * 2.0));
}

void FormulaView::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    if (isEmpty()) {
        if (m_placeholder.isEmpty()) {
            return;
        }
        painter.setPen(palette().color(QPalette::Disabled, QPalette::WindowText));
        painter.drawText(rect(), Qt::AlignCenter | Qt::TextWordWrap, m_placeholder);
        return;
    }

    const QRectF room = QRectF(rect()).adjusted(8, 6, -8, -6);
    const Box natural = measure(m_formula, font());

    double factor = kMaximumScale;
    if (natural.width > 0.0 && natural.height() > 0.0) {
        factor = std::min({kMaximumScale,
                           room.width() / natural.width,
                           room.height() / natural.height()});
    }
    factor = std::max(kMinimumScale, factor);

    const QFont drawn = scaled(font(), factor);
    const Box box = measure(m_formula, drawn);

    painter.setPen(palette().color(QPalette::WindowText));
    draw(painter, m_formula, drawn,
         room.left() + std::max(0.0, (room.width() - box.width) / 2.0),
         room.top() + std::max(0.0, (room.height() - box.height()) / 2.0) + box.ascent);
}

void FormulaView::contextMenuEvent(QContextMenuEvent * event)
{
    if (isEmpty()) {
        return;
    }

    QMenu menu(this);
    QAction * copy = menu.addAction(tr("Copy as LaTeX"));

    if (menu.exec(event->globalPos()) == copy) {
        QApplication::clipboard()->setText(latex());
    }
}

}
