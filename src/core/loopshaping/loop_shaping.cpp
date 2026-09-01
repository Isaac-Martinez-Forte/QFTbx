#include "src/core/loopshaping/loop_shaping.h"

#include <iostream>

#include <QElapsedTimer>

LoopShaping::LoopShaping()
{
}

LoopShaping::~LoopShaping()
{
}


//Dispatch to the selected loop-shaping algorithm. Every algorithm takes
//the plant, the controller search box, the design frequencies and the
//boundaries; NK also takes the local-search starting-point choice, and
//MR the templates and specifications its constraints are built from.
bool LoopShaping::iniciar(LtiSystem * planta, LtiSystem * controlador, QVector<qreal> * omega,
                          BoundaryData * boundaries, qreal epsilon, tools::LoopShapingAlgorithm seleccionado,
                          QVector<QVector<std::complex<qreal>> *> * temp, QVector<qftbx::SpecificationRecord *> * espe,
                          qint32 inicializacion)
{
    QElapsedTimer timer;
    bool re = false;

    const auto report = [&](LtiSystem * resultado) {
        this->controlador = resultado;
        std::cout << "LoopShaping: " << timer.elapsed() << " milliseconds" << std::endl;
        std::cout << "k: " << resultado->gain()->range().x() << std::endl;
    };

    if (seleccionado == tools::nt) {
        AlgorithmNt * nt = new AlgorithmNt();
        nt->set_datos(planta, controlador, omega, boundaries, epsilon);
        timer.start();
        re = nt->init_algorithm();
        if (re) {
            report(nt->getControlador());
        }
        delete nt;
    } else if (seleccionado == tools::nk) {
        AlgorithmNk * nk = new AlgorithmNk();
        nk->set_datos(planta, controlador, omega, boundaries, epsilon, inicializacion);
        timer.start();
        re = nk->init_algorithm();
        if (re) {
            report(nk->getControlador());
        }
        delete nk;
    } else if (seleccionado == tools::mr) {
        AlgorithmMr * mr = new AlgorithmMr();
        mr->set_datos(planta, controlador, omega, boundaries, epsilon, temp, espe);
        timer.start();
        re = mr->init_algorithm();
        if (re) {
            report(mr->getControlador());
        }
        delete mr;
    } else if (seleccionado == tools::mc1) {
        AlgorithmMc1 * mc1 = new AlgorithmMc1();
        mc1->set_datos(planta, controlador, omega, boundaries, epsilon);
        timer.start();
        re = mc1->init_algorithm();
        if (re) {
            report(mc1->getControlador());
        }
        delete mc1;
    } else if (seleccionado == tools::mc_thesis) {
        AlgorithmMcThesis * mc_thesis = new AlgorithmMcThesis();
        mc_thesis->set_datos(planta, controlador, omega, boundaries, epsilon);
        timer.start();
        re = mc_thesis->init_algorithm();
        if (re) {
            report(mc_thesis->getControlador());
        }
        delete mc_thesis;
    }

    return re;
}

LtiSystem * LoopShaping::getControlador()
{
    return controlador;
}
