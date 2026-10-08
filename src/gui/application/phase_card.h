/**
 * @file
 * @brief One phase of the design as one framed card: its form and its
 * diagrams together.
 *
 * The form, which the card takes over, lives inside it, folded away, with
 * the diagrams below it, in tabs when there are several. Unfolding makes
 * the card taller and the canvas moves the rest out of the way; folding or
 * unfolding never changes the width the card was given. The card says when
 * its computation is running, disables the form meanwhile and offers
 * Cancel on its bar; it can be closed, which its owner answers by hiding
 * it until the phase's step button brings it back, and dragged by its bar,
 * the canvas deciding where it lands while the drag goes on.
 *
 * The canvas works out the square the cards count in from its own width,
 * and a card spans 1 to 3 of them on a side, so a wider card is also
 * taller and its diagram bigger. A span chosen by the user, or restored
 * from the last session, is never overruled by the automatic rule; until
 * then a phase that is only a form sizes itself to its form. The size the
 * card asks for is the width of its form when that is open and otherwise
 * that of a diagram worth looking at; any change of it is signalled so
 * that the canvas lays itself out again.
 */

#ifndef QFTBX_PHASE_CARD_H
#define QFTBX_PHASE_CARD_H

#include <QFrame>
#include <QPoint>
#include <QString>

#include <utility>
#include <vector>

class QLabel;
class QProgressBar;
class QScrollArea;
class QToolButton;
class QVBoxLayout;

namespace qftbx {

class PhaseCard : public QFrame
{
    Q_OBJECT

public:
    PhaseCard(const QString & title, const QString & name, QWidget * form,
              const std::vector<std::pair<QString, QWidget *>> & views,
              QWidget * parent = nullptr);

    bool isFormShown() const;

    void showForm(bool shown);

    void setBusy(bool busy, const QString & what = QString());
    bool isBusy() const { return m_busy; }

    int span() const { return m_span; }

    void setSpan(int columns, bool chosen = false);

    void setUnit(QSize unit);

    static QSize unitFor(int width);
    static int columnsFor(int width);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void cancelAsked();

    void closeAsked();

    void sizeChanged();

    void draggedTo(QPoint where);

    void dragFinished();

private:
    bool eventFilter(QObject * watched, QEvent * event) override;

    QWidget * m_bar = nullptr;
    bool m_dragging = false;
    QWidget * m_form = nullptr;
    QScrollArea * m_formArea = nullptr;
    QWidget * m_views = nullptr;
    QToolButton * m_fold = nullptr;
    QToolButton * m_cancel = nullptr;
    QProgressBar * m_progress = nullptr;
    bool m_busy = false;
    QToolButton * m_narrower = nullptr;
    QToolButton * m_wider = nullptr;
    QToolButton * m_close = nullptr;
    QLabel * m_caption = nullptr;
    QString m_title;
    int m_span = 1;
    bool m_spanChosen = false;
    QSize m_unit;
};

}

#endif
