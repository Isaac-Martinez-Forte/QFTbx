#include "src/gui/bench/results_view.h"

#include <algorithm>
#include <map>
#include <set>

#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QSplitter>
#include <QTableWidget>
#include <QVBoxLayout>

#include "qcustomplot.h"

#include "src/gui/application/error_message.h"
#include "src/gui/common/plot_palette.h"

namespace qftbx {

namespace {

enum Column { Structure, Algorithm, Epsilon, Runs, WallMedian, WallCv, CpuMedian, PeakMemory, AlgorithmMemory,
              PeakNodes, Nodes, Verdicts, Gain, Agree, ColumnCount };

enum Measure { MeasureWall, MeasureCpu, MeasurePeakMemory, MeasureAlgorithmMemory, MeasurePeakNodes, MeasureNodes };

double valueOf(const bench::Aggregate & a, int measure)
{
    switch (measure) {
    case MeasureWall: return a.wallMilliseconds.median / 1000.0;
    case MeasureCpu: return a.cpuMilliseconds.median / 1000.0;
    case MeasurePeakMemory: return a.peakMemoryMegabytes.median;
    case MeasureAlgorithmMemory: return a.algorithmMemoryMegabytes.median;
    case MeasurePeakNodes: return static_cast<double>(a.statistics.peakLiveNodes);
    case MeasureNodes: return static_cast<double>(a.statistics.nodesProcessed);
    }
    return 0.0;
}

QTableWidgetItem * cell(const QString & text)
{
    auto * item = new QTableWidgetItem(text);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    return item;
}

} // namespace

ResultsView::ResultsView(QWidget * parent) : QWidget(parent)
{
    auto * layout = new QVBoxLayout(this);
    auto * splitter = new QSplitter(Qt::Vertical, this);
    layout->addWidget(splitter);

    m_table = new QTableWidget(0, ColumnCount, splitter);
    m_table->setObjectName("summary");
    m_table->setHorizontalHeaderLabels({tr("Structure"), tr("Algorithm"), tr("Epsilon"), tr("Runs"), tr("Wall median (s)"),
                                        tr("Wall CV"), tr("CPU median (s)"), tr("Peak memory (MB)"),
                                        tr("Algorithm memory (MB)"), tr("Peak live nodes"), tr("Nodes"),
                                        tr("Stability verdicts"), tr("k"), tr("Agree")});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->verticalHeader()->setVisible(false);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    splitter->addWidget(m_table);

    auto * figure = new QWidget(splitter);
    auto * figureLayout = new QVBoxLayout(figure);
    auto * controls = new QHBoxLayout();
    controls->addWidget(new QLabel(tr("Figure:"), figure));
    m_measure = new QComboBox(figure);
    m_measure->setObjectName("figureMeasure");
    m_measure->addItems({tr("Wall time, median (s)"), tr("CPU time, median (s)"), tr("Peak memory (MB)"),
                         tr("Algorithm memory (MB)"), tr("Peak live nodes"), tr("Nodes processed")});
    controls->addWidget(m_measure);
    m_logarithmic = new QCheckBox(tr("logarithmic axis"), figure);
    m_logarithmic->setChecked(true);
    controls->addWidget(m_logarithmic);
    controls->addStretch();
    auto * exportCsv = new QPushButton(tr("Export CSV..."), figure);
    auto * exportMarkdown = new QPushButton(tr("Export Markdown..."), figure);
    controls->addWidget(exportCsv);
    controls->addWidget(exportMarkdown);
    figureLayout->addLayout(controls);
    m_plot = new QCustomPlot(figure);
    m_plot->setObjectName("figure");
    m_plot->setMinimumHeight(220);
    m_plot->legend->setVisible(true);
    m_plot->xAxis->setLabel(tr("controller structure"));
    figureLayout->addWidget(m_plot);
    m_note = new QLabel(figure);
    m_note->setWordWrap(true);
    figureLayout->addWidget(m_note);
    splitter->addWidget(figure);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);

    connect(m_measure, &QComboBox::currentIndexChanged, this, [this]() { draw(); });
    connect(m_logarithmic, &QCheckBox::toggled, this, [this]() { draw(); });
    connect(exportCsv, &QPushButton::clicked, this, [this]() { this->exportCsv(); });
    connect(exportMarkdown, &QPushButton::clicked, this, [this]() { this->exportMarkdown(); });
}

int ResultsView::rowCount() const
{
    return m_table->rowCount();
}

void ResultsView::clear()
{
    m_aggregates.clear();
    fillTable();
    draw();
}

void ResultsView::show(const std::vector<bench::Aggregate> & aggregates)
{
    m_aggregates = aggregates;
    fillTable();
    draw();
}

