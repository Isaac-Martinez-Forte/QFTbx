#include "src/gui/application/phase_card.h"

#include <algorithm>

#include <QLabel>
#include <QScrollArea>
#include <QScrollBar>
#include <QTabWidget>
#include <QToolButton>
#include <QMouseEvent>
#include <QProgressBar>
#include <QVBoxLayout>

namespace qftbx {

namespace {

//The canvas is a grid of squares, and every card takes one square, four or
//nine of them. That is what makes a wall of them readable: the eye finds
//the next one without looking for it, and the rows come out flush instead
//of ragged.
//
//The square is NOT a fixed size: it is what the canvas divides its own
//width into, so the cards fill the screen they are given. A column of 560
//on a 1280 screen leaves 144 pixels of nothing on the right, and on a 4K
//one it makes six tiny cards where three big ones were wanted.
constexpr int kMinColumn = 480;
constexpr int kMaxColumns = 4;
constexpr int kColumnSpacing = 8;
constexpr int kMaxSpan = 3;

//The proportion of a square, and the range a row is allowed: a diagram
//shorter than this stops being readable, and taller than this wastes the
//screen of whoever has a tall one.
constexpr double kAspect = 0.72;
constexpr int kMinRow = 360;
constexpr int kMaxRow = 560;

constexpr int kFormHeight = 380;

//The air between the border of a card and what is written inside it.
constexpr int kPadding = 8;

//Given to the widget's own layout, which is where a form decides how far
//from its edge its first label sits.
void pad(QWidget * inside)
{
    if (inside != nullptr && inside->layout() != nullptr) {
        inside->layout()->setContentsMargins(kPadding, kPadding, kPadding, kPadding);
    }
}

int widthOf(QSize unit, int columns)
{
    return columns * unit.width() + (columns - 1) * kColumnSpacing;
}

//A bigger card is bigger, not longer: two columns across and two rows down,
//so a diagram that is given more room is given it in both directions. A
//card three times as wide and as tall as one would be a strip.
int heightOf(QSize unit, int rows)
{
    return rows * unit.height() + (rows - 1) * kColumnSpacing;
}

}

PhaseCard::PhaseCard(const QString & title, const QString & name, QWidget * form,
                     const std::vector<std::pair<QString, QWidget *>> & views,
                     QWidget * parent)
    : QFrame(parent), m_form(form)
{
    setObjectName(name);
    setFrameShape(QFrame::StyledPanel);

    auto * layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    //The bar: the name of the phase and the button that unfolds its form.
    auto * bar = new QWidget(this);
    m_bar = bar;
    bar->setObjectName(name + "Bar");
    bar->setCursor(Qt::OpenHandCursor);
    bar->installEventFilter(this);
    //What the style sheet dresses: every card bar, without naming each one.
    bar->setProperty("cardBar", true);
    bar->setAutoFillBackground(true);
    bar->setBackgroundRole(QPalette::AlternateBase);
    auto * barLayout = new QHBoxLayout(bar);
    barLayout->setContentsMargins(8, 4, 4, 4);

    m_title = title;
    m_caption = new QLabel(title, bar);
    QFont bold = m_caption->font();
    bold.setBold(true);
    m_caption->setFont(bold);
    barLayout->addWidget(m_caption);
    barLayout->addStretch();

    //Only while the phase is computing: a bar that says it is working and
    //the button that gives up on it.
    m_progress = new QProgressBar(bar);
    m_progress->setObjectName(name + "Progress");
    m_progress->setRange(0, 0);
    m_progress->setTextVisible(false);
    m_progress->setMaximumSize(120, 12);
    m_progress->setVisible(false);
    barLayout->addWidget(m_progress);

    m_cancel = new QToolButton(bar);
    m_cancel->setObjectName(name + "Cancel");
    m_cancel->setText(tr("Cancel"));
    m_cancel->setToolTip(tr("Give up on the computation of this phase"));
    m_cancel->setVisible(false);
    connect(m_cancel, &QToolButton::clicked, this, &PhaseCard::cancelAsked);
    barLayout->addWidget(m_cancel);

    m_fold = new QToolButton(bar);
    m_fold->setObjectName(name + "Fold");
    m_fold->setCheckable(true);
    m_fold->setText(tr("Data"));
    m_fold->setToolTip(tr("Show or hide what this phase was asked for"));
    barLayout->addWidget(m_fold);

    //How much of the canvas this phase is worth, which is the user's to
    //say: a diagram he is working on takes four squares of it and the rest
    //wait in one. It is a size of its own, kept when the form is folded or
    //unfolded.
    m_narrower = new QToolButton(bar);
    m_narrower->setObjectName(name + "Narrower");
    m_narrower->setText(QString::fromUtf8("\u2212"));
    m_narrower->setToolTip(tr("Narrower"));
    barLayout->addWidget(m_narrower);

    m_wider = new QToolButton(bar);
    m_wider->setObjectName(name + "Wider");
    m_wider->setText(QString::fromUtf8("+"));
    m_wider->setToolTip(tr("Wider"));
    barLayout->addWidget(m_wider);

    m_close = new QToolButton(bar);
    m_close->setObjectName(name + "Close");
    m_close->setText(QString::fromUtf8("\u2715"));
    m_close->setToolTip(tr("Close this phase. Its button at the top of the window opens it "
                           "again, with everything it holds."));
    connect(m_close, &QToolButton::clicked, this, &PhaseCard::closeAsked);
    barLayout->addWidget(m_close);

    //The bar is the handle and shows the hand for it; its buttons are not,
    //and a hand over something you only press says the wrong thing. A
    //cursor is inherited by every child unless the child says otherwise.
    for (QWidget * pressed : {static_cast<QWidget *>(m_fold),
                              static_cast<QWidget *>(m_cancel),
                              static_cast<QWidget *>(m_narrower),
                              static_cast<QWidget *>(m_wider),
                              static_cast<QWidget *>(m_close)}) {
        pressed->setCursor(Qt::ArrowCursor);
    }

    connect(m_narrower, &QToolButton::clicked, this, [this]() {
        m_spanChosen = true;
        setSpan(m_span - 1);
    });
    connect(m_wider, &QToolButton::clicked, this, [this]() {
        m_spanChosen = true;
        setSpan(m_span + 1);
    });

    layout->addWidget(bar);

    //The form, folded away to start with. In a scroll area because the
    //forms are still drawn at fixed positions: a card narrower than one
    //scrolls instead of cutting its fields off.
    m_formArea = new QScrollArea(this);
    m_formArea->setObjectName(name + "FormArea");
    m_form->setMinimumSize(m_form->size());
    m_formArea->setWidget(m_form);
    m_formArea->setWidgetResizable(true);
    m_formArea->setVisible(false);
    //Air between the border of the card and what is written inside it: the
    //forms lay their fields out from their own edge, and against the frame
    //the labels read as if they were glued to it. Given to the form's own
    //layout, and here rather than in seven .ui files, so that every phase
    //gets the same and a form drawn in Designer needs no margins of its own.
    pad(m_form);

    layout->addWidget(m_formArea);

    if (views.size() == 1) {
        m_views = views.front().second;
        m_views->setParent(this);
    } else if (!views.empty()) {
        auto * tabs = new QTabWidget(this);
        tabs->setObjectName(name + "Views");
        for (const auto & [tabTitle, view] : views) {
            tabs->addTab(view, tabTitle);
        }
        m_views = tabs;
    }

    if (m_views != nullptr) {
        m_views->setMinimumSize(kMinColumn / 2, 200);
        for (const auto & [tabTitle, view] : views) {
            (void) tabTitle;
            pad(view);
        }
        layout->addWidget(m_views, 1);

        //The band the form gets when it is unfolded, and not a pixel more:
        //without a height of its own the diagram below took everything the
        //layout would give it and the form came out as a sliver with its
        //fields behind it.
        m_formArea->setFixedHeight(kFormHeight);
    } else {
        //A phase that is only a form has nothing to fold: its form is the
        //card, and it takes the whole of it.
        m_formArea->setVisible(true);
        m_fold->setVisible(false);
        layout->setStretch(1, 1);
    }

    connect(m_fold, &QToolButton::toggled, this, [this](bool on) {
        m_formArea->setVisible(on);
        updateGeometry();
        emit sizeChanged();
    });

}

//The bar is the handle of the card: pressed and moved, it drags the whole
//phase to another place on the canvas.
bool PhaseCard::eventFilter(QObject * watched, QEvent * event)
{
    if (watched != m_bar) {
        return QFrame::eventFilter(watched, event);
    }

    auto * mouse = dynamic_cast<QMouseEvent *>(event);

    if (mouse == nullptr) {
        return QFrame::eventFilter(watched, event);
    }

    switch (event->type()) {
    case QEvent::MouseButtonPress:
        if (mouse->button() == Qt::LeftButton) {
            m_dragging = true;
            m_bar->setCursor(Qt::ClosedHandCursor);
        }
        break;
    case QEvent::MouseMove:
        if (m_dragging) {
            emit draggedTo(mouse->globalPosition().toPoint());
        }
        break;
    case QEvent::MouseButtonRelease:
        if (m_dragging) {
            m_dragging = false;
            m_bar->setCursor(Qt::OpenHandCursor);
            emit dragFinished();
        }
        break;
    default:
        break;
    }

    return QFrame::eventFilter(watched, event);
}

void PhaseCard::setBusy(bool busy, const QString & what)
{
    m_busy = busy;

    m_progress->setVisible(busy);
    m_cancel->setVisible(busy);
    m_fold->setEnabled(!busy);
    m_close->setEnabled(!busy);

    //The form is the input of the computation that is running: touching it
    //while it runs would describe something else.
    m_formArea->setEnabled(!busy);

    m_caption->setText(busy && !what.isEmpty() ? what : m_title);
}

bool PhaseCard::isFormShown() const
{
    //The button and not the widget: a widget inside a window that has never
    //been shown is not visible, whatever its card has been told to do, and
    //this answers what the card was told.
    return m_views == nullptr || m_fold->isChecked();
}

void PhaseCard::showForm(bool shown)
{
    if (m_views == nullptr) {
        //Nothing to fold.
        return;
    }

    m_fold->setChecked(shown);
}

int PhaseCard::columnsFor(int width)
{
    const int fits = (width + kColumnSpacing) / (kMinColumn + kColumnSpacing);

    return std::clamp(fits, 1, kMaxColumns);
}

QSize PhaseCard::unitFor(int width)
{
    const int columns = columnsFor(width);
    //Exactly the width given, shared out: whatever is left over would be a
    //ragged margin down the right of the canvas.
    const int column = std::max(kMinColumn,
                                (width - kColumnSpacing * (columns - 1)) / columns);

    return QSize(column, std::clamp(static_cast<int>(column * kAspect), kMinRow, kMaxRow));
}

void PhaseCard::setUnit(QSize unit)
{
    if (unit == m_unit || !unit.isValid()) {
        return;
    }

    m_unit = unit;

    //A phase that is only a form takes two squares when its form does not
    //fit in one - the specifications, drawn with two models side by side.
    //Only until the user says otherwise: from then on the size is his.
    if (!m_spanChosen && m_views == nullptr) {
        setSpan(m_form->minimumWidth() > m_unit.width() ? 2 : 1);
    }

    updateGeometry();
}

void PhaseCard::setSpan(int columns, bool chosen)
{
    const int wanted = std::clamp(columns, 1, kMaxSpan);

    m_spanChosen = m_spanChosen || chosen;

    m_narrower->setEnabled(wanted > 1);
    m_wider->setEnabled(wanted < kMaxSpan);

    if (wanted == m_span) {
        return;
    }

    m_span = wanted;
    updateGeometry();
    emit sizeChanged();
}

QSize PhaseCard::sizeHint() const
{
    //The width is the span the user chose, and the fold only adds the band
    //of the form BELOW the diagram. Widening on a fold would push the card
    //beside this one into the row below every time somebody opens a form.
    const QSize unit = m_unit.isValid() ? m_unit : unitFor(kMinColumn);
    const int width = widthOf(unit, m_span);

    //A phase that is only a form gets no taller by being wider: there is no
    //diagram in it to see better.
    if (m_views == nullptr) {
        return QSize(width, unit.height());
    }

    const int height = heightOf(unit, m_span);

    return QSize(width, isFormShown() ? height + kFormHeight : height);
}

QSize PhaseCard::minimumSizeHint() const
{
    //Never smaller than half a card: the canvas wraps and scrolls rather
    //than shrinking anything, but a window narrower than one card has to
    //show something.
    return QSize(kMinColumn / 2, 240);
}

} // namespace qftbx
