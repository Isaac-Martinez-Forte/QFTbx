#include "algoritmointerseccionlineal1D.h"
#include <memory>
#define GRADOS_FASE 360

using namespace std;


AlgoritmoInterseccionLineal1D::AlgoritmoInterseccionLineal1D()
{

}

AlgoritmoInterseccionLineal1D::~AlgoritmoInterseccionLineal1D()
{

}

qint32 AlgoritmoInterseccionLineal1D::funcionHash(qreal x, qreal totalFase)
{
    qreal res = totalFase-(abs(x)*(totalFase/(qreal)GRADOS_FASE));
    //cout << res << endl;
    if(res<0) res=0;
    return (qint32) res;
}

void AlgoritmoInterseccionLineal1D::insercionManual(QVector<QVector<QPointF> *> * fronteraHash, QVector<QPointF>::iterator it, qint32 indice, QPointF punto, qreal totalFase)
{
    bool repetido = false;
    for(qint32 i=0;i<fronteraHash->at(funcionHash(punto.x(), totalFase))->size();i++)
    {
        QPointF puntoHash = fronteraHash->at(funcionHash(punto.x(), totalFase))->at(i);
        if(punto.y()==puntoHash.y()) repetido = true;
    }
    if(!repetido) fronteraHash->at(funcionHash(punto.x(), totalFase))->insert(it+indice,punto);
}

QVector< QVector< QVector<QPointF> * > * > * AlgoritmoInterseccionLineal1D::crearFronterasHash(QVector< QVector<QPointF> * > * &fronterasElegidas, qreal totalFase, bool abierta, bool arriba)
{
    QVector< QVector< QVector<QPointF> * > * > * fronterasHash = new QVector< QVector< QVector<QPointF> * > * >();
    for(qint32 i=0;i<NUM_FRONTERAS_HASH;i++)
    {
        fronterasHash->append(new QVector< QVector<QPointF> * >());
        for(qint32 j=0;j<=(qint32)totalFase;j++)
        {
            fronterasHash->at(i)->append(new QVector<QPointF>());
        }
        for(qint32 j=0;j<(qint32)fronterasElegidas->at(i)->size();j++)
        {
            QPointF punto = fronterasElegidas->at(i)->at(j);
            qint32 indice = 0;
            QVector<QPointF>::iterator it = fronterasHash->at(i)->at(funcionHash(punto.x(), totalFase))->begin();
            for(qint32 k=0;k<(qint32)fronterasHash->at(i)->at(funcionHash(punto.x(), totalFase))->size();k++)
            {
                if(arriba)
                {
                    if(punto.y()<fronterasHash->at(i)->at(funcionHash(punto.x(), totalFase))->at(k).y()) indice++;
                }
                else if(punto.y()>fronterasHash->at(i)->at(funcionHash(punto.x(), totalFase))->at(k).y()) indice++;
            }
            if(!abierta||(abierta&&(fronterasHash->at(i)->at(funcionHash(punto.x(), totalFase))->size()==0))) insercionManual(fronterasHash->at(i), it, indice, punto, totalFase);
        }

    }
    return fronterasHash;
}

