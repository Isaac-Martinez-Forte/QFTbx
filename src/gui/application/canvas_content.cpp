/**
 * @file
 * @brief The canvas that shows the banner while it holds no card.
 *
 * The banner is drawn inside a margin, never larger than its own size, and
 * clipped to a rounded rectangle whose radius keeps the proportion of the
 * file's, 26 units in 1200.
 */

#include "src/gui/application/canvas_content.h"

#include "src/gui/application/phase_card.h"

#include <QEvent>
#include <QPainter>
#include <QPainterPath>

namespace qftbx {

namespace {

constexpr int kMargin = 32;
constexpr double kCornerShare = 26.0 / 1200.0;

}

CanvasContent::CanvasContent(QWidget * parent)
    : QWidget(parent), m_banner(QStringLiteral(":/qftbx_banner.svg"))
{
}

bool CanvasContent::showsBanner() const
{
    if (!m_banner.isValid()) {
        return false;
    }
    for (const PhaseCard * card : findChildren<PhaseCard *>(QString(), Qt::FindDirectChildrenOnly)) {
        if (card->isVisibleTo(this)) {
            return false;
        }
    }
    return true;
}

bool CanvasContent::event(QEvent * event)
{
    switch (event->type()) {
    case QEvent::ChildAdded:
    case QEvent::ChildRemoved:
    case QEvent::LayoutRequest:
        update();
        break;
    default:
        break;
    }
    return QWidget::event(event);
}

void CanvasContent::paintEvent(QPaintEvent *)
{
    if (!showsBanner()) {
        return;
    }

    const QRectF room = QRectF(rect()).adjusted(kMargin, kMargin, -kMargin, -kMargin);
    if (room.width() <= 0.0 || room.height() <= 0.0) {
        return;
    }

    const QSizeF natural = m_banner.defaultSize();
    const QSizeF size = natural.width() <= room.width() && natural.height() <= room.height()
                            ? natural
                            : natural.scaled(room.size(), Qt::KeepAspectRatio);
    QRectF target(QPointF(), size);
    target.moveCenter(room.center());

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    QPainterPath corners;
    const double radius = kCornerShare * target.width();
    corners.addRoundedRect(target, radius, radius);
    painter.setClipPath(corners);
    m_banner.render(&painter, target);
}

}
