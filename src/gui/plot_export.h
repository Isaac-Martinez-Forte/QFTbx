#ifndef QFTBX_GUI_PLOT_EXPORT_H
#define QFTBX_GUI_PLOT_EXPORT_H

#include <QString>

class QCustomPlot;
class QWidget;

namespace tools {

/**
 * @brief Asks for a file name and writes the plot to it as PNG, PDF, JPG or
 * BMP, reporting through errorMessage() when it cannot.
 *
 * Six viewers carried this file dialog and format switch verbatim.
 */
void exportPlot(QWidget * parent, QCustomPlot & plot, const QString & title);

/// Writes one plot to fileName in the format the dialog's filter picked;
/// false when the format is unknown or the write failed.
bool savePlotAs(QCustomPlot & plot, const QString & fileName, const QString & extension);

/// The filter string of the export dialog: one entry per format savePlotAs()
/// knows.
QString exportFilter();

} // namespace tools

#endif // QFTBX_GUI_PLOT_EXPORT_H
