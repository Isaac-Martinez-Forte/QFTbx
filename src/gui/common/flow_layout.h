/**
 * @file
 * @brief A layout that wraps its items into rows like words in a paragraph.
 *
 * Every item keeps the size it asks for and what does not fit goes to the
 * next line, so a wall of panels reads at any width; inside a scroll area
 * the overflow is scrolled to. The layout answers height-for-width, which
 * is how the scroll area learns the total height once the width is known,
 * and a hidden widget takes no room. It also moves an item to another
 * position and says where a point would land, which is what dragging a
 * card over another needs.
 */

#ifndef QFTBX_FLOW_LAYOUT_H
#define QFTBX_FLOW_LAYOUT_H

#include <QLayout>
#include <QList>
#include <QRect>
#include <QSize>

namespace qftbx {

class FlowLayout : public QLayout
{
public:
    explicit FlowLayout(QWidget * parent = nullptr, int margin = 8, int spacing = 8);
    ~FlowLayout() override;

    void addItem(QLayoutItem * item) override;
    int count() const override;
    QLayoutItem * itemAt(int index) const override;
    QLayoutItem * takeAt(int index) override;

    void move(int from, int to);

    Qt::Orientations expandingDirections() const override { return {}; }
    bool hasHeightForWidth() const override { return true; }
    int heightForWidth(int width) const override;
    void setGeometry(const QRect & rect) override;
    QSize sizeHint() const override;
    QSize minimumSize() const override;

    int indexAt(const QPoint & position) const;

private:
    int place(const QRect & rect, bool apply) const;

    QList<QLayoutItem *> m_items;
    int m_spacing;
};

}

#endif
