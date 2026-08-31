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


bool LoopShaping::iniciar(LtiSystem *planta, LtiSystem *controlador, QVector<qreal> *omega, BoundaryData *boundaries,
                          qreal epsilon, tools::alg_loop_shaping seleccionado, bool depuracion, qreal delta,
                          QVector <QVector <std::complex <qreal> > * > * temp, QVector <tools::dBND *> * espe,
                          qint32 inicializacion, bool hilos, bool bisection_avanced, bool deteccion_avanced, bool a){


    //sacamos el círculo envolvente del boundarie
    QVector< QVector<QPointF> * > * boun = boundaries->unionBoundaries();

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

    if (seleccionado == tools::nt){
        std::cout << "Algoritmo Sachín" << std::endl;
    }else if (seleccionado == tools::nk){
        std::cout << "Algoritmo Nandkishor" << std::endl;
    } else if(seleccionado == tools::isaac){
        std::cout << "Algoritmo Isaac" << std::endl;
    }


    for (; contador <= nVariables;) {
#endif

#ifdef saltar
        if (contador != 5) {
#endif

            if (seleccionado == tools::nt){
#ifndef pruebas
                std::cout << "Algoritmo Sachín" << std::endl;
#endif
                AlgorithmNt * nt = new AlgorithmNt();
                timer.start();
                nt->set_datos(planta, controlador, omega, boundaries, epsilon, boundaries->unionBuckets());
                re =  nt->init_algorithm();
#ifndef pruebas
                if (re) {
                    this->controlador = nt->getControlador();
                    std::cout << "LoopShaping: " << timer.elapsed() << " milliseconds" << std::endl;
                    std::cout << "k: " << this->controlador->gain()->range().x() << std::endl;
                }


                delete nt;

                return re;
#else
                std::cout << timer.elapsed() << std::endl;
                delete nt;
#endif
            } else if (seleccionado == tools::nk){
#ifndef pruebas
                std::cout << "Algoritmo Nandkishor" << std::endl;
#endif
                timer.start();
                AlgorithmNk * nk = new AlgorithmNk();
                nk->set_datos(planta, controlador, omega, boundaries, epsilon, boundaries->unionBuckets(),
                                      delta, inicializacion);
                re =  nk->init_algorithm();

#ifndef pruebas
                if (re){
                    this->controlador = nk->getControlador();

                    std::cout << "LoopShaping: " << timer.elapsed() << " milliseconds" << std::endl;
                    std::cout << "k: " << this->controlador->gain()->range().x() << std::endl;

                }

                delete nk;

                return re;
#else
                std::cout << timer.elapsed()  << std::endl;
                delete nk;
#endif
            } else if (seleccionado == tools::mr){

                std::cout << "Algoritmo Rambabú" << std::endl;

                AlgorithmMr * mr = new AlgorithmMr();
                mr->set_datos(planta, controlador, omega, boundaries, epsilon, boundaries->unionBuckets(),
                                   depuracion, temp, espe);

                //The elapsed time printed below was garbage: this branch
                //never started the timer.
                timer.start();
                re =  mr->init_algorithm();

                if(re){
                    this->controlador = mr->getControlador();

                    std::cout << "LoopShaping: " << timer.elapsed() << " milliseconds" << std::endl;
                    std::cout << "k: " << this->controlador->gain()->range().x() << std::endl;

                }

                delete mr;

                return re;
            } else if(seleccionado == tools::mc1){
#ifndef pruebas
                std::cout << "Algoritmo primero" << std::endl;
#endif
                AlgorithmMc1 * mc1 = new AlgorithmMc1();

                mc1->set_datos(planta, controlador, omega, boundaries, epsilon, boundaries->unionBuckets(),
                                 depuracion, hilos, radiosMayor, radiosMenor, centros, bisection_avanced, deteccion_avanced, a);

                timer.start();

                re =  mc1->init_algorithm();
#ifndef pruebas

                if (re) {
                    std::cout << "LoopShaping: " << timer.elapsed() << " milliseconds" << std::endl;
                    this->controlador = mc1->getControlador();
                    std::cout << "k: " << this->controlador->gain()->range().x() << std::endl;
                }

                delete mc1;

                return re;
#else
                std::cout << timer.elapsed() << std::endl;
                delete mc1;
#endif
            }else if(seleccionado == tools::mc_thesis){
#ifndef pruebas
                std::cout << "Algoritmo segundo" << std::endl;
#endif
                AlgorithmMcThesis * mc_thesis = new AlgorithmMcThesis();

                mc_thesis->set_datos(planta, controlador, omega, boundaries, epsilon);

                timer.start();

                re =  mc_thesis->init_algorithm();
#ifndef pruebas

                if (re) {
                    std::cout << "LoopShaping: " << timer.elapsed() << " milliseconds" << std::endl;
                    this->controlador = mc_thesis->getControlador();
                    std::cout << "k: " << this->controlador->gain()->range().x() << std::endl;
                }

                delete mc_thesis;

                return re;
#else
                std::cout << timer.elapsed() << std::endl;
                delete mc_thesis;
#endif
            }
#ifdef pruebas


#ifdef saltar
        }
#endif

        contador++;

        if (contador % 2 == 0) {
            controlador->denominator()->at(contadorDeno)->setUncertain(true);
            contadorDeno++;
        } else {

            controlador->numerator()->at(contadorNume)->setUncertain(true);
            contadorNume++;
        }
    }

    std::cout << "terminado" << std::endl;

    return re;
#endif

    return false;
}


LtiSystem * LoopShaping::getControlador(){
    return controlador;
}
