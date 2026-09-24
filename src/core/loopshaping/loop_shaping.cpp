/**
 * @file
 * @brief Dispatch to the selected loop-shaping algorithm.
 *
 * Every algorithm takes the plant, the controller search box, the design
 * frequencies and the boundaries; NK also takes the starting point of its
 * local search, and MR the templates and specifications its constraints are
 * built from. Before any of them starts, and outside their parallel regions,
 * the phase window of the boundaries is checked to cover the whole phase a
 * loop can take, because a narrower window would have the search read
 * verdicts for phases nobody computed. The cost counters of the run are read
 * from the algorithm and kept with the result.
 */

#include <chrono>
#include <vector>
#include <cstdint>
#include "src/core/common/text_tokens.h"
#include "src/core/common/record.h"
#include "src/core/loopshaping/algorithm_name.h"
#include "src/core/loopshaping/loop_shaping.h"

#include <cmath>
#include <memory>

#include "src/core/common/exception.h"

namespace qftbx {

bool LoopShaping::run(LtiSystem * plant, LtiSystem * controller, std::vector<double> * omega,
                          const BoundaryData * boundaries, double epsilon, qftbx::LoopShapingAlgorithm algorithm,
                          const qftbx::CloudSet & contour, const qftbx::SpecificationRecords * specifications,
                          std::int32_t initialisation)
{
    const double phaseSpan = std::abs(boundaries->phaseRange().width());

    if (phaseSpan < 360.0) {
        throw qftbx::ComputationError(QFTBX_TR("Core", "The boundaries were computed over a Nichols phase window of %1 degrees ([%2, %3]), which does not cover the full range a loop phase can take (-360 to 0 degrees). Recompute the boundaries over a window of at least 360 degrees.")
            .arg(phaseSpan).arg(boundaries->phaseRange().min).arg(boundaries->phaseRange().max));
    }

    auto timer = std::chrono::steady_clock::now();
    bool solved = false;

    m_statistics = LoopShapingStatistics();
    const auto report = [&](std::unique_ptr<LtiSystem> designed, LoopShapingStatistics statistics) {
        statistics.milliseconds = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - timer).count();
        m_statistics = statistics;
        m_controller = std::move(designed);
    };

    if (algorithm == qftbx::nt) {
        auto nt = std::make_unique<AlgorithmNt>();
        nt->setProblem(plant, controller, omega, boundaries, epsilon);
        nt->setCancellation(m_cancellation);
        nt->setSettings(m_settings);
        nt->setPlantFamily(m_sweep);
        timer = std::chrono::steady_clock::now();
        solved = nt->solve();
        if (solved) {
            report(nt->controllerStructure(), nt->statistics());
        }
    } else if (algorithm == qftbx::nk) {
        auto nk = std::make_unique<AlgorithmNk>();
        nk->setProblem(plant, controller, omega, boundaries, epsilon, initialisation);
        nk->setCancellation(m_cancellation);
        nk->setSettings(m_settings);
        nk->setPlantFamily(m_sweep);
        timer = std::chrono::steady_clock::now();
        solved = nk->solve();
        if (solved) {
            report(nk->controllerStructure(), nk->statistics());
        }
    } else if (algorithm == qftbx::mr) {
        auto mr = std::make_unique<AlgorithmMr>();
        mr->setProblem(plant, controller, omega, boundaries, epsilon, contour, specifications);
        mr->setCancellation(m_cancellation);
        mr->setSettings(m_settings);
        timer = std::chrono::steady_clock::now();
        solved = mr->solve();
        if (solved) {
            report(mr->controllerStructure(), mr->statistics());
        }
    } else if (algorithm == qftbx::mc1) {
        auto mc1 = std::make_unique<AlgorithmMc1>();
        mc1->setProblem(plant, controller, omega, boundaries, epsilon);
        mc1->setCancellation(m_cancellation);
        mc1->setSettings(m_settings);
        mc1->setPlantFamily(m_sweep);
        timer = std::chrono::steady_clock::now();
        solved = mc1->solve();
        if (solved) {
            report(mc1->controllerStructure(), mc1->statistics());
        }
    } else if (algorithm == qftbx::mc_thesis) {
        auto mc_thesis = std::make_unique<AlgorithmMcThesis>();
        mc_thesis->setProblem(plant, controller, omega, boundaries, epsilon);
        mc_thesis->setCancellation(m_cancellation);
        mc_thesis->setSettings(m_settings);
        mc_thesis->setPlantFamily(m_sweep);
        timer = std::chrono::steady_clock::now();
        solved = mc_thesis->solve();
        if (solved) {
            report(mc_thesis->controllerStructure(), mc_thesis->statistics());
        }
    } else if (algorithm == qftbx::mc2) {
        auto mc2 = std::make_unique<AlgorithmMc2>();
        mc2->setProblem(plant, controller, omega, boundaries, epsilon);
        mc2->setCancellation(m_cancellation);
        mc2->setSettings(m_settings);
        mc2->setPlantFamily(m_sweep);
        timer = std::chrono::steady_clock::now();
        solved = mc2->solve();
        if (solved) {
            report(mc2->controllerStructure(), mc2->statistics());
        }
    } else if (algorithm == qftbx::mc3) {
        auto mc3 = std::make_unique<AlgorithmMc3>();
        mc3->setProblem(plant, controller, omega, boundaries, epsilon);
        mc3->setCancellation(m_cancellation);
        mc3->setSettings(m_settings);
        mc3->setPlantFamily(m_sweep);
        timer = std::chrono::steady_clock::now();
        solved = mc3->solve();
        if (solved) {
            report(mc3->controllerStructure(), mc3->statistics());
        }
    }

    if (qftbx::record::isOpen()){
        std::string numbers = qftbx::record::milliseconds(m_statistics.milliseconds)
                              + " " + qftbx::record::number("epsilon", epsilon)
                              + " " + qftbx::record::number("frequencies", omega->size());
        if (solved && m_controller != nullptr){
            numbers += " " + qftbx::record::number("gain", m_controller->gain().range().min);
        } else {
            numbers += " solved=no";
        }
        qftbx::record::write("loop shaping", qftbx::algorithmName(algorithm), numbers);
    }

    return solved;
}

std::unique_ptr<LtiSystem> LoopShaping::controllerStructure()
{
    return std::move(m_controller);
}

}
