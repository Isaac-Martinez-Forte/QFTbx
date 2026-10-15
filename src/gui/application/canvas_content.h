/**
 * @file
 * @brief The content of the canvas: the phase cards, and the banner of the
 * program while there is none.
 *
 * With no card to show, when the program starts or a new session begins,
 * the canvas draws the banner the README shows, docs/qftbx_banner.svg,
 * centred, at its own size or smaller when the window is, with the rounded
 * corners of the file, which Qt's renderer does not clip by itself. It
 * draws again whenever a card comes, goes or is hidden.
 */

#ifndef QFTBX_CANVAS_CONTENT_H
#define QFTBX_CANVAS_CONTENT_H

#include <QSvgRenderer>
#include <QWidget>

namespace qftbx {

class CanvasContent : public QWidget
{
    Q_OBJECT

public:
    explicit CanvasContent(QWidget * parent = nullptr);

    bool showsBanner() const;

protected:
    bool event(QEvent * event) override;

    void paintEvent(QPaintEvent * event) override;

private:
    QSvgRenderer m_banner;
};

}

#endif
