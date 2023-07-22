#include "algoritmointerseccionproyectivo2D.h"

using namespace std;


AlgoritmoInterseccionProyectivo2D::AlgoritmoInterseccionProyectivo2D()
{

}

AlgoritmoInterseccionProyectivo2D::~AlgoritmoInterseccionProyectivo2D()
{

}

//Algoritmo Flood Fill de 4 Direcciones (Algoritmo de Rellenado de Bote de Pintura)
void AlgoritmoInterseccionProyectivo2D::rellenarZona(bool ** &fotoIntermedia, qint32 x, qint32 y, qint32 totalFase, qint32 totalMagnitud, bool zonaPermitida)
{
    bool alguna = true;
    QStack<QPointF> pila;
    //cout << "X: " << x << " Y:" << y << endl;
    while(alguna)
    {
        fotoIntermedia[x][y] = zonaPermitida;
        alguna = false;
        if(x<totalFase-1) if(fotoIntermedia[x+1][y]==!zonaPermitida) {x++; alguna = true; pila.push(QPointF(x,y));}
        if(x>0) if(fotoIntermedia[x-1][y]==!zonaPermitida) {x--; alguna = true; pila.push(QPointF(x,y));}
        if(y<totalMagnitud-1) if(fotoIntermedia[x][y+1]==!zonaPermitida) {y++; alguna = true; pila.push(QPointF(x,y));}
        if(y>0) if(fotoIntermedia[x][y-1]==!zonaPermitida) {y--; alguna = true; pila.push(QPointF(x,y));}
        if(!alguna&&!pila.empty())
        {
            x = pila.top().x();
            y = pila.top().y();
            pila.pop();
            alguna = true;
        }
    }
}

//Función para dibujar sólo los puntos de la frontera en el grid
void AlgoritmoInterseccionProyectivo2D::dibujarFrontera(QVector<QPointF> * curva, bool ** &fotoIntermedia, qint32 totalFase, qint32 totalMagnitud, qint32 minDB, bool zonaPermitida)
{
    for(qint32 x=0;x<totalFase;x++)
    {
        //Según si la zona permitida es por Arriba o por Abajo se inicializa el grid a 1 (si por Arriba) o a 0 (si por Abajo)
        memset(fotoIntermedia[x], !zonaPermitida, totalMagnitud);
    }

    //Dibujamos
    for(qint32 i=0;i<curva->size();i++)
    {
        if(curva->at(i).y()>0)
            fotoIntermedia[(qint32)curva->at(i).x()+(totalFase-1)][(qint32)curva->at(i).y()+minDB+totalMagnitud] = zonaPermitida;
        else if(curva->at(i).y()==0)
        {
            fotoIntermedia[(qint32)curva->at(i).x()+(totalFase-1)][(qint32)curva->at(i).y()+minDB+totalMagnitud] = zonaPermitida;
            fotoIntermedia[(qint32)curva->at(i).x()+(totalFase-1)][(qint32)curva->at(i).y()+minDB+totalMagnitud-1] = zonaPermitida;
        }
        else
            fotoIntermedia[(qint32)curva->at(i).x()+(totalFase-1)][(qint32)curva->at(i).y()+minDB+totalMagnitud-1] = zonaPermitida;
    }
}

//Método que retorna las intersecciones de boundaries
QVector <bool **> * AlgoritmoInterseccionProyectivo2D::getInterseccionesBoundaries(){
    return interseccionesBoundaries;
}

