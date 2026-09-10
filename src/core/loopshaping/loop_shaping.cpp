#include <chrono>
#include <vector>
#include <cstdint>
#include "src/core/common/text_tokens.h"
#include "src/core/loopshaping/loop_shaping.h"

#include <cmath>
#include <memory>


#include "src/core/common/exception.h"

//Dispatch to the selected loop-shaping algorithm. Every algorithm takes
//the plant, the controller search box, the design frequencies and the
//boundaries; NK also takes the local-search starting-point choice, and
//MR the templates and specifications its constraints are built from.
namespace qftbx {

bool LoopShaping::run(LtiSystem * plant, LtiSystem * controller, std::vector<double> * omega,
                          const BoundaryData * boundaries, double epsilon, qftbx::LoopShapingAlgorithm algorithm,
                          const qftbx::CloudSet & contour, const qftbx::SpecificationRecords * specifications,
                          std::int32_t initialisation)
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
    const double phaseSpan = std::abs(boundaries->phaseRange().width());

    if (phaseSpan < 360.0) {
        throw qftbx::ComputationError(QFTBX_TR("Core", "The boundaries were computed over a Nichols phase window of %1 degrees ([%2, %3]), which does not cover the full range a loop phase can take (-360 to 0 degrees). Recompute the boundaries over a window of at least 360 degrees.")
            .arg(phaseSpan).arg(boundaries->phaseRange().min).arg(boundaries->phaseRange().max));
    }

    //The algorithms own themselves through unique_ptr: solve()
    //throws on an invalid or infeasible problem, and the raw new/delete
    //pair leaked the whole algorithm (its lists, its detection, its
    //nominal-plant caches) on every such throw.
    auto timer = std::chrono::steady_clock::now();
    bool solved = false;

    //What the run cost is read from the algorithm's own counters and kept
    //with the result, where the interface and the benchmarks read it; the
    //peak live-node count is what the ceiling of kDefaultMaxLiveNodes has
    //to be tuned against.
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
        timer = std::chrono::steady_clock::now();
        solved = mc2->solve();
        if (solved) {
            report(mc2->controllerStructure(), mc2->statistics());
        }
    }

    return solved;
}

std::unique_ptr<LtiSystem> LoopShaping::controllerStructure()
{
    return std::move(m_controller);
}

} // namespace qftbx
