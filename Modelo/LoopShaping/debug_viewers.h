#ifndef QFTBX_DEBUG_VIEWERS_H
#define QFTBX_DEBUG_VIEWERS_H

//Debug-only viewers of the loop-shaping algorithms, used exclusively
//inside the VER_DIAGRAMAS / COMPARACION_CAJAS / VER_ANTES blocks of the
//MC-prev and MC algorithms. This is the ONLY backend header that may pull
//Qt Widgets: every regular build stays GUI-free, which also lets the
//interval-computing translation units carry -frounding-math (Qt's
//constexpr float arithmetic does not compile under it).
//TODO(8b.5/8b.6): replace with a GUI-side observer so the backend never
//links widgets, debug builds included.

#include <QPointF>
#include <QVector>

#include "GUI/boundary_union_viewer.h"
#include "src/core/boundaries/boundary_data.h"

namespace FC {

inline BoundaryUnionViewer * showDiagram(QVector <QVector<QPointF> * > *vector, QVector <qreal> * omega, BoundaryData * boundaries) {
    BoundaryUnionViewer * view = new BoundaryUnionViewer();

    view->setDatos(boundaries->unionBoundaries(), omega);

    view->showDiagram();

    qint32 contador = 0;

    foreach(QVector <QPointF> * vec, *vector) {
        view->drawBox(vec->at(0), vec->at(1), vec->at(2), vec->at(3), contador);
        contador++;
    }

    //view->exec();

        //delete view;

    return view;
}

inline void mostrar_diagrama2 (QVector <QVector<QPointF> * > *vector, QVector <qreal> * omega, BoundaryData * boundaries, BoundaryUnionViewer * view) {
    //BoundaryUnionViewer * view = new BoundaryUnionViewer();

    /*view->setDatos(boundaries->unionBoundaries(), omega);

        view->showDiagram();*/

    qint32 contador = 0;

    foreach(QVector <QPointF> * vec, *vector) {
        view->drawBox2(vec->at(0), vec->at(1), vec->at(2), vec->at(3), contador);
        contador++;
    }

    view->exec();

    delete view;
}

inline void mostrar_diagramaBox(QVector<QPointF> * caja, QVector <qreal> * omega, BoundaryData * boundaries) {
    BoundaryUnionViewer * view = new BoundaryUnionViewer();

    view->setDatos(boundaries->unionBoundaries(), omega);

    view->showDiagram();

    view->drawBox(caja->at(0), caja->at(1), caja->at(2), caja->at(3), 0);

    view->exec();

    delete view;
    caja->clear();
}


} // namespace FC

#endif // QFTBX_DEBUG_VIEWERS_H
