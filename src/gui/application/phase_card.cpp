/**
 * @file
 * @brief Layout rules and behaviour of a phase card.
 *
 * The canvas is a grid of squares whose side is what the canvas divides its
 * own width into, within bounds, so the cards fill any screen without a
 * ragged margin; a row is a fixed fraction of a column, clamped so a
 * diagram stays readable. A card of span two or three grows in both
 * directions, so its diagram gets bigger rather than wider. The form sits
 * in a scroll area of fixed height, so unfolding it adds a band below the
 * diagram and never widens the card. A phase that is only a form has
 * nothing to fold, takes the whole card, and asks for two squares when its
 * form does not fit in one, until the user chooses a size. The bar is the
 * drag handle and shows the hand; its buttons keep the arrow.
 */

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

constexpr int kMinColumn = 480;
constexpr int kMaxColumns = 4;
constexpr int kColumnSpacing = 8;
constexpr int kMaxSpan = 3;

constexpr double kAspect = 0.72;
constexpr int kMinRow = 360;
constexpr int kMaxRow = 560;

constexpr int kFormHeight = 380;

constexpr int kPadding = 8;

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

    auto * bar = new QWidget(this);
    m_bar = bar;
    bar->setObjectName(name + "Bar");
    bar->setCursor(Qt::OpenHandCursor);
    bar->installEventFilter(this);
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

    m_formArea = new QScrollArea(this);
    m_formArea->setObjectName(name + "FormArea");
    m_form->setMinimumSize(m_form->size());
    m_formArea->setWidget(m_form);
    m_formArea->setWidgetResizable(true);
    m_formArea->setVisible(false);
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

        m_formArea->setFixedHeight(kFormHeight);
    } else {
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

    m_formArea->setEnabled(!busy);

    m_caption->setText(busy && !what.isEmpty() ? what : m_title);
}

bool PhaseCard::isFormShown() const
{
    return m_views == nullptr || m_fold->isChecked();
}

void PhaseCard::showForm(bool shown)
{
    if (m_views == nullptr) {
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
    const QSize unit = m_unit.isValid() ? m_unit : unitFor(kMinColumn);
    const int width = widthOf(unit, m_span);

    if (m_views == nullptr) {
        return QSize(width, unit.height());
    }

    const int height = heightOf(unit, m_span);

    return QSize(width, isFormShown() ? height + kFormHeight : height);
}

QSize PhaseCard::minimumSizeHint() const
{
    return QSize(kMinColumn / 2, 240);
}

}
