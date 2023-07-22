#include "contour.h"

#include <iostream>
#include <QFile>
#include <QFileDialog>
#include <QDebug>
#include <QTextStream>
#include <iomanip>

using namespace std;

Contour::Contour(){

}

void Contour::setDatos(tools::dBND *altura, QVector<QVector<qreal> *> *sabana, qreal omega){
    this->altura = altura;
    this->sabana = sabana;
    this->omega = omega;


   /*for (qint32 i = 0; i < sabana->size(); i++){
        for (qint32 j = 0; j < sabana->at(0)->size(); j++){
            cout << std::setprecision(6) << fixed;
            cout << sabana->at(i)->at(j) << " ";
        }
        cout << setprecision(6) << fixed;
        cout << endl;
    }*/
}



/*void Contour::setDatos (bool ** sabana){
    this->sabana_bool = sabana;
}*/


QVector <QVector <QPointF> *> * Contour::getContour(qreal nPuntosFas, qreal tamFas, qreal nPuntosMag, qreal tamMag, qreal moverMag)
{
    qint32 ancho = sabana->size();
    qint32 alto = sabana->at(0)->size();
    qreal alt = getAltura();

    QVector <bool> mascara ((ancho + 1) * (alto + 1), false);

    QVector <QVector <QPointF> *> * resultado = new QVector <QVector <QPointF> *> ();

    // We look for pixels contained in a connected set (gray-level value >= threshold) in the image
    for (qint32 fila = 0; fila < alto; fila++)
        for (qint32 columna = 0; columna < ancho; columna++)
        {
            // If we find any pixel like that, we can be sure (because the search we did) it belongs to it's border.
            if (sabana->at(columna)->at(fila) >= alt)
            {
                // if pixel is not marked, we get it's Contour
                if (!mascara.at(columna * alto + fila))/////////////////////////////////////////////////////////////////////////
                {
                    QVector <QPointF> * lista_aux = new QVector <QPointF> ();
                    QVector <QPointF> * lista = getContourFromBorderPoint(sabana, columna, fila, alt, lista_aux,
                                                                          nPuntosFas, tamFas, nPuntosMag, tamMag, moverMag);

                    lista_aux->prepend(QPointF(lista_aux->first().x()+1, lista_aux->first().y()));
                    lista_aux->append(QPointF(lista_aux->last().x()-1, lista_aux->last().y()));

                    resultado->append(lista_aux);



                    QVectorIterator<QPointF> iterator(*lista);
                    for (QPointF previous = iterator.next(), actual; iterator.hasNext(); previous = actual)
                    {
                        actual = iterator.next();
                        foreach (QPointF point, line(actual.x(), actual.y(), previous.x(), previous.y()))
                            mascara.replace (point.x() * alto + point.y(), true);
                    }
                    lista->clear();
                }

                // We ensure next pixel we process will not belong to a connected set.
                while (columna+2 < ancho)
                {
                    if (sabana->at(columna+1)->at(fila) < alt)
                        break;
                    columna++;
                }

                // This is for the case in which we find an internal Contour, that has not been processed and marked.
                if (!mascara.at(columna * alto + fila))
                {
                    QVector <QPointF> * lista_aux = new QVector <QPointF> ();
                    QVector <QPointF> * lista = getContourFromBorderPoint(sabana, columna, fila, alt, lista_aux,
                                                                          nPuntosFas, tamFas, nPuntosMag, tamMag, moverMag);

                    lista_aux->prepend(QPointF(lista_aux->first().x()+1, lista_aux->first().y()));
                    lista_aux->append(QPointF(lista_aux->last().x()-1, lista_aux->last().y()));

                    resultado->append(lista_aux);


                    QVectorIterator<QPointF> iterator(*lista);
                    for (QPointF previous = iterator.next(), actual; iterator.hasNext(); previous = actual)
                    {
                        actual = iterator.next();
                        foreach (QPointF point, line(actual.x(), actual.y(), previous.x(), previous.y()))
                            mascara.replace(point.x() * alto + point.y(), true);
                    }
                    lista->clear();
                }
            }

        }

    return resultado;
}

