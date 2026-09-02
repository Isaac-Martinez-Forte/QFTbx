#include "src/core/loopshaping/loop_shaping.h"

#include <cmath>
#include <iostream>
#include <memory>

#include <QElapsedTimer>

#include "src/core/exception.h"

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
                          const BoundaryData * boundaries, qreal epsilon, tools::LoopShapingAlgorithm algorithm,
                          const qftbx::CloudSet & contour, const qftbx::SpecificationRecords * specifications,
                          qint32 initialisation)
{
    //Precondition, checked ONCE and sequentially, before any algorithm
    //starts: the phase window the boundaries were computed over must cover
    //the phase a loop can take. Every caller normalises phase into
    //(-360, 0], so a narrower window leaves the search classifying loop
    //points against buckets that were never computed for their phase - the
    //reader clamps to the edge bucket rather than reading out of bounds, so
    //the answer would be a verdict nobody calculated, which for a search
    //that claims a global optimum is worse than an error.
    //
    //Here and not inside the algorithms: a throw escaping an OpenMP region
    //ends the process. A narrow window is still fine for merely LOOKING at
    //boundaries, which is why the boundaries dialog does not forbid it.
    const qreal phaseSpan = std::abs(boundaries->phaseRange().y() - boundaries->phaseRange().x());

    if (phaseSpan < 360.0) {
        const QString message =
            QString("The boundaries were computed over a Nichols phase window of "
                    "%1 degrees ([%2, %3]), which does not cover the full range a "
                    "loop phase can take (-360 to 0 degrees). Recompute the "
                    "boundaries over a window of at least 360 degrees.")
                .arg(phaseSpan)
                .arg(boundaries->phaseRange().x())
                .arg(boundaries->phaseRange().y());

        throw qftbx::ComputationError(message.toStdString());
    }

    //The algorithms own themselves through unique_ptr: solve()
    //throws on an invalid or infeasible problem, and the raw new/delete
    //pair leaked the whole algorithm (its lists, its detection, its
    //nominal-plant caches) on every such throw.
    QElapsedTimer timer;
    bool re = false;

    //The peak live-node count is what a run costs in memory, and what the
    //ceiling of kDefaultMaxLiveNodes has to be tuned against: it is reported
    //rather than left to be guessed.
    const auto report = [&](std::unique_ptr<LtiSystem> resultado, std::size_t peakLiveNodes) {
        std::cout << "LoopShaping: " << timer.elapsed() << " milliseconds" << std::endl;
        std::cout << "k: " << resultado->gain().range().min << std::endl;
        std::cout << "peak live nodes: " << peakLiveNodes << std::endl;
        this->controller = std::move(resultado);
    };

    if (algorithm == tools::nt) {
        auto nt = std::make_unique<AlgorithmNt>();
        nt->setProblem(plant, controller, omega, boundaries, epsilon);
        timer.start();
        re = nt->solve();
        if (re) {
            report(nt->controllerStructure(), nt->peakLiveNodes());
        }
    } else if (algorithm == tools::nk) {
        auto nk = std::make_unique<AlgorithmNk>();
        nk->setProblem(plant, controller, omega, boundaries, epsilon, initialisation);
        timer.start();
        re = nk->solve();
        if (re) {
            report(nk->controllerStructure(), nk->peakLiveNodes());
        }
    } else if (algorithm == tools::mr) {
        auto mr = std::make_unique<AlgorithmMr>();
        mr->setProblem(plant, controller, omega, boundaries, epsilon, contour, specifications);
        timer.start();
        re = mr->solve();
        if (re) {
            report(mr->controllerStructure(), mr->peakLiveNodes());
        }
    } else if (algorithm == tools::mc1) {
        auto mc1 = std::make_unique<AlgorithmMc1>();
        mc1->setProblem(plant, controller, omega, boundaries, epsilon);
        timer.start();
        re = mc1->solve();
        if (re) {
            report(mc1->controllerStructure(), mc1->peakLiveNodes());
        }
    } else if (algorithm == tools::mc_thesis) {
        auto mc_thesis = std::make_unique<AlgorithmMcThesis>();
        mc_thesis->setProblem(plant, controller, omega, boundaries, epsilon);
        timer.start();
        re = mc_thesis->solve();
        if (re) {
            report(mc_thesis->controllerStructure(), mc_thesis->peakLiveNodes());
        }
    }

    return re;
}

std::unique_ptr<LtiSystem> LoopShaping::controllerStructure()
{
    return std::move(controller);
}
