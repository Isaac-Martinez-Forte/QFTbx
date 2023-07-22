#ifndef DATOSBOUND_H
#define DATOSBOUND_H

#include <QVector>
#include <QPointF>
#include <QMap>
#include <QString>
#include <cinterval.hpp>

class DatosBound
{
public:
    DatosBound(QVector <QMap <QString, QVector <QVector <QPointF> * > *> * > * boundaries,
               QVector <bool> * metadatosabierta, QVector <bool> * metadatosarriba,
               qint32 tamFas, QPointF datosFas, QVector< QVector<QPointF> * > * boundaries_reun,
               QVector< QVector< QVector<QPointF> * > * > * hash_inter_boundaries, qint32 tamMag, QPointF DatosMag);

    ~DatosBound();

    QVector <QMap <QString, QVector <QVector <QPointF> * > *> * > * getBoundaries ();
    qint32 getTamFas ();
    qint32 getTamMag ();
    QPointF getDatosFas ();
    QPointF getDatosMag ();
    QVector<QVector<QPointF> * > *getBoundariesReun();
    QVector< QVector< QVector<QPointF> * > * > *  getBoundariesReunHash();
    QVector <bool> * getMetaDatosAbierta();
    QVector <bool> * getMetaDatosArriba ();
    QVector<QPointF> * getDatosFasBound ();
    void setDatosFasBound (QVector<QPointF> * v);
    QVector<QPointF> * getDatosMagBound();
    void setDatosMagBound (QVector<QPointF> * v);
    QVector<QPointF> * getDatosFasBoundLin ();
    void setDatosFasBoundLin (QVector<QPointF> * v);
    QVector<QPointF> * getDatosMagBoundLin();
    void setDatosMagBoundLin (QVector<QPointF> * v);

    cxsc::cinterval getBox();
    void setBox (cxsc::cinterval a);

private:
    QVector <QMap <QString, QVector <QVector <QPointF> * > *> * > * boundaries;
    QVector <bool> * metadatosabierta;
    QVector <bool> * metadatosarriba;
    qint32 tamFas;
    QPointF datosFas;
    qint32 tamMag;
    QPointF DatosMag;
    QVector< QVector<QPointF> * > * boundaries_reun;
    QVector< QVector< QVector<QPointF> * > * > * hash_inter_boundaries;
    QVector<QPointF> * datosFasBound = NULL;
    QVector<QPointF> * datosMagBound = NULL;

    QVector<QPointF> * datosFasBoundLin = NULL;
    QVector<QPointF> * datosMagBoundLin = NULL;

    cxsc::cinterval box;

};

#endif // DATOSBOUND_H