QVector<QPointF> * AlgoritmoInterseccionLineal1D::dibujarPrimeraCapa(QVector< QVector<QPointF> * > * &fronterasElegidas, QVector< QVector< QVector<QPointF> * > * > * &fronterasHash, qreal totalFase, bool abierta1, bool abierta2)
{
    QVector<QPointF> * capa1 = new QVector<QPointF>();
    if(abierta1)
    {
        for(qint32 i=0;i<(qint32)fronterasElegidas->at(1)->size();i++)
        {
            qint32 tamCubeta = (qint32) fronterasHash->at(0)->at(funcionHash(fronterasElegidas->at(1)->at(i).x(), totalFase))->size();
            QPointF puntoCurva = fronterasElegidas->at(1)->at(i);
            QVector<QPointF> * puntosHash = fronterasHash->at(0)->at(funcionHash(fronterasElegidas->at(1)->at(i).x(), totalFase));
            if(tamCubeta==0) capa1->push_back(puntoCurva);
            if(tamCubeta==1)
            {
                //cout << fronteras[1][i].y() << " <---> " << fronterasHash->at(0)[funcionHash(fronteras[1][i].x(), totalFase)][0].y() << endl;
                if(puntoCurva.y() >= puntosHash->at(0).y()) capa1->push_back(puntoCurva);
            }
            else if(tamCubeta==2)
            {
                if((puntoCurva.y() >= puntosHash->at(0).y())||
                        (puntoCurva.y() <= puntosHash->at(1).y())) capa1->push_back(puntoCurva);
            }
            else
            {
                for(qint32 j=0;j<(qint32)puntosHash->size();j+=2)
                {
                    if(puntoCurva.y() >= puntosHash->at(0).y()) capa1->push_back(puntoCurva);
                    if(tamCubeta-j>1)
                    {
                        if((puntoCurva.y() <= puntosHash->at(j+1).y())&&
                                (puntoCurva.y() >= puntosHash->at(j+2).y())) capa1->push_back(puntoCurva);
                    }
                    else
                    {
                        if((puntoCurva.y() >= puntosHash->at(j).y())&&
                                (puntoCurva.y() <= puntosHash->at(j-1).y())) capa1->push_back(puntoCurva);
                    }
                }
            }
        }
    }
    else
    {
        for(qint32 i=0;i<(qint32)fronterasElegidas->at(1)->size();i++)
        {
            qint32 tamCubeta = (qint32) fronterasHash->at(0)->at(funcionHash(fronterasElegidas->at(1)->at(i).x(), totalFase))->size();
            QPointF puntoCurva = fronterasElegidas->at(1)->at(i);
            QVector<QPointF> * puntosHash = fronterasHash->at(0)->at(funcionHash(fronterasElegidas->at(1)->at(i).x(), totalFase));
            if(tamCubeta>1&&tamCubeta%2==0)
            {
                bool fuera = true;
                if(abierta2&&puntoCurva.y() <= puntosHash->at(0).y())
                {
                    fuera = false;
                }
                else
                {
                    for(qint32 j=0;j<tamCubeta;j+=2)
                    {
                        //cout << puntosHash[j].y() << " ... " << puntosHash[j+1].y() << endl;
                        if((puntoCurva.y() <= puntosHash->at(j).y())&&
                                (puntoCurva.y() >= puntosHash->at(j+1).y())) fuera = false;
                    }
                }
                if(fuera) capa1->push_back(puntoCurva);
            }
            else if(tamCubeta%2==1&&puntoCurva.y() > puntosHash->at(0).y())
            {
                capa1->push_back(puntoCurva);
            }
            else if(tamCubeta==0) capa1->push_back(puntoCurva);
        }
    }
    return capa1;
}

QVector<QPointF> * AlgoritmoInterseccionLineal1D::dibujarSegundaCapa(QVector< QVector<QPointF> * > * &fronterasElegidas, QVector< QVector< QVector<QPointF> * > * > * &fronterasHash, qreal totalFase, bool abierta1, bool abierta2)
{
    QVector<QPointF> * capa2 = new QVector<QPointF>();
    if(abierta2)
    {
        for(qint32 i=0;i<(qint32)fronterasElegidas->at(0)->size();i++)
        {
            qint32 tamCubeta = (qint32) fronterasHash->at(1)->at(funcionHash(fronterasElegidas->at(0)->at(i).x(), totalFase))->size();
            QPointF puntoCurva = fronterasElegidas->at(0)->at(i);
            QVector<QPointF> * puntosHash = fronterasHash->at(1)->at(funcionHash(fronterasElegidas->at(0)->at(i).x(), totalFase));
            if(tamCubeta==0) capa2->push_back(puntoCurva);
            if(tamCubeta==1)
            {
                if(puntoCurva.y() >= puntosHash->at(0).y()) capa2->push_back(puntoCurva);
            }
            else if(tamCubeta==2)
            {
                if((puntoCurva.y() >= puntosHash->at(0).y())||
                        (puntoCurva.y() <= puntosHash->at(1).y())) capa2->push_back(puntoCurva);
            }
            else
            {
                for(qint32 j=0;j<(qint32)puntosHash->size();j+=2)
                {
                    if(puntoCurva.y() >= puntosHash->at(0).y()) capa2->push_back(puntoCurva);
                    if(tamCubeta-j>1)
                    {
                        if((puntoCurva.y() <= puntosHash->at(j+1).y())&&
                                (puntoCurva.y() >= puntosHash->at(j+2).y())) capa2->push_back(puntoCurva);
                    }
                    else
                    {
                        if((puntoCurva.y() >= puntosHash->at(j).y())&&
                                (puntoCurva.y() <= puntosHash->at(j-1).y())) capa2->push_back(puntoCurva);
                    }
                }
            }
        }
    }
    else
    {
        for(qint32 i=0;i<(qint32)fronterasElegidas->at(0)->size();i++)
        {
            qint32 tamCubeta = fronterasHash->at(1)->at(funcionHash(fronterasElegidas->at(0)->at(i).x(), totalFase))->size();
            QPointF puntoCurva = fronterasElegidas->at(0)->at(i);
            QVector<QPointF> * puntosHash = fronterasHash->at(1)->at(funcionHash(fronterasElegidas->at(0)->at(i).x(), totalFase));
            if(tamCubeta>1&&tamCubeta%2==0)
            {
                bool fuera = true;
                if(abierta1&&puntoCurva.y() <= puntosHash->at(0).y())
                {
                    fuera = false;
                }
                else
                {
                    for(qint32 j=0;j<tamCubeta;j+=2)
                    {
                        if((puntoCurva.y() < puntosHash->at(j).y())&&
                                (puntoCurva.y() > puntosHash->at(j+1).y())) fuera = false;
                    }
                }
                if(fuera) capa2->push_back(puntoCurva);
            }
            else if(tamCubeta%2==1&&puntoCurva.y() > puntosHash->at(0).y())
            {
                capa2->push_back(puntoCurva);
            }
            else if(tamCubeta==0) capa2->push_back(puntoCurva);
        }
    }
    return capa2;
}

