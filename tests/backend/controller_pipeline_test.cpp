// End-to-end validation of the PFC workflow through Controlador, the
// mediator the GUI drives: load a full project, recompute the boundaries
// with the same grid the file was made with, compare them against the
// stored ones, then save and reload the whole project. This covers the DAO
// wiring and orchestration that the per-module golden tests bypass; the
// interactive walk through the dialogs stays a manual check.

#include <gtest/gtest.h>

#include <QString>
#include <QTemporaryDir>
#include <QVector>

#include "Modelo/controlador.h"
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

GridPoint currentToGrid(QPointF p)
{
    return {static_cast<int>(std::lround(p.x() + 360.0)),
            static_cast<int>(std::lround(p.y() + 60.0))};
}

GridPoint goldenToGrid(QPointF p)
{
    return {static_cast<int>(std::lround((p.x() + 361.0) * 360.0 / 361.0)),
            static_cast<int>(std::lround((p.y() + 60.0) * 120.0 / 121.0))};
}

TEST(ControllerPipeline, RecomputedBoundariesMatchTheLoadedProject)
{
    Controlador controller;

    QVector<bool>* flags = controller.cargarSistema(fixture("multivaluados.qft"));
    ASSERT_EQ(flags->size(), 8);
    EXPECT_TRUE(flags->at(0)); // plant
    EXPECT_TRUE(flags->at(1)); // specifications
    EXPECT_TRUE(flags->at(2)); // frequencies
    EXPECT_TRUE(flags->at(3)); // templates
    EXPECT_TRUE(flags->at(4)); // boundaries
    delete flags;

    // Keep the traces of the boundaries as loaded from the file: getBound()
    // hands a view whose containers are replaced when recomputing.
    BoundaryData* loaded = controller.getBound();
    QVector<QVector<QVector<QPointF>>> storedTraces;
    for (auto* map : *loaded->boundaries()) {
        QVector<QVector<QPointF>> perFrequency;
        foreach (const QString& key, map->keys()) {
            for (QVector<QPointF>* trace : *map->value(key)) {
                perFrequency.append(*trace);
            }
        }
        storedTraces.append(perFrequency);
    }
    ASSERT_EQ(storedTraces.size(), 5);

    // Recompute through the same call the GUI makes, with the grid the
    // fixture was generated on (contour input, no CUDA).
    ASSERT_TRUE(controller.calcularBoundaries(QPointF(-360.0, 0.0), 361,
                                              QPointF(-60.0, 60.0), 121,
                                              -1.0, true, false));

    BoundaryData* recomputed = controller.getBound();
    ASSERT_NE(recomputed, nullptr);
    ASSERT_EQ(recomputed->boundaries()->size(), 5);

    for (int f = 0; f < 5; ++f) {
        auto* map = recomputed->boundaries()->at(f);
        ASSERT_EQ(map->size(), 1) << "frequency " << f;
        auto* traces = map->value(QStringLiteral("Tracking"));
        ASSERT_NE(traces, nullptr) << "frequency " << f;
        ASSERT_EQ(traces->size(), storedTraces.at(f).size()) << "frequency " << f;

        for (int t = 0; t < traces->size(); ++t) {
            const QVector<QPointF>& gold = storedTraces.at(f).at(t);
            QVector<QPointF>* got = traces->at(t);
            ASSERT_EQ(got->size(), gold.size()) << "frequency " << f << " trace " << t;

            // Current layout: [synthetic, core..., synthetic]; the legacy
            // file: [synthetic(last+1), synthetic(first-1), core...].
            for (int k = 0; k < got->size() - 2; ++k) {
                const GridPoint a = currentToGrid(got->at(1 + k));
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
        Controlador controller;
        delete controller.cargarSistema(fixture("planta1.qft"));
        ASSERT_TRUE(controller.guardarSistema(saved));
    }

    // The saved v2 file must carry the same project as the legacy original.
    ProjectReader original;
    QVector<bool>* originalFlags = original.load(fixture("planta1.qft"));
    ProjectReader rewritten;
    QVector<bool>* rewrittenFlags = rewritten.load(saved);

    ASSERT_EQ(*originalFlags, *rewrittenFlags);
    delete originalFlags;
    delete rewrittenFlags;

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