void ResultsView::fillTable()
{
    m_table->setRowCount(0);
    m_table->setRowCount(static_cast<int>(m_aggregates.size()));
    int row = 0;
    for (const bench::Aggregate & a : m_aggregates) {
        QString runs = QStringLiteral("%1/%2").arg(a.solved).arg(a.runs);
        if (a.infeasible > 0) {
            runs += tr(" (%1 infeasible)").arg(a.infeasible);
        }
        if (a.failed > 0) {
            runs += tr(" (%1 failed)").arg(a.failed);
        }
        m_table->setItem(row, Structure, cell(QString::fromStdString(a.structure)));
        m_table->setItem(row, Algorithm, cell(QString::fromStdString(a.algorithm)));
        m_table->setItem(row, Epsilon, cell(QString::number(a.epsilon)));
        m_table->setItem(row, Runs, cell(runs));
        m_table->setItem(row, WallMedian, cell(QString::number(a.wallMilliseconds.median / 1000.0, 'f', 3)));
        m_table->setItem(row, WallCv, cell(QString::number(100.0 * a.wallMilliseconds.coefficientOfVariation, 'f', 1) + " %"));
        m_table->setItem(row, CpuMedian, cell(QString::number(a.cpuMilliseconds.median / 1000.0, 'f', 3)));
        m_table->setItem(row, PeakMemory, cell(QString::number(a.peakMemoryMegabytes.median, 'f', 1)));
        m_table->setItem(row, AlgorithmMemory, cell(QString::number(a.algorithmMemoryMegabytes.median, 'f', 1)));
        m_table->setItem(row, PeakNodes, cell(QString::number(a.statistics.peakLiveNodes)));
        m_table->setItem(row, Nodes, cell(QString::number(a.statistics.nodesProcessed)));
        m_table->setItem(row, Verdicts, cell(QString::number(a.statistics.stabilityVerdicts)));
        m_table->setItem(row, Gain, cell(QString::number(a.gain, 'g', 8)));
        QTableWidgetItem * agree = cell(a.resultsAgree && a.countersAgree ? tr("yes") : tr("NO"));
        if (!(a.resultsAgree && a.countersAgree)) {
            agree->setForeground(Qt::red);
        }
        m_table->setItem(row, Agree, agree);
        ++row;
    }
}

void ResultsView::draw()
{
    m_plot->clearGraphs();
    const int measure = m_measure->currentIndex();

    //The structures in order of steps applied, as the categories of the x
    //axis; one curve per algorithm, drawn at the epsilon the plan ran (the
    //figure takes the first when there are several).
    std::map<std::size_t, QString> structures;
    std::set<std::string> algorithms;
    double epsilon = 0.0;
    for (const bench::Aggregate & a : m_aggregates) {
        structures[a.stepsApplied] = QString::fromStdString(a.structure);
        algorithms.insert(a.algorithm);
        if (epsilon == 0.0) {
            epsilon = a.epsilon;
        }
    }

    QSharedPointer<QCPAxisTickerText> ticker(new QCPAxisTickerText);
    std::map<std::size_t, double> position;
    double x = 0.0;
    for (const auto & [steps, label] : structures) {
        position[steps] = x;
        ticker->addTick(x, label);
        x += 1.0;
    }
    m_plot->xAxis->setTicker(ticker);
    m_plot->xAxis->setRange(-0.5, x - 0.5);

    int colour = 0;
    double low = 0.0;
    double high = 0.0;
    bool any = false;
    for (const std::string & algorithm : algorithms) {
        QVector<double> xs, ys;
        for (const bench::Aggregate & a : m_aggregates) {
            if (a.algorithm != algorithm || a.epsilon != epsilon || a.solved == 0) {
                continue;
            }
            const double value = valueOf(a, measure);
            xs.push_back(position[a.stepsApplied]);
            ys.push_back(value);
            low = any ? std::min(low, value) : value;
            high = any ? std::max(high, value) : value;
            any = true;
        }
        QCPGraph * graph = m_plot->addGraph();
        graph->setName(QString::fromStdString(algorithm));
        graph->setPen(QPen(randomColor(colour), 2));
        graph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, 7));
        graph->setData(xs, ys);
        ++colour;
    }

    const bool logarithmic = m_logarithmic->isChecked() && any && low > 0.0;
    if (logarithmic) {
        m_plot->yAxis->setScaleType(QCPAxis::stLogarithmic);
        m_plot->yAxis->setTicker(QSharedPointer<QCPAxisTickerLog>(new QCPAxisTickerLog));
        m_plot->yAxis->setRange(low / 2.0, high * 2.0);
    } else {
        m_plot->yAxis->setScaleType(QCPAxis::stLinear);
        m_plot->yAxis->setTicker(QSharedPointer<QCPAxisTicker>(new QCPAxisTicker));
        m_plot->yAxis->setRange(0.0, any ? high * 1.1 : 1.0);
    }
    m_plot->yAxis->setLabel(m_measure->currentText());
    m_note->setText(epsilon > 0.0 ? tr("Median over the repetitions, epsilon %1. A structure an algorithm did not solve is left out.").arg(epsilon)
                                  : QString());
    m_plot->replot();
}

QString ResultsView::chooseFile(const QString & filter)
{
    if (m_chooseFile) {
        return m_chooseFile(true, filter);
    }
    return QFileDialog::getSaveFileName(this, tr("Export"), QString(), filter);
}

void ResultsView::exportCsv()
{
    const QString path = chooseFile(tr("CSV (*.csv)"));
    if (path.isEmpty()) {
        return;
    }
    try {
        bench::writeCsv(m_aggregates, path.toStdString());
    } catch (const std::exception & failure) {
        errorMessage(QString::fromUtf8(failure.what()), tr("Export"));
    }
}

void ResultsView::exportMarkdown()
{
    const QString path = chooseFile(tr("Markdown (*.md)"));
    if (path.isEmpty()) {
        return;
    }
    try {
        bench::writeMarkdown(m_aggregates, path.toStdString());
    } catch (const std::exception & failure) {
        errorMessage(QString::fromUtf8(failure.what()), tr("Export"));
    }
}

} // namespace qftbx