int compareQPointF (const void * a, const void * b)
{
    return (*(QPointF*)a).x() <  (*(QPointF*)b).x();

}

inline qint32 AlgoritmoInterseccionLineal1D::funcionHash(qreal x, qreal totalFase, qint32 numeroFases)
{
    double res = (abs(x)*((qreal)numeroFases/totalFase));
    if(res<0) res=0;
    return (qint32) res;
}

QVector< QVector<QPointF> * > * AlgoritmoInterseccionLineal1D::crearInterseccionHash(QVector<QPointF> * &interseccion, qreal totalFase, qint32 npuntos)
{
    QVector< QVector<QPointF> * > * interseccionHash = new QVector< QVector<QPointF> * > ();


    for(qint32 i=0;i<npuntos;i++)
    {
        interseccionHash->append(new QVector<QPointF> ());
    }

    QPointF punto;

    for (qint32 i=1;i < interseccion->size(); i++) {
        punto = interseccion->at(i);
        interseccionHash->at(funcionHash(punto.x(), totalFase, npuntos))->append(punto);
    }

    return interseccionHash;
}

QVector<QPointF> * AlgoritmoInterseccionLineal1D::unirCapas(QVector<QPointF> * &capa1, QVector<QPointF> * &capa2)
{
    QVector<QPointF> * unionCapas = new QVector<QPointF>(*capa1+*capa2);
    return unionCapas;
}