QVector<QPointF> * Contour::getContourFromBorderPoint(QVector <QVector <qreal> *> * sabana, qint32 posx, qint32 posy, qreal altura,
                                                     QVector <QPointF> * lista_aux, qreal nPuntosFas, qreal tamFas, qreal nPuntosMag, qreal tamMag, qreal moverMag)
{
    QVector <QPointF> * lista = new QVector <QPointF> ();

    qint32 ancho = sabana->size();
    qint32 alto = sabana->at(0)->size();


     lista->append(QPointF(posx, posy));
    if (posx > 0 && posx < (ancho-1) && posy > 0 && posy < (alto-1)){
        lista_aux->append(QPointF(posy * tamFas / nPuntosFas - tamFas, posx * tamMag / nPuntosMag - moverMag));
    }

    // We check this is not an interior pixel, neither a solitary one.
    // Also we look for a neighbour pixel not belonging to any connected set.
    qint32 searchDir = 128, numOuterPixels = 0;
    for (qint32 i = 0; i<8; i++)
    {
        qint32 x =  posx + coorX8Connect[i], y =  posy + coorY8Connect[i];

        if ((x < 0) || (x >= ancho) || (y < 0) || (y >= alto))
        {
            numOuterPixels++;
            searchDir = i;
        }
        else if (sabana->at(x)->at(y) < altura)
        {
            numOuterPixels++;
            searchDir = i;
        }
    }

    // Case we have a solitary pixel, we return that pixel.
    if (numOuterPixels == 8)
        return lista;

    // We insert each point of the border Contour, inserting it to the point list.
    qint32 sumSearchDir = 0, actualPointX = posx, actualPointY = posy;

    while (true)
    {
        // We search for the next point belonging to the Contour.
        qint32 d;
        qint32    nextPointX, nextPointY;

       /* bool rodeado = false;
        for (qint32 i = 0; i<8; i++)
        {
            qint32 x =  posx + coorX8Connect[i], y =  posy + coorY8Connect[i];

            if (sabana->at(x)->at(y) >= altura){
                rodeado = true;
            }
        }

        if (!rodeado){
            return lista;
        }*/

        for (d = 0; d < 8; d++)
        {
            searchDir = (searchDir+1)%8;
            nextPointX = actualPointX + coorX8Connect[searchDir];
            nextPointY = actualPointY + coorY8Connect[searchDir];
            if ((nextPointX >= 0) && (nextPointX < ancho) && (nextPointY >= 0) && (nextPointY < alto))
                if (sabana->at(nextPointX)->at(nextPointY) >= altura)
                    break;
        }

        sumSearchDir += d - 3;

        actualPointX = nextPointX;
        actualPointY = nextPointY;

        if ( sabana->at(actualPointX)->at(actualPointY) < altura )
            break;

        if ( posx == actualPointX && posy == actualPointY){
            break;
        }

        lista->append(QPointF(actualPointX, actualPointY));
        if (actualPointX > 0 && actualPointX < (ancho-1) && actualPointY > 0 && actualPointY < (alto-1)){
            lista_aux->append(QPointF(actualPointY * tamFas / nPuntosFas - tamFas,
                                      actualPointX * tamMag / nPuntosMag - moverMag));
        }

        searchDir = searchDir + 4;
    }

    return lista;
}

