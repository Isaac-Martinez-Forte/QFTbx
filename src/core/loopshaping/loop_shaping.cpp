#include "src/core/loopshaping/loop_shaping.h"

#include <iostream>
#include <memory>

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
bool LoopShaping::run(LtiSystem * plant, LtiSystem * controller, QVector<qreal> * omega,
                          BoundaryData * boundaries, qreal epsilon, tools::LoopShapingAlgorithm algorithm,
                          QVector<QVector<std::complex<qreal>> *> * contour, QVector<qftbx::SpecificationRecord *> * specifications,
                          qint32 initialisation)
{
    //The algorithms own themselves through unique_ptr: init_algorithm()
    //throws on an invalid or infeasible problem, and the raw new/delete
    //pair leaked the whole algorithm (its lists, its detection, its
    //nominal-plant caches) on every such throw.
    QElapsedTimer timer;
    bool re = false;

    const auto report = [&](LtiSystem * resultado) {
        this->controller = resultado;
        std::cout << "LoopShaping: " << timer.elapsed() << " milliseconds" << std::endl;
        std::cout << "k: " << resultado->gain().range().min << std::endl;
    };

    if (algorithm == tools::nt) {
        auto nt = std::make_unique<AlgorithmNt>();
        nt->set_datos(plant, controller, omega, boundaries, epsilon);
        timer.start();
        re = nt->init_algorithm();
        if (re) {
            report(nt->controllerStructure());
        }
    } else if (algorithm == tools::nk) {
        auto nk = std::make_unique<AlgorithmNk>();
        nk->set_datos(plant, controller, omega, boundaries, epsilon, initialisation);
        timer.start();
        re = nk->init_algorithm();
        if (re) {
            report(nk->controllerStructure());
        }
    } else if (algorithm == tools::mr) {
        auto mr = std::make_unique<AlgorithmMr>();
        mr->set_datos(plant, controller, omega, boundaries, epsilon, contour, specifications);
        timer.start();
        re = mr->init_algorithm();
        if (re) {
            report(mr->controllerStructure());
        }
    } else if (algorithm == tools::mc1) {
        auto mc1 = std::make_unique<AlgorithmMc1>();
        mc1->set_datos(plant, controller, omega, boundaries, epsilon);
        timer.start();
        re = mc1->init_algorithm();
        if (re) {
            report(mc1->controllerStructure());
        }
    } else if (algorithm == tools::mc_thesis) {
        auto mc_thesis = std::make_unique<AlgorithmMcThesis>();
        mc_thesis->set_datos(plant, controller, omega, boundaries, epsilon);
        timer.start();
        re = mc_thesis->init_algorithm();
        if (re) {
            report(mc_thesis->controllerStructure());
        }
    }

    return re;
}

LtiSystem * LoopShaping::controllerStructure()
{
    return controller;
}
