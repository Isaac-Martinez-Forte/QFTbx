#include "src/gui/common/plot_export.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QPainter>
#include <QSvgGenerator>

#include "qcustomplot.h"
#include "src/gui/application/error_message.h"

namespace qftbx {

namespace {

//The figure a paper wants, applied to the plot and undone afterwards: the
//viewer keeps whatever the interface's theme gave it.
class PublishingLook
{
public:
    explicit PublishingLook(QCustomPlot & plot, bool apply) : m_plot(plot), m_applied(apply)
    {
        if (!m_applied) {
            return;
        }

        m_background = m_plot.background();
        m_plot.setBackground(Qt::white);

        for (QCPAxis * axis : {m_plot.xAxis, m_plot.yAxis, m_plot.xAxis2, m_plot.yAxis2}) {
            m_axes.push_back({axis, axis->basePen(), axis->tickPen(), axis->subTickPen(),
                              axis->labelColor(), axis->tickLabelColor(), axis->labelFont(),
                              axis->tickLabelFont()});
            axis->setBasePen(QPen(Qt::black, 1.0));
            axis->setTickPen(QPen(Qt::black, 1.0));
            axis->setSubTickPen(QPen(Qt::black, 0.8));
            axis->setLabelColor(Qt::black);
            axis->setTickLabelColor(Qt::black);
            axis->setLabelFont(heavier(axis->labelFont()));
            axis->setTickLabelFont(heavier(axis->tickLabelFont()));
        }

        for (int i = 0; i < m_plot.plottableCount(); ++i) {
            QCPAbstractPlottable * plottable = m_plot.plottable(i);
            QPen pen = plottable->pen();
            m_pens.push_back({plottable, pen});
            pen.setWidthF(pen.widthF() * 1.6);
            plottable->setPen(pen);
        }
    }

    ~PublishingLook()
    {
        if (!m_applied) {
            return;
        }

        m_plot.setBackground(m_background);
        for (const Axis & a : m_axes) {
            a.axis->setBasePen(a.base);
            a.axis->setTickPen(a.tick);
            a.axis->setSubTickPen(a.subTick);
            a.axis->setLabelColor(a.label);
            a.axis->setTickLabelColor(a.tickLabel);
            a.axis->setLabelFont(a.labelFont);
            a.axis->setTickLabelFont(a.tickLabelFont);
        }
        for (const Pen & p : m_pens) {
            p.plottable->setPen(p.pen);
        }
    }

    PublishingLook(const PublishingLook &) = delete;
    PublishingLook & operator=(const PublishingLook &) = delete;

private:
    static QFont heavier(QFont font)
    {
        font.setPointSizeF(font.pointSizeF() * 1.25);
        return font;
    }

    struct Axis {
        QCPAxis * axis;
        QPen base, tick, subTick;
        QColor label, tickLabel;
        QFont labelFont, tickLabelFont;
    };
    struct Pen { QCPAbstractPlottable * plottable; QPen pen; };

    QCustomPlot & m_plot;
    bool m_applied;
    QBrush m_background;
    std::vector<Axis> m_axes;
    std::vector<Pen> m_pens;
};

bool saveSvg(QCustomPlot & plot, const ExportRequest & request)
{
    //QCustomPlot has no SVG of its own, but it paints onto any QPainter and
    //QSvgGenerator is one: the curves travel as paths, not as pixels.
    const QSize size = request.size.isEmpty() ? plot.size() : request.size;

    QSvgGenerator generator;
    generator.setFileName(request.fileName);
    generator.setSize(size);
    generator.setViewBox(QRect(QPoint(0, 0), size));
    generator.setTitle(QObject::tr("QFTbx"));

    QCPPainter painter;
    if (!painter.begin(&generator)) {
        return false;
    }
    painter.setMode(QCPPainter::pmVectorized);
    painter.setMode(QCPPainter::pmNoCaching);
    plot.toPainter(&painter, size.width(), size.height());
    painter.end();

    return QFileInfo::exists(request.fileName);
}

} // namespace

QString exportFilter()
{
    return QObject::tr("Vector (*.pdf);;Vector (*.svg);;Image (*.png)");
}

bool savePlot(QCustomPlot & plot, const ExportRequest & request)
{
    if (request.fileName.isEmpty()) {
        return false;
    }

    const PublishingLook look(plot, request.profile == ExportProfile::ForPublishing);
    const QString suffix = QFileInfo(request.fileName).suffix().toLower();
    const QSize size = request.size;

    if (suffix == "pdf") {
        return plot.savePdf(request.fileName, size.width(), size.height());
    }
    if (suffix == "svg") {
        return saveSvg(plot, request);
    }
    if (suffix == "png") {
        return plot.savePng(request.fileName, size.width(), size.height(), request.scale);
    }

    return false;
}

void exportPlot(QWidget * parent, QCustomPlot & plot, const QString & title)
{
    QString selected;
    QString fileName = QFileDialog::getSaveFileName(parent, QObject::tr("Save figure"), QString(),
                                                    exportFilter(), &selected);
    if (fileName.isEmpty()) {
        return;
    }

    //The dialog does not always add the suffix, and the suffix is what picks
    //the format: take it from the filter the user chose.
    if (QFileInfo(fileName).suffix().isEmpty()) {
        for (const QString & known : {"pdf", "svg", "png"}) {
            if (selected.contains("." + known, Qt::CaseInsensitive)) {
                fileName += "." + known;
                break;
            }
        }
    }

    ExportRequest request;
    request.fileName = fileName;
    //Twice the plot's own size, so that a small window does not fix the
    //figure at a small size; a vector format ignores it beyond the layout.
    request.size = plot.size() * 2;
    request.profile = ExportProfile::ForPublishing;

    if (!savePlot(plot, request)) {
        errorMessage(QObject::tr("The figure could not be saved"), title);
    }
}

} // namespace qftbx
