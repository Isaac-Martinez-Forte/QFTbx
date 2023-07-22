#include "contour2.h"

using namespace std;
using namespace tools;

Contour2::Contour2(){

}

void Contour2::setDatos(tools::dBND *altura, QVector<QVector<qreal> *> *sabana, qreal omega, dBND *altura2){

    this->altura = altura;
    this->sabana = sabana;
    this->omega = omega;
    this->altura2 = altura2;


    /*if (altura->nombre == "seguimiento"){
        for (qint32 i = 0; i < sabana->size(); i++){
            for (qint32 j = 0; j < sabana->at(0)->size(); j++){
                cout << sabana->at(i)->at(j) << " ";
            }
            cout << endl;
        }

        cout << "------------------------------------------------------------------------------------------------------------" << endl;
    }

    cout << "------------------------------------------------------------------------------------------------------------" << endl;*/
}

void Contour2::setDatos(dBND *altura, float *sabana, qreal omega, dBND *altura2){
    this->altura = altura;
    this->altura2 = altura2;
    this->sabanaCuda = sabana;
    this->omega = omega;
}


QVector <QVector <QPointF> *> * Contour2::getContour(qreal nPuntosFas, qreal nPuntosMag, qreal moverMag)
{

    qint32 ancho = sabana->at(0)->size();
    qint32 alto = sabana->size();
    qreal alt = getAltura();

    qint32 tamFas = ancho - 1;
    qint32 tamMag = alto - 1;

    mascara = new QVector <bool> ((ancho + 1) * (alto + 1), false);

    QVector <QVector <QPointF> *> * resultado = new QVector <QVector <QPointF> *> ();

    // We look for pixels contained in a connected set (gray-level value >= threshold) in the image
    for (qint32 columna = 1; columna < ancho-1; columna++){
        for (qint32 fila = 1; fila < alto-1; fila++)
        {

            if ((sabana->at(fila)->at(columna) >= alt) && (!mascara->at(fila * ancho + columna))){

                QVector <QPointF> * lista = new QVector <QPointF> ();


                qint32 nuevax = columna;
                qint32 nuevay = fila;

                qint32 corte = 0;

                qint32 x,y;

                while (true){

                    corte = 0;

                    for (qint32 i = 15; i>7; i--)
                    {
                        x =  nuevax + coorX8Connect[i % 8];
                        y =  nuevay + coorY8Connect[i % 8];

                        if ((x > 0) && (x < ancho-1) && (y > 0) && (y < alto-1) && (sabana->at(y)->at(x) < alt))
                        {
                            x =  nuevax + coorX8Connect[(i - 1) % 8];
                            y =  nuevay + coorY8Connect[(i - 1) % 8];

                            if ((sabana->at(y)->at(x) >= alt) && (!mascara->at(y * ancho + x))){


                                lista->append(QPointF(((nuevax * nPuntosFas) / tamFas) - nPuntosFas, ((nuevay *
                                                      nPuntosMag) / tamMag) - moverMag));

                                mascara->replace(nuevay * ancho + nuevax, true);
                                nuevax = x;
                                nuevay = y;
                                corte++;
                                break;
                            }
                        }
                    }


                    if (corte == 0){

                        lista->append(QPointF(((nuevax * nPuntosFas) / tamFas) - nPuntosFas, ((nuevay *
                                              nPuntosMag) / tamMag) - moverMag));

                        break;
                    }
                }

                if (lista->size() <= 1){
                    lista->clear();
                } else {

                    lista->prepend(QPointF(lista->first().x()-(nPuntosFas / tamFas), lista->first().y()));
                    lista->append(QPointF(lista->last().x()+(nPuntosFas / tamFas), lista->last().y()));

                    resultado->append(lista);
                }
            }
        }
    }

    mascara->clear();

    return resultado;
}


