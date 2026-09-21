/**
 * @file
 * @brief Builds the list of shipped examples and reports the one chosen.
 *
 * The list is read once when the dialog opens, since the directory belongs to
 * the installation and does not change under it. Each row costs the root
 * element of its file and nothing more, so moving down the list is immediate
 * however heavy the projects are. A dialog built where there is no such
 * directory says so and offers nothing, which is what an incomplete
 * installation looks like from here.
 */

#include "src/gui/application/examples_dialog.h"

#include <QDialogButtonBox>
#include <QFont>
#include <QFrame>
#include <QScrollArea>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

namespace qftbx {

ExamplesDialog::ExamplesDialog(QWidget * parent, const QString & selected)
    : QDialog(parent)
{
    setObjectName("examplesDialog");
    setWindowTitle(tr("Examples"));

    m_examples = shippedExamples();

    m_list = new QListWidget(this);
    m_list->setObjectName("examplesList");
    for (const Example & example : m_examples) {
        m_list->addItem(example.name);
    }

    m_title = new QLabel(this);
    m_title->setObjectName("exampleName");
    m_title->setWordWrap(true);
    m_title->setTextInteractionFlags(Qt::TextSelectableByMouse);
    QFont heading = m_title->font();
    heading.setBold(true);
    heading.setPointSizeF(heading.pointSizeF() * 1.15);
    m_title->setFont(heading);

    m_description = new QLabel(this);
    m_description->setObjectName("exampleDescription");
    m_description->setWordWrap(true);
    m_description->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_description->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_description->setMinimumWidth(320);

    m_source = new QLabel(this);
    m_source->setObjectName("exampleSource");
    m_source->setWordWrap(true);
    m_source->setTextFormat(Qt::RichText);
    m_source->setOpenExternalLinks(true);
    m_source->setTextInteractionFlags(Qt::TextBrowserInteraction);

    QWidget * pane = new QWidget(this);
    QVBoxLayout * paneLayout = new QVBoxLayout(pane);
    paneLayout->setContentsMargins(0, 0, 12, 0);
    paneLayout->setSpacing(10);
    paneLayout->addWidget(m_title);
    paneLayout->addWidget(m_description);
    paneLayout->addWidget(m_source);
    paneLayout->addStretch(1);

    QScrollArea * reading = new QScrollArea(this);
    reading->setObjectName("exampleReading");
    reading->setWidget(pane);
    reading->setWidgetResizable(true);
    reading->setFrameShape(QFrame::NoFrame);
    reading->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QDialogButtonBox * buttons = new QDialogButtonBox(this);
    buttons->setObjectName("examplesButtons");
    m_open = buttons->addButton(tr("Open"), QDialogButtonBox::AcceptRole);
    m_open->setObjectName("openExampleButton");
    QPushButton * cancel = buttons->addButton(QDialogButtonBox::Cancel);
    cancel->setObjectName("cancelExampleButton");

    QHBoxLayout * columns = new QHBoxLayout;
    columns->setSpacing(16);
    columns->addWidget(m_list, 2);
    columns->addWidget(reading, 3);

    QVBoxLayout * layout = new QVBoxLayout(this);
    if (m_examples.empty()) {
        QLabel * missing = new QLabel(
                    tr("No examples were found. The installation carries them in "
                       "a folder of its own, which this build resolves to:"), this);
        missing->setObjectName("examplesMissing");
        missing->setWordWrap(true);
        layout->addWidget(missing);

        QLabel * where = new QLabel(examplesDirectory(), this);
        where->setObjectName("examplesDirectory");
        where->setWordWrap(true);
        where->setTextInteractionFlags(Qt::TextSelectableByMouse);
        layout->addWidget(where);
    }
    layout->addLayout(columns);
    layout->addWidget(buttons);

    connect(m_list, &QListWidget::currentRowChanged, this,
            [this](int row) { showDescriptionOf(row); });
    connect(m_list, &QListWidget::itemDoubleClicked, this,
            [this](QListWidgetItem * item) { acceptRow(m_list->row(item)); });
    connect(m_open, &QPushButton::clicked, this,
            [this]() { acceptRow(m_list->currentRow()); });
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);

    if (!m_examples.empty()) {
        int row = 0;
        for (std::size_t i = 0; i < m_examples.size(); ++i) {
            if (m_examples[i].path == selected) {
                row = static_cast<int>(i);
                break;
            }
        }
        m_list->setCurrentRow(row);
        m_list->setFocus();
    } else {
        m_open->setEnabled(false);
        showDescriptionOf(-1);
    }

    resize(760, 340);
}

void ExamplesDialog::showDescriptionOf(int row)
{
    if (row < 0 || static_cast<std::size_t>(row) >= m_examples.size()) {
        m_title->clear();
        m_description->clear();
        m_source->clear();
        m_source->setVisible(false);
        m_open->setEnabled(false);
        return;
    }

    const Example & example = m_examples[static_cast<std::size_t>(row)];
    m_title->setText(example.name);
    m_description->setText(example.description);

    const QString address = doiAddress(example.doi);
    m_source->setVisible(!address.isEmpty());
    if (address.isEmpty()) {
        m_source->clear();
    } else {
        m_source->setText(tr("Source: <a href=\"%1\">%2</a>")
                          .arg(address, example.doi.toHtmlEscaped()));
    }

    m_open->setEnabled(true);
}

void ExamplesDialog::acceptRow(int row)
{
    if (row < 0 || static_cast<std::size_t>(row) >= m_examples.size()) {
        return;
    }

    m_chosen = m_examples[static_cast<std::size_t>(row)].path;
    accept();
}

}
