/**
 * @file
 * @brief A checkable button whose arrow points where what it folds will
 * open, and turns round when it does.
 *
 * Declares the button that folds a part of a card away and back. Beside
 * its text it draws a small triangle that points the way the part unfolds
 * while it is folded, and the other way while it is open, turning half a
 * circle in a short animation when it is pressed. The arrow takes the
 * colour of the button's text, and is drawn again when the theme changes.
 */

#ifndef QFTBX_FOLD_BUTTON_H
#define QFTBX_FOLD_BUTTON_H

#include <QToolButton>

class QVariantAnimation;

namespace qftbx {

class FoldButton : public QToolButton
{
    Q_OBJECT

public:
    explicit FoldButton(Qt::ArrowType opensTowards, QWidget * parent = nullptr);

    qreal arrowAngle() const { return m_angle; }

protected:
    void checkStateSet() override;
    void changeEvent(QEvent * event) override;

private:
    void turnTo(qreal angle);
    void drawArrow();
    qreal restingAngle(bool open) const;

    qreal m_folded = 0.0;
    qreal m_angle = 0.0;
    QVariantAnimation * m_turn = nullptr;
};

}

#endif