QVector <QVector <QPointF> *> * Contour2::getContour(qreal nPuntosFas, qreal tamFas, qreal nPuntosMag,
                                                     qreal tamMag, qreal moverMag){

    qint32 ancho = tamFas;
    qint32 alto = tamMag;
    qreal alt = getAltura();
    tamFas--;
    tamMag--;


    mascara = new QVector <bool> ((ancho + 1) * (alto + 1), false);

    QVector <QVector <QPointF> *> * resultado = new QVector <QVector <QPointF> *> ();

    // We look for pixels contained in a connected set (gray-level value >= threshold) in the image
    for (qint32 columna = 1; columna < ancho-1; columna++){
        for (qint32 fila = 1; fila < alto-1; fila++)
        {

            if ((sabanaCuda[columna * alto + fila] >= alt) && (!mascara->at(fila * ancho + columna))){

                QVector <QPointF> * lista = new QVector <QPointF> ();


                qint32 nuevax = columna;
                qint32 nuevay = fila;

                qint32 corte = 0;

                qint32 x,y;

                while (true){

                    corte = 0;

                    for (qint32 i = 15; i>7; i--)
                    {
                        x =  nuevax + coorX8Connect[i % 8];
                        y =  nuevay + coorY8Connect[i % 8];

                        if ((x > 0) && (x < ancho-1) && (y > 0) && (y < alto-1) &&
                                (sabanaCuda[x * alto + y] <= alt))
                        {
                            x =  nuevax + coorX8Connect[(i - 1) % 8];
                            y =  nuevay + coorY8Connect[(i - 1) % 8];

                            if ((sabanaCuda[x * alto + y] > alt) && (!mascara->at(y * ancho + x))){
                                lista->append(QPointF(((nuevax * nPuntosFas) / tamFas) - nPuntosFas, ((nuevay *
                                                      nPuntosMag) / tamMag) - moverMag));
                                mascara->replace(nuevay * ancho + nuevax, true);
                                nuevax = x;
                                nuevay = y;
                                corte++;
                                break;
                            }
                        }
                    }


                    if (corte == 0){
                        lista->append(QPointF(((nuevax * nPuntosFas) / tamFas) - nPuntosFas, ((nuevay *
                                              nPuntosMag) / tamMag) - moverMag));
                        break;
                    }
                }

                if (lista->size() <= 1){
                    lista->clear();
                }else if (lista->size() == 2){
                    lista->clear();

                    QVector <QPointF> * lista = new QVector <QPointF> ();


                    qint32 nuevax = columna;
                    qint32 nuevay = fila;

                    qint32 corte = 0;

                    qint32 x,y;

                    while (true){

                        corte = 0;

                        for (qint32 i = 0; i<8; i++)
                        {
                            x =  nuevax + coorX8Connect[i];
                            y =  nuevay + coorY8Connect[i];

                            if ((x > 0) && (x < ancho-1) && (y > 0) && (y < alto-1) &&
                                    (sabanaCuda[x * alto + y] <= alt))
                            {
                                x =  nuevax + coorX8Connect[(i + 1) % 8];
                                y =  nuevay + coorY8Connect[(i + 1) % 8];

                                if ((sabanaCuda[x * alto + y] > alt) && (!mascara->at(y * ancho + x))){
                                    lista->append(QPointF(((nuevax * nPuntosFas) / tamFas) - nPuntosFas, ((nuevay *
                                                          nPuntosMag) / tamMag) - moverMag));
                                    mascara->replace(nuevay * ancho + nuevax, true);
                                    nuevax = x;
                                    nuevay = y;
                                    corte++;
                                    break;
                                }
                            }
                        }


                        if (corte == 0){
                            lista->append(QPointF(((nuevax * nPuntosFas) / tamFas) - nPuntosFas, ((nuevay *
                                                  nPuntosMag) / tamMag) - moverMag));
                            break;
                        }
                    }

                    if (lista->size() <= 1){
                        lista->clear();
                    } else{
                        resultado->append(lista);
                    }
                } else {
                    resultado->append(lista);
                }
            }
        }
    }

    mascara->clear();

    return resultado;

}

qreal Contour2::getAltura(){

    qreal a, b;

    if (altura->nombre == "seguimiento"){

        a = altura->getAltura(omega);
        b = altura2->getAltura(omega);

        //std::cout << "a: " << a << ", b: " << b << ", b-a: " << a - b << endl;

        return  a - b;
    }

    a = altura->getAltura(omega);

    return a;
}
