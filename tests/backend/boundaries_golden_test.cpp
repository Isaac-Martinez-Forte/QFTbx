// Golden tests for the boundary computation against tests/data/
// multivaluados.qft, which ships the boundaries computed by the original
// (sequential, dB) build for 5 frequencies of a tracking specification.
//
// Key note: the fixture stores the legacy Spanish map keys ("Seguimiento");
// the loader maps them to the canonical English ones ("Tracking").
//
// Coordinate note: the fixture was produced by a legacy mapping that scaled
// by the POINT COUNT instead of the range (x = n*361/360 - 361,
// y = m*121/120 - 60) and prepended its two synthetic endpoints in inverted
// order; the current code maps correctly (x = n - 360, y = m - 60) and
// appends first-1 / last+1 around the core. Comparisons therefore run in
// GRID-INDEX space, where the equality is exact and integer.

#include <gtest/gtest.h>

#include <memory>

#include "src/core/math/sequences.h"

#include <vector>

#include <QMap>
#include <QPointF>
#include "src/core/range.h"
#include <QString>
#include <QVector>

#include "src/core/boundaries/boundary_engine.h"
#include "src/core/boundaries/boundary_data.h"
#include "src/core/frequencies/omega.h"
#include "src/persistence/project_reader.h"
#include "src/core/system/free_form.h"
#include "src/core/exception.h"
#include "src/core/project_controller.h"

namespace {

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

class BoundariesGolden : public ::testing::Test
{
protected:
    void SetUp() override
    {
        parser.load(
            QStringLiteral(QFTBX_TEST_DATA_DIR "/multivaluados.qft"));

        engine.compute(parser.omega()->values(), parser.plant(),
                             parser.contour(), parser.specifications(),
                             QPointF(-360.0, 0.0), 361, QPointF(-60.0, 60.0), 121,
                             -1.0, false);

        got = engine.boundaryData();
        gold = parser.boundaries();
        ASSERT_NE(got, nullptr);
        ASSERT_NE(gold, nullptr);
    }

    ProjectReader parser;
    BoundaryEngine engine;
    //boundaryData() hands out a NON-owning view, freshly allocated each call:
    //the engine keeps the data. The view itself is the caller's, and every
    //one of these tests used to drop it on the floor.
    void TearDown() override
    {
        delete got;
    }

