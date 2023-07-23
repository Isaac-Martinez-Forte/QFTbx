#include "loopshaping.h"

using namespace std;


//#define pruebas
//#define saltar

LoopShaping::LoopShaping()
{

}

LoopShaping::~LoopShaping()
{

}


bool LoopShaping::iniciar(Sistema *planta, Sistema *controlador, QVector<qreal> *omega, std::shared_ptr<DatosBound> boundaries,
                          qreal epsilon, tools::alg_loop_shaping seleccionado, bool depuracion, qreal delta,
                          QVector <QVector <std::complex <qreal> > * > * temp, QVector <tools::dBND *> * espe,
                          qint32 inicializacion, bool hilos, bool bisection_avanced, bool deteccion_avanced, bool a){


    //sacamos el círculo envolvente del boundarie
    QVector< QVector<QPointF> * > * boun = boundaries->getBoundariesReun();

    qreal maglineal;
    qreal x,y;

    qreal mayorDistancia, menorDistancia;
    qreal distancia;

    qreal media_x, media_y;
    QVector <QPointF> * centros = new QVector <QPointF> ();
    QVector <qreal> * radiosMayor = new QVector <qreal> ();
    QVector <qreal> * radiosMenor = new QVector <qreal> ();

    foreach (auto vector, *boun) {

        mayorDistancia = std::numeric_limits<qreal>::lowest();
        menorDistancia = std::numeric_limits<qreal>::max();
        media_x = 0; media_y = 0;

        foreach (auto p, *vector) {
            maglineal = pow(10,p.y()/20);
            media_x += maglineal * cos (p.x() * M_PI / 180);
            media_y += maglineal * sin (p.x() * M_PI / 180);
        }

        QPointF centro (media_x/vector->size(), media_y/vector->size());

        foreach (auto p, *vector) {
            maglineal = pow(10,p.y()/20);
            x = maglineal * cos (p.x() * M_PI / 180);
            y = maglineal * sin (p.x() * M_PI / 180);

            distancia = sqrt(pow(x - centro.x(), 2) + pow(y - centro.y(), 2));

            if (distancia > mayorDistancia){
                mayorDistancia = distancia;
            }

            if (distancia < menorDistancia){
                menorDistancia = distancia;
            }
        }

        centros->append(centro);
        radiosMayor->append(mayorDistancia);
        radiosMenor->append(menorDistancia);
    }

    QElapsedTimer timer;

    bool re;

#ifdef pruebas
    qint32 nVariables = 8, contador = 1, contadorNume = 0, contadorDeno = 0;

    if (seleccionado == tools::sachin){
        std::cout << "Algoritmo Sachín" << std::endl;
    }else if (seleccionado == tools::nandkishor){
        std::cout << "Algoritmo Nandkishor" << std::endl;
    } else if(seleccionado == tools::isaac){
        std::cout << "Algoritmo Isaac" << std::endl;
    }


    for (; contador <= nVariables;) {
#endif

#ifdef saltar
        if (contador != 5) {
#endif

            if (seleccionado == tools::sachin){
#ifndef pruebas
                std::cout << "Algoritmo Sachín" << std::endl;
#endif
                Algorithm_sachin * sachin = new Algorithm_sachin();
                timer.start();
                sachin->set_datos(planta, controlador, omega, boundaries, epsilon, boundaries->getBoundariesReunHash());
                re =  sachin->init_algorithm();
#ifndef pruebas
                if (re) {
                    this->controlador = sachin->getControlador();
                    std::cout << "LoopShaping: " << timer.elapsed() << " milliseconds" << std::endl;
                    std::cout << "k: " << this->controlador->getK()->getRango().x() << std::endl;
                }


                delete sachin;

                return re;
#else
                std::cout << timer.elapsed() << std::endl;
                delete sachin;
#endif
            } else if (seleccionado == tools::nandkishor){
#ifndef pruebas
                std::cout << "Algoritmo Nandkishor" << std::endl;
#endif
                timer.start();
                Algorithm_nandkishor * nandkishor = new Algorithm_nandkishor();
                nandkishor->set_datos(planta, controlador, omega, boundaries, epsilon, boundaries->getBoundariesReunHash(),
                                      delta, inicializacion);
                re =  nandkishor->init_algorithm();

#ifndef pruebas
                if (re){
                    this->controlador = nandkishor->getControlador();

                    std::cout << "LoopShaping: " << timer.elapsed() << " milliseconds" << std::endl;
                    std::cout << "k: " << this->controlador->getK()->getRango().x() << std::endl;

                }

                delete nandkishor;

                return re;
#else
                std::cout << timer.elapsed()  << std::endl;
                delete nandkishor;
#endif
            } else if (seleccionado == tools::rambabu){

                std::cout << "Algoritmo Rambabú" << std::endl;

                Algorithm_rambabu * rambabu = new Algorithm_rambabu();
                rambabu->set_datos(planta, controlador, omega, boundaries, epsilon, boundaries->getBoundariesReunHash(),
                                   depuracion, temp, espe);
                re =  rambabu->init_algorithm();

                if(re){
                    this->controlador = rambabu->getControlador();

                    std::cout << "LoopShaping: " << timer.elapsed() << " milliseconds" << std::endl;
                    std::cout << "k: " << this->controlador->getK()->getRango().x() << std::endl;

                }

                delete rambabu;

                return re;
            } else if(seleccionado == tools::primer_articulo){
#ifndef pruebas
                std::cout << "Algoritmo primero" << std::endl;
#endif
                Algorithm_primer_articulo * primer_articulo = new Algorithm_primer_articulo();

                primer_articulo->set_datos(planta, controlador, omega, boundaries, epsilon, boundaries->getBoundariesReunHash(),
                                 depuracion, hilos, radiosMayor, radiosMenor, centros, bisection_avanced, deteccion_avanced, a);

                timer.start();

                re =  primer_articulo->init_algorithm();
#ifndef pruebas

                if (re) {
                    std::cout << "LoopShaping: " << timer.elapsed() << " milliseconds" << std::endl;
                    this->controlador = primer_articulo->getControlador();
                    std::cout << "k: " << this->controlador->getK()->getRango().x() << std::endl;
                }

                delete primer_articulo;

                return re;
#else
                std::cout << timer.elapsed() << std::endl;
                delete primer_articulo;
#endif
            }else if(seleccionado == tools::segundo_articulo){
#ifndef pruebas
                std::cout << "Algoritmo segundo" << std::endl;
#endif
                Algorithm_segundo_articulo * segundo_articulo = new Algorithm_segundo_articulo();

                segundo_articulo->set_datos(planta, controlador, omega, boundaries, epsilon);

                timer.start();

                re =  segundo_articulo->init_algorithm();
#ifndef pruebas

                if (re) {
                    std::cout << "LoopShaping: " << timer.elapsed() << " milliseconds" << std::endl;
                    this->controlador = segundo_articulo->getControlador();
                    std::cout << "k: " << this->controlador->getK()->getRango().x() << std::endl;
                }

                delete segundo_articulo;

                return re;
#else
                std::cout << timer.elapsed() << std::endl;
                delete segundo_articulo;
#endif
            }
#ifdef pruebas


#ifdef saltar
        }
#endif

        contador++;

        if (contador % 2 == 0) {
            controlador->getDenominador()->at(contadorDeno)->setVariable(true);
            contadorDeno++;
        } else {

            controlador->getNumerador()->at(contadorNume)->setVariable(true);
            contadorNume++;
        }
    }

    std::cout << "terminado" << std::endl;

    return re;
#endif

    return false;
}


Sistema * LoopShaping::getControlador(){
    return controlador;
}
