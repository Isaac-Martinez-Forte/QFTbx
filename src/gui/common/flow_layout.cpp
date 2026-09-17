#include "src/gui/common/flow_layout.h"

#include <QWidget>

namespace qftbx {

FlowLayout::FlowLayout(QWidget * parent, int margin, int spacing)
    : QLayout(parent), m_spacing(spacing)
{
    setContentsMargins(margin, margin, margin, margin);
}

FlowLayout::~FlowLayout()
{
    while (QLayoutItem * item = takeAt(0)) {
        delete item;
    }
}

void FlowLayout::addItem(QLayoutItem * item)
{
    m_items.append(item);
}

int FlowLayout::count() const
{
    return static_cast<int>(m_items.size());
}

QLayoutItem * FlowLayout::itemAt(int index) const
{
    return m_items.value(index);
}

QLayoutItem * FlowLayout::takeAt(int index)
{
    if (index < 0 || index >= m_items.size()) {
        return nullptr;
    }

    return m_items.takeAt(index);
}

void FlowLayout::move(int from, int to)
{
    if (from == to || from < 0 || from >= m_items.size() || to < 0 || to >= m_items.size()) {
        return;
    }

    m_items.move(from, to);
    invalidate();
}

int FlowLayout::heightForWidth(int width) const
{
    return place(QRect(0, 0, width, 0), false);
}

void FlowLayout::setGeometry(const QRect & rect)
{
    QLayout::setGeometry(rect);
    place(rect, true);
}

QSize FlowLayout::sizeHint() const
{
    return minimumSize();
}

QSize FlowLayout::minimumSize() const
{
    QSize size;
    for (const QLayoutItem * item : m_items) {
        size = size.expandedTo(item->minimumSize());
    }

    const QMargins margins = contentsMargins();

    return size + QSize(margins.left() + margins.right(), margins.top() + margins.bottom());
}

int FlowLayout::indexAt(const QPoint & position) const
{
    //Read off the GEOMETRIES and not off the order of the list: the
    //placement below fills the holes in a row with whatever item fits, so
    //the item drawn to the left of another is not always the one before it.
    int nearest = -1;
    qint64 shortest = 0;

    for (int i = 0; i < m_items.size(); ++i) {
        const QRect where = m_items.at(i)->geometry();

        //The half of an item the cursor is on decides whether the dragged
        //one goes before or after it: dropping on the left half of a card
        //means "in front of this one".
        if (where.contains(position)) {
            return position.x() < where.center().x() ? i : i + 1;
        }

        const qint64 dx = position.x() < where.left() ? where.left() - position.x()
                        : position.x() > where.right() ? position.x() - where.right() : 0;
        const qint64 dy = position.y() < where.top() ? where.top() - position.y()
                        : position.y() > where.bottom() ? position.y() - where.bottom() : 0;
        const qint64 distance = dx * dx + dy * dy;

        if (nearest < 0 || distance < shortest) {
            nearest = i;
            shortest = distance;
        }
    }

    if (nearest < 0) {
        return static_cast<int>(m_items.size());
    }

    const QRect where = m_items.at(nearest)->geometry();

    return position.x() < where.center().x() ? nearest : nearest + 1;
}

int FlowLayout::place(const QRect & rect, bool apply) const
{
    const QMargins margins = contentsMargins();
    const QRect inside = rect.adjusted(margins.left(), margins.top(), -margins.right(), -margins.bottom());

    int x = inside.x();
    int y = inside.y();
    int rowHeight = 0;

    //In the order they were given, and only in that order: what goes where
    //is the caller's business. The canvas of phases decides which of two
    //phases comes first from how many columns it has, because only two of
    //them may change places - a boundary set that jumped ahead of the
    //specifications it is computed from would be a lie about the design.
    for (QLayoutItem * item : m_items) {
        //Every item at the size it asks for, never at the size that is
        //left: that is the whole point of wrapping.
        QSize size = item->sizeHint();
        size.setWidth(std::min(size.width(), inside.width()));

        if (x > inside.x() && x + size.width() > inside.right() + 1) {
            //It does not fit in what is left of this row: down to the next.
            x = inside.x();
            y += rowHeight + m_spacing;
            rowHeight = 0;
        }

        if (apply) {
            item->setGeometry(QRect(QPoint(x, y), size));
        }

        x += size.width() + m_spacing;
        rowHeight = std::max(rowHeight, size.height());
    }

    return y + rowHeight - rect.y() + margins.bottom();
}

} // namespace qftbx
