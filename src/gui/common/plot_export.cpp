#include "src/gui/common/plot_export.h"

#include <QFileDialog>

#include "qcustomplot.h"
#include "src/gui/application/error_message.h"

namespace qftbx {

QString exportFilter()
{
    return QObject::tr(".png (*.png);;.pdf(*.pdf);; .jpg(*.jpg);; .bmp(*.bmp)");
}

bool savePlotAs(QCustomPlot & plot, const QString & fileName, const QString & extension)
{
    if (extension.contains(".pdf", Qt::CaseInsensitive)) {
        return plot.savePdf(fileName, true);
    }
    if (extension.contains(".png", Qt::CaseInsensitive)) {
        return plot.savePng(fileName);
    }
    if (extension.contains(".jpg", Qt::CaseInsensitive)) {
        return plot.saveJpg(fileName);
    }
    if (extension.contains(".bmp", Qt::CaseInsensitive)) {
        return plot.saveBmp(fileName);
    }

    return false;
}

void exportPlot(QWidget * parent, QCustomPlot & plot, const QString & title)
{
    QString extension;
    const QString fileName = QFileDialog::getSaveFileName(parent, QObject::tr("Save file"), "",
                                                          exportFilter(), &extension);
    if (fileName.isEmpty()) {
        return;
    }

    if (!savePlotAs(plot, fileName, extension)) {
        errorMessage(QObject::tr("The image could not be saved"), title);
    }
}

} // namespace qftbx
