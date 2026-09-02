// End-to-end validation of the PFC workflow through ProjectController, the
// mediator the GUI drives: load a full project, recompute the boundaries
// with the same grid the file was made with, compare them against the
// stored ones, then save and reload the whole project. This covers the DAO
// wiring and orchestration that the per-module golden tests bypass; the
// interactive walk through the dialogs stays a manual check.

#include <gtest/gtest.h>

#include "src/core/point.h"

#include "src/core/range.h"

#include <QString>
#include <QTemporaryDir>
#include <QVector>

#include "src/core/project_controller.h"
#include "project_compare.h"
#include "src/persistence/project_reader.h"

namespace {

using namespace qftbx_tests;

QString fixture(const char* name)
{
    return QStringLiteral(QFTBX_TEST_DATA_DIR "/") + QLatin1String(name);
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

GridPoint currentToGrid(qftbx::Point p)
{
    return {static_cast<int>(std::lround(p.x + 360.0)),
            static_cast<int>(std::lround(p.y + 60.0))};
}

GridPoint goldenToGrid(qftbx::Point p)
{
    return {static_cast<int>(std::lround((p.x + 361.0) * 360.0 / 361.0)),
            static_cast<int>(std::lround((p.y + 60.0) * 120.0 / 121.0))};
}

TEST(ControllerPipeline, RecomputedBoundariesMatchTheLoadedProject)
{
    ProjectController controller;

    const std::vector<bool> flags = controller.load(fixture("multivaluados.qft"));
    ASSERT_EQ(static_cast<int>(flags.size()), 8);
    EXPECT_TRUE(flags.at(0)); // plant
    EXPECT_TRUE(flags.at(1)); // specifications
    EXPECT_TRUE(flags.at(2)); // frequencies
    EXPECT_TRUE(flags.at(3)); // templates
    EXPECT_TRUE(flags.at(4)); // boundaries

    // Keep the traces of the boundaries as loaded from the file: boundaries()
    // hands a view whose containers are replaced when recomputing.
    BoundaryData* loaded = controller.boundaries();
    QVector<QVector<QVector<qftbx::Point>>> storedTraces;
    for (const auto & map : loaded->boundaries()) {
        QVector<QVector<qftbx::Point>> perFrequency;
        for (const auto & entry : map) {
            for (const qftbx::Trace & trace : entry.second) {
                perFrequency.append(QVector<qftbx::Point>(trace.begin(), trace.end()));
            }
        }
        storedTraces.append(perFrequency);
    }
    ASSERT_EQ(storedTraces.size(), 5);

    // Recompute through the same call the GUI makes, with the grid the
    // fixture was generated on (contour input, no CUDA).
    ASSERT_TRUE(controller.computeBoundaries(qftbx::Range(-360.0, 0.0), 361,
                                              qftbx::Range(-60.0, 60.0), 121,
                                              -1.0, true, false));

    BoundaryData* recomputed = controller.boundaries();
    ASSERT_NE(recomputed, nullptr);
    ASSERT_EQ(recomputed->boundaries().size(), 5u);

    for (int f = 0; f < 5; ++f) {
        const auto & map = recomputed->boundaries().at(static_cast<std::size_t>(f));
        ASSERT_EQ(map.size(), 1u) << "frequency " << f;
        const auto foundTraces = map.find(QStringLiteral("Tracking"));
        ASSERT_NE(foundTraces, map.end()) << "frequency " << f;
        const qftbx::TraceSet & traces = foundTraces->second;
        ASSERT_EQ(static_cast<int>(traces.size()), storedTraces.at(f).size()) << "frequency " << f;

        for (int t = 0; t < static_cast<int>(traces.size()); ++t) {
            const QVector<qftbx::Point>& gold = storedTraces.at(f).at(t);
            const qftbx::Trace & got = traces.at(static_cast<std::size_t>(t));
            ASSERT_EQ(static_cast<int>(got.size()), gold.size()) << "frequency " << f << " trace " << t;

            // Current layout: [synthetic, core..., synthetic]; the legacy
            // file: [synthetic(last+1), synthetic(first-1), core...].
            for (int k = 0; k < static_cast<int>(got.size()) - 2; ++k) {
                const GridPoint a = currentToGrid(got.at(static_cast<std::size_t>(1 + k)));
                const GridPoint b = goldenToGrid(gold.at(2 + k));
                ASSERT_TRUE(a == b) << "frequency " << f << " trace " << t
                                    << " point " << k;
            }
        }
    }
}

TEST(ControllerPipeline, SaveAndReloadRoundTripsTheProject)
{
    QTemporaryDir temporary;
    ASSERT_TRUE(temporary.isValid());
    const QString saved = temporary.filePath(QStringLiteral("pipeline.qft"));

    {
        ProjectController controller;
        controller.load(fixture("planta1.qft"));
        ASSERT_TRUE(controller.save(saved));
    }

    // The saved v2 file must carry the same project as the legacy original.
    ProjectReader original;
    const std::vector<bool> originalFlags = original.load(fixture("planta1.qft"));
    ProjectReader rewritten;
    const std::vector<bool> rewrittenFlags = rewritten.load(saved);

    ASSERT_EQ(originalFlags, rewrittenFlags);

    const QVector<qreal> probes = *original.omega()->values();
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