    BoundaryData* got = nullptr;
    BoundaryData* gold = nullptr;   //owned by the reader
};

TEST_F(BoundariesGolden, GridMetadataMatches)
{
    EXPECT_EQ(got->phaseCount(), gold->phaseCount());   // 361
    EXPECT_EQ(got->magnitudeCount(), gold->magnitudeCount());   // 121
    EXPECT_EQ(got->phaseRange(), gold->phaseRange()); // (-360, 0)
    EXPECT_EQ(got->magnitudeRange(), gold->magnitudeRange()); // (-60, 60)

    ASSERT_NE(got->openFlags(), nullptr);
    ASSERT_EQ(got->openFlags()->size(), 5);
    for (int f = 0; f < 5; ++f) {
        EXPECT_FALSE(got->openFlags()->at(f));
        EXPECT_FALSE(got->upperFlags()->at(f));
    }
}

TEST_F(BoundariesGolden, TracesMatchTheGoldenInGridIndices)
{
    auto* gotB = got->boundaries();
    auto* goldB = gold->boundaries();
    ASSERT_EQ(gotB->size(), 5);
    ASSERT_EQ(goldB->size(), 5);

    const int expectedTraces[] = {5, 1, 2, 4, 5};

    for (int f = 0; f < 5; ++f) {
        auto* gotMap = gotB->at(f);
        auto* goldMap = goldB->at(f);
        ASSERT_TRUE(gotMap->contains(QStringLiteral("Tracking")))
            << "frequency " << f;
        ASSERT_EQ(goldMap->size(), 1);

        auto* gotTraces = gotMap->value(QStringLiteral("Tracking"));
        auto* goldTraces = goldMap->first();
        ASSERT_EQ(goldTraces->size(), expectedTraces[f]) << "frequency " << f;
        ASSERT_EQ(gotTraces->size(), goldTraces->size()) << "frequency " << f;

        for (int t = 0; t < gotTraces->size(); ++t) {
            QVector<QPointF>* gotTrace = gotTraces->at(t);
            QVector<QPointF>* goldTrace = goldTraces->at(t);
            ASSERT_EQ(gotTrace->size(), goldTrace->size())
                << "frequency " << f << " trace " << t;

            // Core: current = [synthetic, core..., synthetic]; the legacy
            // golden = [synthetic(last+1), synthetic(first-1), core...].
            const int n = gotTrace->size();
            for (int k = 0; k < n - 2; ++k) {
                const GridPoint a = currentToGrid(gotTrace->at(1 + k));
                const GridPoint b = goldenToGrid(goldTrace->at(2 + k));
                ASSERT_TRUE(a == b)
                    << "frequency " << f << " trace " << t << " point " << k
                    << ": got grid (" << a.n << "," << a.m << ") vs golden ("
                    << b.n << "," << b.m << ")";
            }
        }
    }
}

TEST_F(BoundariesGolden, ReunionIsTheConcatenationOfTheTraces)
{
    auto* reun = got->unionBoundaries();
    ASSERT_NE(reun, nullptr);
    ASSERT_EQ(reun->size(), 5);

    auto* gotB = got->boundaries();
    for (int f = 0; f < 5; ++f) {
        auto* traces = gotB->at(f)->value(QStringLiteral("Tracking"));
        int total = 0;
        for (QVector<QPointF>* t : *traces) {
            total += t->size();
        }
        ASSERT_EQ(reun->at(f)->size(), total) << "frequency " << f;

        int idx = 0;
        for (QVector<QPointF>* t : *traces) {
            for (const QPointF& p : *t) {
                ASSERT_EQ(reun->at(f)->at(idx), p)
                    << "frequency " << f << " flat index " << idx;
                ++idx;
            }
        }
    }
}

TEST_F(BoundariesGolden, ContourInputIsEquivalentToFullTemplates)
{
    // The sheet is a max/min over the cloud, so feeding the full clouds
    // instead of the contours must give the same boundaries.
    ProjectReader parser2;
    parser2.load(
        QStringLiteral(QFTBX_TEST_DATA_DIR "/multivaluados.qft"));

    BoundaryEngine engine2;
    engine2.compute(parser2.omega()->values(), parser2.plant(),
                          parser2.templates(), parser2.specifications(),
                          QPointF(-360.0, 0.0), 361, QPointF(-60.0, 60.0), 121,
                          -1.0, false);
    const std::unique_ptr<BoundaryData> other(engine2.boundaryData());

    auto* a = got->boundaries();
    auto* b = other->boundaries();
    ASSERT_EQ(a->size(), b->size());
    for (int f = 0; f < a->size(); ++f) {
        auto* ta = a->at(f)->value(QStringLiteral("Tracking"));
        auto* tb = b->at(f)->value(QStringLiteral("Tracking"));
        ASSERT_EQ(ta->size(), tb->size()) << "frequency " << f;
        for (int t = 0; t < ta->size(); ++t) {
            ASSERT_EQ(*ta->at(t), *tb->at(t)) << "frequency " << f << " trace " << t;
        }
    }
}

TEST_F(BoundariesGolden, ReunionHashIsSortedDeduplicatedAndInRange)
{
    // Fixed (C7): the per-phase buckets are built from ALL reunion points
    // (the first one used to be skipped), the border bucket no longer
    // indexes out of range, and each bucket is sorted ascending by
    // magnitude with duplicate magnitudes dropped - the semantics of the
    // layer buckets and of the historical files.
    auto* hash = got->unionBuckets();
    ASSERT_NE(hash, nullptr);
    ASSERT_EQ(hash->size(), 5);

    auto* reun = got->unionBoundaries();

    for (int f = 0; f < 5; ++f) {
        ASSERT_EQ(hash->at(f)->size(), 361) << "frequency " << f;

        int total = 0;
        for (QVector<QPointF>* bucket : *hash->at(f)) {
            for (int k = 0; k < bucket->size(); ++k) {
                EXPECT_TRUE(reun->at(f)->contains(bucket->at(k)))
                    << "frequency " << f;
                if (k > 0) {
                    EXPECT_GT(bucket->at(k).y(), bucket->at(k - 1).y())
                        << "frequency " << f << " bucket not strictly sorted";
                }
            }
            total += bucket->size();
        }
        EXPECT_GT(total, 0) << "frequency " << f;
        EXPECT_LE(total, reun->at(f)->size()) << "frequency " << f;
    }
}

// ---------------------------------------------------------------------------
// The critical Nichols point and non-finite plant magnitudes (decision D8)
// ---------------------------------------------------------------------------

TEST(BoundaryCriticalPoint, CriticalCellViolatesEverySpecification)
{
    // At the grid point L = -1 (phase -180 deg, magnitude 0 dB) the nominal
    // plant makes the closed-loop denominator vanish: the loop has a pole on
    // the imaginary axis, so |T| is unbounded and the cell must violate any
    // finite specification. The exact zero never materialises because
    // sin(-pi) is -1.2e-16 instead of 0, which is why the cell reads as a
    // huge but finite value; the engine no longer depends on that accident,
    // as the tracking case below shows.
    ProjectController controller;
    controller.load(
        QStringLiteral(QFTBX_TEST_DATA_DIR "/acc90.qft"));

    LtiSystem* plant = controller.plant();
    QVector<qreal>* omega = controller.omega()->values();
    const qftbx::CloudSet & templates = controller.templates();

    const std::complex<qreal> L(std::pow(10.0, 0.0 / 20.0) * std::cos(-180.0 * M_PI / 180.0),
                                std::pow(10.0, 0.0 / 20.0) * std::sin(-180.0 * M_PI / 180.0));

    for (int i = 0; i < omega->size(); ++i) {
        const std::complex<qreal> p0 = plant->evaluate(omega->at(i));

        qreal worst = -std::numeric_limits<qreal>::infinity();
        for (const std::complex<qreal>& p : templates.at(static_cast<std::size_t>(i))) {
            worst = std::max(worst, std::abs(L / ((p0 / p) + L)));
        }

        // Way above any sane robust-stability threshold (gamma = 1.75 here).
        EXPECT_GT(20.0 * std::log10(worst), 200.0)
            << "frequency " << omega->at(i);
    }
}

TEST(BoundaryCriticalPoint, NanSheetValueWouldReadAsAllowed)
{
    // Why the engine states non-finite cells as violating: a NaN compares
    // FALSE against the threshold, so a NaN cell would silently read as
    // ALLOWED, while an infinity reads as forbidden, which is correct.
    const qreal threshold = 1.75;
    EXPECT_FALSE(std::nan("") > threshold);
    EXPECT_TRUE(std::numeric_limits<qreal>::infinity() > threshold);
}

TEST(BoundaryCriticalPoint, UndampedResonanceIsRejectedWithAdvice)
{
    // The ACC'90 plant without the light damping the literature prescribes:
    // P(s) = ev/(s^2 (s^2 + 2ev)) has poles at +-j*sqrt(2ev), so with
    // ev in [0.5, 2] the resonance sweeps [1, 2] rad/s and SOME plant of the
    // value set blows up at every frequency of that band - no frequency grid
    // can dodge it, which is why the literature damps the poles instead.
    //
    // The magnitude comes out astronomical rather than infinite (muParserX
    // evaluates (1i)^2 as -1 + 1.2e-16i, so the resonant denominator never
    // hits an exact zero), and no epsilon walks a cloud of that span: the
    // engine reports the frequency, the largest magnitude found and the
    // likely cause instead of the bare "could not compute" it used to give.
    ProjectController controller;
    controller.load(
        QStringLiteral(QFTBX_TEST_DATA_DIR "/acc90.qft"));

    // Undamped version of the fixture's plant, swept exactly at a resonance.
    // The parameter is named 'ev' as in the fixture: 'e' is Euler's number
    // in muParserX.
    std::vector<Parameter> numerator;
    numerator.push_back(Parameter(QStringLiteral("ev"), Range(0.5, 2.0), 1.0,
                                    QStringLiteral("ev")));
    std::vector<Parameter> denominator;
    denominator.push_back(Parameter(QStringLiteral("ev"), Range(0.5, 2.0), 1.0,
                                      QStringLiteral("ev")));

    LtiSystem* undamped = new qftbx::FreeForm(
        QStringLiteral("undamped"), numerator, denominator,
        Parameter(1.0), Parameter(0.0),
        QStringLiteral("ev"), QStringLiteral("s^2*(s^2 + 2*ev)"));

    controller.setPlant(undamped);

    auto* frequencies = new QVector<qreal>();
    frequencies->append(1.0);              // exact resonance of ev = 0.5
    controller.setOmega(new Omega(frequencies->at(0), frequencies->at(0), 1,
                                   frequencies, Omega::Manual));

    auto* epsilon = new QVector<qreal>();
    epsilon->append(10.0);
    // sqrt(2*0.5) = 1 exactly: |P| = inf at that frequency
    qftbx::ParameterGrids grids{{QStringLiteral("ev"), {0.5}}};

    try {
        controller.computeTemplates(epsilon, grids, false);
        delete epsilon;
        FAIL() << "an undamped resonance must be reported, not swept under";
    } catch (const qftbx::ComputationError & error) {
        //A refusal never took the epsilon: the store only keeps it on success.
        delete epsilon;
        const QString message = QString::fromUtf8(error.what());
        EXPECT_TRUE(message.contains(QStringLiteral("1 rad/s"))) << error.what();
        EXPECT_TRUE(message.contains(QStringLiteral("largest |P|"))) << error.what();
        EXPECT_TRUE(message.contains(QStringLiteral("resonance"))) << error.what();
    }

}

} // namespace
