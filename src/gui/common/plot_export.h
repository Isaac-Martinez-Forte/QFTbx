#ifndef QFTBX_GUI_PLOT_EXPORT_H
#define QFTBX_GUI_PLOT_EXPORT_H

#include <QSize>
#include <QString>

class QCustomPlot;
class QWidget;

namespace qftbx {

/// What a figure is being exported for.
enum class ExportProfile {
    /// As it looks on screen, at the size asked for.
    AsSeen,
    /// For a paper: white ground, black axes, heavier strokes and larger
    /// type, whatever the interface's own theme is. A figure is printed on
    /// white.
    ForPublishing
};

/// How a plot is to be written out.
struct ExportRequest
{
    QString fileName;
    /// The pixel size of a raster export. A zero size means the plot's own,
    /// which is the window's and therefore nobody's choice.
    QSize size;
    /// Multiplies the raster size without changing the layout: 2 gives the
    /// same figure at twice the resolution.
    double scale = 1.0;
    ExportProfile profile = ExportProfile::AsSeen;
};

/**
 * @brief Writes one plot to a file: PNG, PDF or SVG.
 *
 * Things to keep in mind:
 * - PDF and SVG are VECTOR: they carry the curves, not a picture of them, so
 *   they do not blur and a journal can set them at any size. PNG is there
 *   for a slide or a message.
 * - A raster export at the plot's own size is the size of the window at that
 *   moment, which is nobody's decision: ask for one.
 * - JPEG and BMP are not offered. One degrades the lines it exists to show
 *   and the other is large for nothing.
 */
bool savePlot(QCustomPlot & plot, const ExportRequest & request);

/// Asks for a file name and the size, then writes it, reporting through
/// errorMessage() when it cannot.
void exportPlot(QWidget * parent, QCustomPlot & plot, const QString & title);

/// The filter string of the export dialog: one entry per format savePlot()
/// knows, vector first.
QString exportFilter();

} // namespace qftbx

#endif // QFTBX_GUI_PLOT_EXPORT_H
