/**
 * @file
 * @brief Draws the arrow of a fold button and turns it.
 *
 * The arrow is a triangle painted at the device's pixel ratio and rotated
 * about its centre, pointing right at no angle and turning clockwise, so
 * down is a quarter turn. It follows the checked state however that is
 * set, a signal blocked or not, and turns at once while the button is not
 * on screen.
 */

#include "src/gui/common/fold_button.h"

#include <QEvent>
#include <QIcon>
#include <QPainter>
#include <QPixmap>
#include <QPolygonF>
#include <QVariantAnimation>

namespace qftbx {

namespace {

constexpr int kTurnMilliseconds = 160;
constexpr int kArrowSize = 10;

qreal angleOf(Qt::ArrowType direction)
{
    switch (direction) {
    case Qt::DownArrow:
        return 90.0;
    case Qt::LeftArrow:
        return 180.0;
    case Qt::UpArrow:
        return 270.0;
    default:
        return 0.0;
    }
}

}

FoldButton::FoldButton(Qt::ArrowType opensTowards, QWidget * parent)
    : QToolButton(parent), m_folded(angleOf(opensTowards)), m_angle(m_folded)
{
    m_turn = new QVariantAnimation(this);
    m_turn->setDuration(kTurnMilliseconds);
    m_turn->setEasingCurve(QEasingCurve::InOutQuad);
    connect(m_turn, &QVariantAnimation::valueChanged, this, [this](const QVariant & value) {
        m_angle = value.toReal();
        drawArrow();
    });

    setCheckable(true);
    setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    setIconSize(QSize(kArrowSize, kArrowSize));
    drawArrow();
}

qreal FoldButton::restingAngle(bool open) const
{
    return open ? m_folded + 180.0 : m_folded;
}

void FoldButton::checkStateSet()
{
    QToolButton::checkStateSet();
    turnTo(restingAngle(isChecked()));
}

void FoldButton::turnTo(qreal angle)
{
    m_turn->stop();
    if (!isVisible() || angle == m_angle) {
        m_angle = angle;
        drawArrow();
        return;
    }
    m_turn->setStartValue(m_angle);
    m_turn->setEndValue(angle);
    m_turn->start();
}

void FoldButton::changeEvent(QEvent * event)
{
    if (event->type() == QEvent::PaletteChange) {
        drawArrow();
    }
    QToolButton::changeEvent(event);
}

void FoldButton::drawArrow()
{
    const qreal ratio = devicePixelRatioF();
    QPixmap pixmap(QSize(kArrowSize, kArrowSize) * ratio);
    pixmap.setDevicePixelRatio(ratio);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.translate(kArrowSize / 2.0, kArrowSize / 2.0);
    painter.rotate(m_angle);
    painter.setPen(Qt::NoPen);
    painter.setBrush(palette().color(QPalette::ButtonText));
    const qreal reach = 0.4 * kArrowSize;
    painter.drawPolygon(QPolygonF{QPointF(reach, 0.0), QPointF(-0.6 * reach, -0.85 * reach),
                                  QPointF(-0.6 * reach, 0.85 * reach)});
    painter.end();

    setIcon(QIcon(pixmap));
}

}