/*QVector <QVector <QPointF> *> * Contour::getContour(qint32 alto, qint32 ancho)
{

    QVector <bool> mascara ((ancho + 1) * (alto + 1), false);

    QVector <QVector <QPointF> *> * resultado = new QVector <QVector <QPointF> *> ();

    // We look for pixels contained in a connected set (gray-level value >= threshold) in the image
    for (qint32 fila = 0; fila < alto; fila++)
        for (qint32 columna = 0; columna < ancho; columna++)
        {
            // If we find any pixel like that, we can be sure (because the search we did) it belongs to it's border.
            if (sabana_bool[fila][columna] == 0)
            {
                // if pixel is not marked, we get it's Contour
                if (!mascara.at(columna * alto + fila))
                {
                    QVector <QPointF> * lista_aux = new QVector <QPointF> ();
                    QVector <QPointF> * lista = getContourFromBorderPoint(fila, columna, alto, ancho, lista_aux);
                    resultado->append(lista_aux);

                    QVectorIterator<QPointF> iterator(*lista);
                    for (QPointF previous = iterator.next(), actual; iterator.hasNext(); previous = actual)
                    {
                        actual = iterator.next();
                        foreach (QPointF point, line(actual.x(), actual.y(), previous.x(), previous.y()))
                            mascara.replace (point.y() * alto + point.x(), true);
                    }
                    lista->clear();
                }

                // We ensure next pixel we process will not belong to a connected set.
                while (columna+2 < ancho)
                {
                    if (sabana_bool[fila][columna+1] == 1)
                        break;
                    columna++;
                }

                // This is for the case in which we find an internal Contour, that has not been processed and marked.
                if (!mascara.at(columna * alto + fila))
                {
                    QVector <QPointF> * lista_aux = new QVector <QPointF> ();
                    QVector <QPointF> * lista = getContourFromBorderPoint(fila, columna, alto, ancho, lista_aux);
                    resultado->append(lista_aux);

                    QVectorIterator<QPointF> iterator(*lista);
                    for (QPointF previous = iterator.next(), actual; iterator.hasNext(); previous = actual)
                    {
                        actual = iterator.next();
                        foreach (QPointF point, line(actual.x(), actual.y(), previous.x(), previous.y()))
                            mascara.replace(point.y() * alto + point.x(), true);
                    }
                    lista->clear();
                }
            }
        }

    return resultado;
}



QVector<QPointF> * Contour::getContourFromBorderPoint(qint32 posx, qint32 posy, qreal alto, qreal ancho,
                                                     QVector <QPointF> * lista_aux)
{

    QVector <QPointF> * lista = new QVector <QPointF> ();

    lista->append(QPointF(posx, posy));
    lista_aux->append(QPointF(posx - alto, posy - ancho / 2));

    // We check this is not an interior pixel, neither a solitary one.
    // Also we look for a neighbour pixel not belonging to any connected set.
    qint32 searchDir = 128, numOuterPixels = 0;
    for (qint32 i = 0; i<8; i++)
    {
        qint32 x =  posx + coorX8Connect[i], y =  posy + coorY8Connect[i];
        if ((x < 0) || (x >= alto) || (y < 0) || (y >= ancho))
        {
            numOuterPixels++;
            searchDir = i;
        }
        else if (sabana_bool[x][y] == 1)
        {
            numOuterPixels++;
            searchDir = i;
        }
    }

    // Case we have a solitary pixel, we return that pixel.
    if (numOuterPixels == 8)
        return lista;

    // We insert each point of the border Contour, inserting it to the point list.
    qint32 sumSearchDir = 0, actualPointX = posx, actualPointY = posy;
    while (true)
    {
        // We search for the next point belonging to the Contour.
        qint32 d;
        qint32    nextPointX, nextPointY;
        for (d = 0; d < 8; d++)
        {
            searchDir = (searchDir+1)%8;
            nextPointX = actualPointX + coorX8Connect[searchDir];
            nextPointY = actualPointY + coorY8Connect[searchDir];
            if ((nextPointX >= 0) && (nextPointX < alto) && (nextPointY >= 0) && (nextPointY < ancho))
                if (sabana_bool[nextPointX][nextPointY] == 0)
                    break;
        }

        sumSearchDir += d - 3;

        actualPointX = nextPointX;
        actualPointY = nextPointY;

        if (sabana_bool[actualPointX][actualPointY] == 1)
            break;

        if ( posx == actualPointX && posy == actualPointY){
            break;
        }

        lista->append(QPointF(actualPointX, actualPointY));
        lista_aux->append(QPointF(actualPointX - alto, actualPointY - ancho / 2));
        searchDir = searchDir + 4;
    }

    return lista;
}*/

qreal Contour::getAltura(){
    return altura->getAltura(omega);
}

//http://stackoverflow.com/questions/1903954/is-there-a-standard-sign-function-signum-sgn-in-c-c
template <typename T> qint32 sgn(T val) {
    return (T(0) < val) - (val < T(0));
}

QVector <QPointF> Contour::line(qint32 x1, qint32 y1, qint32 x2, qint32 y2)
{
    QVector <QPointF> line;

    line.append(QPointF( x1, y1 ));

    if (x1 == x2 && y1 == y2)
        return line;

    qint32 i,dx,dy,sdx,sdy,dxabs,dyabs,x,y,px,py;

    dx=x2-x1;      // the horizontal distance of the line
    dy=y2-y1;      // the vertical distance of the line
    dxabs=abs(dx);
    dyabs=abs(dy);
    sdx=sgn(dx);
    sdy=sgn(dy);
    x=dyabs>>1;
    y=dxabs>>1;
    px=x1;
    py=y1;

    if (dxabs>=dyabs) // the line is more horizontal than vertical
        for(i=0;i<dxabs;i++)
        {
            y+=dyabs;
            if (y>=dxabs)
            {
                y-=dxabs;
                py+=sdy;
            }
            px+=sdx;
            line.append(QPointF( px, py ));
        }
    else    // the line is more vertical than horizontal
        for(i=0;i<dyabs;i++)
        {
            x+=dxabs;
            if (x>=dyabs)
            {
                x-=dyabs;
                px+=sdx;
            }
            py+=sdy;
            line.append(QPointF( px, py ));
        }

    return line;
}