//Algoritmo principal del Pintor
void AlgoritmoInterseccionProyectivo2D::ejecutarAlgoritmo(DatosBound * boundaries,
                                                    QVector <QMap <QString, QVector <QPoint> * > * > * vectorMetadatos)
{
    //Vamos a guardar un plano de nichols intersección por cada especificación
    interseccionesBoundaries = new QVector <bool **> ();


    //Cada Frontera tiene un Mapa, cada mapa tiene unas Especificaciones, y cada Especificación tiene una Curva Paramétrica de Puntos(Fase, Magnitud) que empieza desde -360º a 0º
    //El tipo que devuelve para fase y magnitud es de tipo entero
    QVector <QMap <QString, QVector <QVector <QPointF> * > *> * > * vectorBoundaries = boundaries->getBoundaries();

    qint32 totalMagnitud = boundaries->getTamMag();
    qint32 totalFase = boundaries->getTamFas();
    qint32 minDB = boundaries->getDatosMag().x();

    for(qint32 i=0;i<vectorBoundaries->size();i++) {

        QMap <QString, QVector <QVector <QPointF> * > *> * mapa = vectorBoundaries->at(i);

        QMap <QString, QVector <QPoint> * > * mapaMetadatos = vectorMetadatos->at(i);

        //Representamos el plano de Nichols en un array de booleanos
        bool ** planoNichols = new bool *[(qint32)totalFase];
        bool ** fotoIntermedia = new bool *[(qint32)totalFase];

        //En un principio hay que inicializar el Plano de Nichols, por defecto lo inicializamos a True pero también se podría incializar a False;
        //es indiferente para el resultado final ya que la primera capa que se superponga cambiará, o no, el resultado dependiendo de si la zona
        //permitida es por Arriba o por Abajo

        for(qint32 i=0;i<totalFase;i++)
        {
            planoNichols[i] = new bool [(qint32)totalMagnitud];
            memset(planoNichols[i], 1, totalMagnitud);

            fotoIntermedia[i] = new bool [(qint32)totalMagnitud];
        }

        //Dibujamos las fronteras en el plano de Nichols
        foreach (QVector <QVector <QPointF> * > * vector_especificaciones, *mapa)
        {
            QMapIterator <QString, QVector <QPoint> * > iterador(*mapaMetadatos);
            bool zonaPermitida = true; //La inicializamos a true por inicializar la variable a algún valor
            bool abierta = false, arriba = false;
            if(iterador.hasNext())
            {
                iterador.next();
                QVector <QPoint> * fronterasMetadatos = iterador.value();
                //Si en el eje X de cualquiera de los puntos devuelve 0 la parte permitida va por Arriba y 1 si va por Abajo
                //cout << "El metadato X es " << fronteras_metadatos->at(0).x() << endl;
                zonaPermitida = fronterasMetadatos->at(0).x(); //Nos vale el primer punto para saber si la zona permitida es por Arriba o por Abajo
                arriba = !zonaPermitida;
            }
            for(qint32 j=0;j<(qint32)vector_especificaciones->size();j++)
            {
                QVector <QPointF> * vec = vector_especificaciones->at(j);

                //cout << vec->at(0).x() << " -=- " << vec->at(vec->size()-1).x() << endl;
                if(vec->at(0).x()==0&&vec->at(vec->size()-1).x()==-1) abierta = true;
                dibujarFrontera(vec, fotoIntermedia, totalFase, totalMagnitud, minDB, zonaPermitida);
            }

            //Tras dibujar la frontera buscamos un punto dentro de la zona No Permitida si Arriba o Zona Permitida si Abajo
            qint32 x=0, y=0; //Inicializamos las variables a 0 por ejemplo
            //cout << abierta << " --- " << arriba << endl;

            if(abierta&&arriba)
            {
                //Rellenamos con el Algoritmo Flood Fill de 4 Direcciones con Pila (Algoritmo de Bote de Pintura)
                rellenarZona(fotoIntermedia, x, y, totalFase, totalMagnitud, zonaPermitida);
            }
            else
            {
                for(qint32 j=1;j<totalFase;j++)
                {
                    bool cambio = false, primeraLinea = true;
                    for(int k=1;k<totalMagnitud-1;k++)
                    {
                        if(primeraLinea&&fotoIntermedia[j][k-1]==!zonaPermitida&&fotoIntermedia[j][k]==zonaPermitida&&fotoIntermedia[j][k+1]==!zonaPermitida&&fotoIntermedia[j+1][k+1]==!zonaPermitida&&fotoIntermedia[j-1][k+1]==!zonaPermitida)
                        {
                            int lineas = 1;
                            for(qint32 l=k+1;l<totalMagnitud;l++) //Contamos líneas totales
                            {
                                //Si hay otra línea más
                                if(fotoIntermedia[j][l-1]==!zonaPermitida&&fotoIntermedia[j][l]==zonaPermitida&&fotoIntermedia[j][l+1]==!zonaPermitida&&fotoIntermedia[j+1][l+1]==!zonaPermitida&&fotoIntermedia[j-1][l+1]==!zonaPermitida)
                                {
                                    lineas++; //La añadimos al contador de líneas
                                }
                            }
                            if(lineas%2==0) //Si el total de líneas es par es porque estamos dentro
                                cambio = true;
                            if(cambio)
                            {
                                x = j;
                                y = k+1;
                                break;
                            }
                        }
                        else if(fotoIntermedia[j][k]==zonaPermitida) primeraLinea = false;
                    }
                    if(cambio)
                    {
                        //Rellenamos con el Algoritmo Flood Fill de 4 Direcciones (Algoritmo de Bote de Pintura)
                        rellenarZona(fotoIntermedia, x, y, totalFase, totalMagnitud, zonaPermitida);
                    }
                }
            }


            //Ahora superponemos la foto intermedia encima del plano de Nichols que llevamos hasta ahora
            for(qint32 j=0;j<totalFase;j++)
            {
                for(qint32 k=0;k<totalMagnitud;k++)
                {
                    planoNichols[j][k] = planoNichols[j][k] && fotoIntermedia[j][k];
                }
            }

        }

        delete [] fotoIntermedia;
        interseccionesBoundaries->append(planoNichols);
    }
}


