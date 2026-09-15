// End-to-end validation of the PFC workflow through ProjectController, the
// mediator the GUI drives: load a full project, recompute the boundaries
// with the same grid the file was made with, compare them against the
// stored ones, then save and reload the whole project. This covers the DAO
// wiring and orchestration that the per-module golden tests bypass; the
// interactive walk through the dialogs stays a manual check.

#include <gtest/gtest.h>
#include <cstdio>
#include <utility>
#include <set>
#include <iterator>
#include <algorithm>

#include <string>

#include <vector>

#include "src/core/math/point.h"

#include "src/core/math/range.h"

#include <QTemporaryDir>

#include "src/app/project_controller.h"
#include "project_compare.h"
#include "src/persistence/project_reader.h"

using namespace qftbx;

namespace {

using namespace qftbx_tests;

std::string fixture(const char* name)
{
    return std::string(QFTBX_TEST_DATA_DIR "/") + name;
}

// Grid-index comparison against the legacy fixture mapping, as in the
// boundary golden tests (the file stores x = n*361/360 - 361 and inverted
// synthetic endpoints; the current engine stores x = n - 360).
struct GridPoint
{
    int n;
    int m;
    bool operator==(const GridPoint& o) const { return n == o.n && m == o.m; }
};

GridPoint currentToGrid(qftbx::NicholsPoint p)
{
    return {static_cast<int>(std::lround(p.phase + 360.0)),
            static_cast<int>(std::lround(p.magnitude + 60.0))};
}

GridPoint goldenToGrid(qftbx::NicholsPoint p)
{
    return {static_cast<int>(std::lround((p.phase + 361.0) * 360.0 / 361.0)),
            static_cast<int>(std::lround((p.magnitude + 60.0) * 120.0 / 121.0))};
}

TEST(ControllerPipeline, RecomputedBoundariesMatchTheLoadedProject)
{
    ProjectController controller;

    const qftbx::StepSet sections = controller.load(fixture("multivaluados.qft"));
    EXPECT_TRUE(sections.has(qftbx::Step::Plant));
    EXPECT_TRUE(sections.has(qftbx::Step::Specifications));
    EXPECT_TRUE(sections.has(qftbx::Step::Frequencies));
    EXPECT_TRUE(sections.has(qftbx::Step::Templates));
    EXPECT_TRUE(sections.has(qftbx::Step::Boundaries));

    //And what a load says it read is what the project says it holds.
    EXPECT_EQ(controller.completed(), sections);

    // Keep the traces of the boundaries as loaded from the file: boundaries()
    // hands a view whose containers are replaced when recomputing.
    BoundaryData* loaded = controller.boundaries();
    std::vector<std::vector<std::vector<qftbx::NicholsPoint>>> storedTraces;
    for (const auto & map : loaded->boundaries()) {
        std::vector<std::vector<qftbx::NicholsPoint>> perFrequency;
        for (const auto & entry : map) {
            for (const qftbx::Trace & trace : entry.second) {
                perFrequency.push_back(std::vector<qftbx::NicholsPoint>(trace.begin(), trace.end()));
            }
        }
        storedTraces.push_back(perFrequency);
    }
    ASSERT_EQ(storedTraces.size(), 5);

    // Recompute through the same call the GUI makes, with the grid the
    // fixture was generated on (contour input, no CUDA). The engine now
    // guards its sample near the singular locus (SingularLocus), which the
    // fixture's raw sweep did not: at 5 rad/s a cell next to the locus turns
    // forbidden and the boundary there is re-routed around it - a few trace
    // points leave, a few appear, one fewer in all. Everything else must
    // match the file point for point, and every point that moved must be
    // where the locus is: next to -180 degrees, around 0 dB.
    ASSERT_TRUE(controller.computeBoundaries(qftbx::Range(-360.0, 0.0), 361,
                                              qftbx::Range(-60.0, 60.0), 121,
                                              -1.0, true, false));

    BoundaryData* recomputed = controller.boundaries();
    ASSERT_NE(recomputed, nullptr);
    ASSERT_EQ(recomputed->boundaries().size(), 5u);

    for (int f = 0; f < 5; ++f) {
        const auto & map = recomputed->boundaries().at(static_cast<std::size_t>(f));
        ASSERT_EQ(map.size(), 1u) << "frequency " << f;
        const auto foundTraces = map.find(std::string("Tracking"));
        ASSERT_NE(foundTraces, map.end()) << "frequency " << f;
        const qftbx::TraceSet & traces = foundTraces->second;
        ASSERT_EQ(static_cast<int>(traces.size()), storedTraces.at(f).size()) << "frequency " << f;

        for (int t = 0; t < static_cast<int>(traces.size()); ++t) {
            const std::vector<qftbx::NicholsPoint>& gold = storedTraces.at(f).at(t);
            const qftbx::Trace & got = traces.at(static_cast<std::size_t>(t));

            // Current layout: [synthetic, core..., synthetic]; the legacy
            // file: [synthetic(last+1), synthetic(first-1), core...]. The
            // core points, as sets in grid indices.
            std::set<std::pair<int, int>> stored, recomputed;
            for (std::size_t k = 2; k < gold.size(); ++k) {
                const GridPoint g = goldenToGrid(gold.at(k));
                stored.insert({g.n, g.m});
            }
            for (std::size_t k = 1; k + 1 < got.size(); ++k) {
                const GridPoint g = currentToGrid(got.at(k));
                recomputed.insert({g.n, g.m});
            }

            std::vector<std::pair<int, int>> missing, extra;
            std::set_difference(stored.begin(), stored.end(), recomputed.begin(), recomputed.end(),
                                std::back_inserter(missing));
            std::set_difference(recomputed.begin(), recomputed.end(), stored.begin(), stored.end(),
                                std::back_inserter(extra));

            if (f == 2 && t == 0) {
                EXPECT_EQ(got.size() + 1, gold.size()) << "one point fewer in all";
                EXPECT_FALSE(missing.empty());
                //Grid index n is the phase from -360, m the magnitude from -60:
                //the locus of this frequency sits next to n = 180, m = 60.
                for (const auto & moved : {missing, extra}) {
                    for (const std::pair<int, int> & p : moved) {
                        std::printf("GUARD-MOVED f=2 n=%d m=%d (phase %d deg, %d dB)\n",
                                    p.first, p.second, p.first - 360, p.second - 60);
                        EXPECT_NEAR(p.first, 180, 30) << "phase index, near -180 degrees";
                        EXPECT_NEAR(p.second, 60, 15) << "magnitude index, near 0 dB";
                    }
                }
                std::fflush(stdout);
            } else {
                EXPECT_TRUE(missing.empty() && extra.empty())
                        << "frequency " << f << " trace " << t << " differs from the file";
            }
        }
    }
}

TEST(ControllerPipeline, SaveAndReloadRoundTripsTheProject)
{
    QTemporaryDir temporary;
    ASSERT_TRUE(temporary.isValid());
    const std::string saved = temporary.filePath("pipeline.qft").toStdString();

    {
        ProjectController controller;
        controller.load(fixture("planta1.qft"));
        controller.save(saved);
    }

    // The saved v2 file must carry the same project as the legacy original.
    ProjectReader original;
    const ProjectReader::Loaded originalSections = original.load(fixture("planta1.qft"));
    ProjectReader rewritten;
    const ProjectReader::Loaded rewrittenSections = rewritten.load(saved);

    ASSERT_EQ(originalSections.steps, rewrittenSections.steps);
    ASSERT_EQ(originalSections.hasContour, rewrittenSections.hasContour);

    const std::vector<double> probes = *original.omega()->values();
    expectSameSystem(original.plant(), rewritten.plant(), probes, "plant");
    expectSameSpecifications(original.specifications(), rewritten.specifications());
    EXPECT_EQ(*original.epsilon(), *rewritten.epsilon());
    expectSameComplexVectors(original.templates(), rewritten.templates(), "templates");
    expectSameComplexVectors(original.contour(), rewritten.contour(), "contour");
    expectSameBoundaries(original.boundaries(), rewritten.boundaries());
    expectSameSystem(original.controller(), rewritten.controller(), probes, "controller");
    expectSameLoopShaping(original.loopShaping(), rewritten.loopShaping());
}

} // namespace