void AlgoritmoInterseccionLineal1D::ejecutarAlgoritmo(std::shared_ptr<DatosBound> boundaries,
                                                      QVector <QMap <QString, QVector <QPoint> * > * > * vectorMetadatos)
{
    interseccionesBoundaries = new QVector< QVector < QVector<QPointF> * > * > ();
    interseccionesVectores = new QVector< QVector<QPointF> * > ();
    metadatosAbierta = new QVector<bool>();
    metadatosArriba = new QVector<bool>();


    //Cada Frecuencia de diseño tiene un Mapa, cada mapa tiene unas Especificaciones, y cada Especificación tiene una Curva Paramétrica de Puntos(Fase, Magnitud) que empieza desde -360º a 0º
    //Ejemplo: Frecuencia: 0.1; Especificaciones: Estabilidad Robusta, Ruido Entrada, Ruido Salida; Curva Paramétrica.
    //El tipo que devuelve para fase y magnitud es de tipo entero
    QVector <QMap <QString, QVector <QVector <QPointF> * > *> * > * vectorBoundaries = boundaries->getBoundaries();

    qreal totalFase = -boundaries->getDatosFas().x();
    qint32 nPuntosFas = boundaries->getTamFas();

    for(qint32 i=0;i<vectorBoundaries->size();i++)
    {

        QMap <QString, QVector <QVector <QPointF> * > *> * mapa = vectorBoundaries->at(i);

        QMap <QString, QVector <QPoint> * > * mapaMetadatos = vectorMetadatos->at(i);

        QVector<QPointF> * interseccion = new QVector <QPointF> ();
        bool abierta1 = false, arriba = false, abierta2 = false;
        metadatosAbierta->append(false);
        metadatosArriba->append(false);

        bool primeraEspecificacion = true;
        foreach (QVector <QVector <QPointF> * > * vectorEspecificaciones, *mapa)
        {
            QMapIterator <QString, QVector <QPoint> * > iterador(*mapaMetadatos);
            if(iterador.hasNext())
            {
                iterador.next();
                QVector <QPoint> * fronterasMetadatos = iterador.value();
                //Si en el eje X de cualquiera de los puntos devuelve 0 la parte permitida va por Arriba y 1 si va por Abajo
                arriba = !fronterasMetadatos->at(0).x(); //Nos vale el primer punto para saber si la zona permitida es por Arriba o por Abajo
            }
            if(primeraEspecificacion)
            {
                //La primera especificación se une en intersección
                for(qint32 j=0;j<(qint32)vectorEspecificaciones->size();j++)
                {
                    *interseccion += *vectorEspecificaciones->at(j);

                }
                primeraEspecificacion = false;
                continue;
            }

            //Las siguientes especificaciones se une en frontera actual
            QVector <QPointF> * fronteraActual = new QVector <QPointF>();
            for(qint32 j=0;j<(qint32)vectorEspecificaciones->size();j++)
            {
                *fronteraActual += *vectorEspecificaciones->at(j);
            }


            if(interseccion->size()>=totalFase) abierta1 = true;
            if(fronteraActual->size()>=totalFase) abierta2 = true;

            QVector< QVector<QPointF> * > * fronterasElegidas = new QVector< QVector<QPointF> * >();
            fronterasElegidas->append(interseccion);
            fronterasElegidas->append(fronteraActual);
            QVector< QVector< QVector<QPointF> * > * > * fronterasHash = crearFronterasHash(fronterasElegidas, totalFase, abierta1||abierta2, arriba);

            QVector<QPointF> * capa1 = dibujarPrimeraCapa(fronterasElegidas,fronterasHash,totalFase,abierta1,abierta2);
            QVector<QPointF> * capa2 = dibujarSegundaCapa(fronterasElegidas,fronterasHash,totalFase,abierta1,abierta2);
            interseccion->clear();
            interseccion = unirCapas(capa1, capa2);
            //cout << "--->" << (abierta1||abierta2) << endl;
            if((abierta1||abierta2)&&!metadatosAbierta->at(i))
            {
                metadatosAbierta->replace(i, abierta1||abierta2);
                metadatosArriba->replace(i, arriba);
            }
            fronterasElegidas->clear();
        }

        //cout << "Arriba: " << metadatosArriba->at(i) << endl;

        interseccionesVectores->append(interseccion);



        QVector< QVector<QPointF> * > * interseccionHash = crearInterseccionHash(interseccion, totalFase, nPuntosFas);

        interseccionesBoundaries->append(interseccionHash);
    }





    QVector<QVector<QPointF> *> * dos = new QVector<QVector<QPointF> *> ();

    qint32 c = 0;
    foreach (QVector<QPointF> * b, *interseccionesVectores) {
        if (metadatosAbierta->at(c)){
            dos->append(OrdenacionPuntos(b));
        } else {
            dos->append(b);
        }
        c++;
    }

    interseccionesVectores->clear();
    interseccionesVectores = dos;
}

QVector<QVector<QVector<QPointF> *> *> *AlgoritmoInterseccionLineal1D::getInterseccionesBoundaries()
{
    return interseccionesBoundaries;
}

QVector<QVector<QPointF> *> * AlgoritmoInterseccionLineal1D::getInterseccionesVectores()
{
    return interseccionesVectores;
}

QVector <bool> * AlgoritmoInterseccionLineal1D::getMetadatosAbierta()
{
    return metadatosAbierta;
}

QVector <bool> * AlgoritmoInterseccionLineal1D::getMetadatosArriba()
{
    return metadatosArriba;
}



void AlgoritmoInterseccionLineal1D::SelectionSort(QVector<QPointF> * array) {

    qint32 n = array->size();
    QPointF tmp;
    qint32 x, y, min;

    for(x = 0; x < n; x++) {
        min = x;
        for(y = x + 1; y < n; y++) {
            if(array->at(min).x() > array->at(y).x()) {
                min = y;
            }
        }
        tmp = array->at(min);
        array->replace(min, array->at(x));
        array->replace(x, tmp);
    }
}

QVector<QPointF> * AlgoritmoInterseccionLineal1D::OrdenacionPuntos(QVector<QPointF> * array) {

    QVector<QPointF> * v = new QVector<QPointF> ();
    v->reserve(array->size());

    QPointF tmp = QPointF(10000, 1);

    foreach (QPointF p, *array) {
        if (p.x() < tmp.x()){
            tmp = QPointF (p);
        }
    }

    v->append(tmp);

    array->removeOne(tmp);
    qint32 lon = array->size();

    qreal dis, dis2;
    for (qint32 i = 0; i < lon; i++){
        QPointF uno(tmp);
        dis = 10000;
        for (qint32 j = 0; j < array->size(); j++){
            QPointF dos = array->at(j);
            dis2 = sqrt(pow(uno.x() - dos.x(), 2) + pow(uno.y() - dos.y(), 2));
            if (dis2 < dis){
                dis = dis2;
                tmp = QPointF (dos);
            }
        }

        v->append(tmp);
        array->removeOne(tmp);
    }
    array->clear();
    return v;
}

