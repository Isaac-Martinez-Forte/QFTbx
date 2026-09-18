#ifndef QFTBX_FLOW_LAYOUT_H
#define QFTBX_FLOW_LAYOUT_H

#include <QLayout>
#include <QList>
#include <QRect>
#include <QSize>

namespace qftbx {

/**
 * @brief A layout that places its items in a row and WRAPS to the next one
 * when they no longer fit, like words in a paragraph.
 *
 * The layouts Qt ships divide the space they are given: a row of five in a
 * window too narrow for five is five columns too narrow to read. This one
 * does the other thing - it keeps every item at the size it asks for and
 * moves what does not fit to the line below - which is what makes a wall of
 * panels readable at any width. Inside a scroll area, what does not fit
 * downwards is scrolled to.
 *
 * It answers heightForWidth(), which is how the scroll area learns how tall
 * the whole thing is once the width is known. An item whose widget is
 * hidden takes no room: the row closes over it.
 */
class FlowLayout : public QLayout
{
public:
    explicit FlowLayout(QWidget * parent = nullptr, int margin = 8, int spacing = 8);
    ~FlowLayout() override;

    void addItem(QLayoutItem * item) override;
    int count() const override;
    QLayoutItem * itemAt(int index) const override;
    QLayoutItem * takeAt(int index) override;

    /// Moves the item at @p from to the position @p to, which is how a card
    /// dragged over another changes places with it.
    void move(int from, int to);

    Qt::Orientations expandingDirections() const override { return {}; }
    bool hasHeightForWidth() const override { return true; }
    int heightForWidth(int width) const override;
    void setGeometry(const QRect & rect) override;
    QSize sizeHint() const override;
    QSize minimumSize() const override;

    /// Where an item dropped at @p position would land: the index it would
    /// take in the row it falls on.
    int indexAt(const QPoint & position) const;

private:
    /// Lays the items out inside @p rect, or only measures them when
    /// @p apply is false, and answers the height it took.
    int place(const QRect & rect, bool apply) const;

    QList<QLayoutItem *> m_items;
    int m_spacing;
};

} // namespace qftbx

#endif // QFTBX_FLOW_LAYOUT_H
