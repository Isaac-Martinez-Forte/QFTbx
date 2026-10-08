/**
 * @file
 * @brief Writing a plot to a PNG, PDF or SVG file.
 *
 * PDF and SVG carry the curves as vectors, so a journal can set them at any
 * size; PNG is for a slide or a message. JPEG and BMP are not offered: one
 * degrades the lines it exists to show and the other is large for nothing.
 * A request names the file, the raster size and its scale, and a profile:
 * as seen on screen, or for publishing, which is white ground, black axes,
 * heavier strokes and larger type whatever the interface's theme, since a
 * figure is printed on white. A zero size means the plot's own, the
 * window's at that moment, which is nobody's choice; the scale multiplies
 * the raster size without changing the layout. savePlot() returns whether
 * the file was written. exportPlot() is the dialog front end: it asks for
 * the file and the size and reports a failure through the GUI's error
 * reporter. exportFilter() lists the formats savePlot() knows, vector first.
 */

#ifndef QFTBX_GUI_PLOT_EXPORT_H
#define QFTBX_GUI_PLOT_EXPORT_H

#include <QSize>
#include <QString>

class QCustomPlot;
class QWidget;

namespace qftbx {

enum class ExportProfile {
    AsSeen,
    ForPublishing
};

struct ExportRequest
{
    QString fileName;
    QSize size;
    double scale = 1.0;
    ExportProfile profile = ExportProfile::AsSeen;
};

bool savePlot(QCustomPlot & plot, const ExportRequest & request);

void exportPlot(QWidget * parent, QCustomPlot & plot, const QString & title);

QString exportFilter();

}

#endif
