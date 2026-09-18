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

/**
 * @brief One phase of the design as one thing on the screen: what it is
 * asked for and what came out of it, together.
 *
 * The form of the phase lives INSIDE the card, folded away, and its
 * diagrams below it. Unfolding it makes the card taller and wider, and the
 * canvas moves the rest out of the way - which is the difference between
 * this and the docks it replaces: a dock took its space from its
 * neighbours, and six of them left six slivers nobody could read.
 *
 * The card is framed and its name is on a bar of its own, so that where one
 * phase ends and the next begins is never in doubt.
 */
class PhaseCard : public QFrame
{
    Q_OBJECT

public:
    /**
     * @param title the name of the phase, on the bar.
     * @param name the object name, which its parts derive theirs from.
     * @param form the form of the phase; the card takes it.
     * @param views its diagrams, each with the name of its tab. One is
     *        shown on its own, several in tabs.
     */
    PhaseCard(const QString & title, const QString & name, QWidget * form,
              const std::vector<std::pair<QString, QWidget *>> & views,
              QWidget * parent = nullptr);

    /// Whether the form is unfolded.
    bool isFormShown() const;

    /// Unfolds the form, or folds it away. The width of the card is not its
    /// business: a card made wide stays wide with its form open or closed.
    void showForm(bool shown);

    /**
     * @brief Says that the computation of this phase is running, or that it
     * is not.
     *
     * A card that is working says so on its bar and puts a bar of its own
     * across it; the form below is not to be touched while its numbers are
     * being used, so it is disabled. What the user can still do is scroll,
     * look at the other phases - and press Cancel, which is what the button
     * on the bar becomes.
     */
    void setBusy(bool busy, const QString & what = QString());
    bool isBusy() const { return m_busy; }

    /// How many squares of the canvas the card takes on a side, 1 to 3: a
    /// card of 2 is two columns across and two rows down, which is what
    /// makes its diagram bigger rather than wider.
    int span() const { return m_span; }

    /// @param chosen whether this is the USER's size, which the automatic
    /// rule below must not overrule afterwards: a size restored from the
    /// last session is the user's, the one a first run starts from is not.
    void setSpan(int columns, bool chosen = false);

    /// The square of the canvas this card counts in, which the canvas
    /// works out from its own width: the cards fill the screen they are
    /// given instead of leaving a margin of dead pixels on a wide one and
    /// crowding a narrow one.
    void setUnit(QSize unit);

    /// The size of the square, and how many of them fit across @p width.
    static QSize unitFor(int width);
    static int columnsFor(int width);

    /// How wide and how tall the card asks to be, which is what the canvas
    /// lays out: the width of its form when that is open, and otherwise the
    /// width of a diagram worth looking at.
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    /// The user pressed Cancel on the bar of a card that is working.
    void cancelAsked();

    /// The user closed this phase. The card is hidden by whoever owns the
    /// canvas, and the step button of the phase brings it back.
    void closeAsked();

    /// The card wants a different size - its form was folded or unfolded,
    /// or it was made wider or narrower. The canvas lays itself out again.
    void sizeChanged();

    /// The user is dragging the card by its bar, and the cursor is at
    /// @p where (in screen coordinates). The canvas decides where it lands
    /// while the drag goes on, so the hole opens under the cursor instead
    /// of at the end of it.
    void draggedTo(QPoint where);

    /// The drag ended, wherever it ended.
    void dragFinished();

private:
    /// The bar is the handle: pressing it and moving drags the card.
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
    /// Whether the user has said how big this card should be. Until he
    /// does, a phase that is only a form sizes itself to its form.
    bool m_spanChosen = false;
    QSize m_unit;
};

} // namespace qftbx

#endif // QFTBX_PHASE_CARD_H
