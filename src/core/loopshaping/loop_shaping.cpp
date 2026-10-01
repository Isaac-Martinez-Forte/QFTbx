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
 * verdicts for phases nobody computed. When MC2 reads the points exactly,
 * the specification records are turned into the set the exact check
 * evaluates, once per run, and handed to it with the templates. Each branch
 * states the problem; the cancellation, the settings, the clock and the
 * search are the same for all seven. The cost counters of the run are read
 * from the algorithm and kept with the result.
 */

#include "src/core/loopshaping/loop_shaping.h"

#include <chrono>
#include <cmath>
#include <optional>

#include "src/core/common/exception.h"
#include "src/core/common/record.h"
#include "src/core/loopshaping/algorithm_name.h"
#include "src/core/loopshaping/mc1/algorithm_mc1.h"
#include "src/core/loopshaping/mc2/algorithm_mc2.h"
#include "src/core/loopshaping/mc3/algorithm_mc3.h"
#include "src/core/loopshaping/mc_thesis/algorithm_mc_thesis.h"
#include "src/core/loopshaping/mr/algorithm_mr.h"
#include "src/core/loopshaping/nk/algorithm_nk.h"
#include "src/core/loopshaping/nt/algorithm_nt.h"

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

    bool solved = false;
    m_statistics = LoopShapingStatistics();

    const auto solveWith = [&](auto & search) {
        search.setCancellation(m_cancellation);
        search.setSettings(m_settings);
        const auto start = std::chrono::steady_clock::now();
        solved = search.solve();
        if (solved) {
            LoopShapingStatistics statistics = search.statistics();
            statistics.milliseconds = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
            m_statistics = std::move(statistics);
            m_controller = search.controllerStructure();
        }
    };

    switch (algorithm) {
    case qftbx::nt: {
        auto nt = std::make_unique<AlgorithmNt>();
        nt->setProblem(plant, controller, omega, boundaries, epsilon);
        nt->setPlantFamily(m_sweep);
        solveWith(*nt);
        break;
    }
    case qftbx::nk: {
        auto nk = std::make_unique<AlgorithmNk>();
        nk->setProblem(plant, controller, omega, boundaries, epsilon, initialisation);
        nk->setPlantFamily(m_sweep);
        solveWith(*nk);
        break;
    }
    case qftbx::mr: {
        auto mr = std::make_unique<AlgorithmMr>();
        mr->setProblem(plant, controller, omega, boundaries, epsilon, contour, specifications);
        solveWith(*mr);
        break;
    }
    case qftbx::mc1: {
        auto mc1 = std::make_unique<AlgorithmMc1>();
        mc1->setProblem(plant, controller, omega, boundaries, epsilon);
        mc1->setPlantFamily(m_sweep);
        solveWith(*mc1);
        break;
    }
    case qftbx::mc_thesis: {
        auto mcThesis = std::make_unique<AlgorithmMcThesis>();
        mcThesis->setProblem(plant, controller, omega, boundaries, epsilon);
        mcThesis->setPlantFamily(m_sweep);
        solveWith(*mcThesis);
        break;
    }
    case qftbx::mc2: {
        std::optional<qftbx::SpecificationSet> specificationSet;
        if (m_settings.research.mc2Reading == qftbx::Settings::Research::PointReading::Exact
                && specifications != nullptr) {
            specificationSet.emplace(toSpecificationSet(*specifications));
        }
        auto mc2 = std::make_unique<AlgorithmMc2>();
        mc2->setProblem(plant, controller, omega, boundaries, epsilon);
        mc2->setPlantFamily(m_sweep);
        mc2->setSpecifications(m_templates, specificationSet.has_value() ? &*specificationSet : nullptr);
        solveWith(*mc2);
        break;
    }
    case qftbx::mc3: {
        auto mc3 = std::make_unique<AlgorithmMc3>();
        mc3->setProblem(plant, controller, omega, boundaries, epsilon);
        mc3->setPlantFamily(m_sweep);
        solveWith(*mc3);
        break;
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
