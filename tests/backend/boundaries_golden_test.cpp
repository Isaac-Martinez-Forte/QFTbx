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

#include <string>

#include <algorithm>
#include <memory>

#include "src/core/math/sequences.h"

#include <vector>

#include "src/core/point.h"
#include "src/core/range.h"

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

class BoundariesGolden : public ::testing::Test
{
protected:
    void SetUp() override
    {
        parser.load(
            std::string(QFTBX_TEST_DATA_DIR "/multivaluados.qft"));

        engine.compute(parser.omega()->values(), parser.plant(),
                             parser.contour(), parser.specifications(),
                             qftbx::Range(-360.0, 0.0), 361, qftbx::Range(-60.0, 60.0), 121,
                             -1.0, false);

        got = engine.boundaryData();
        gold = parser.boundaries();
        ASSERT_NE(gold, nullptr);
    }

    ProjectReader parser;
    BoundaryEngine engine;
    //boundaryData() returns a snapshot BY VALUE now: no allocation, no
    //TearDown, and no chance of dropping it on the floor. It used to be a
    //freshly allocated non-owning view that every one of these tests leaked.
    std::optional<BoundaryData> got;
    const BoundaryData* gold = nullptr;   //owned by the reader
};

TEST_F(BoundariesGolden, GridMetadataMatches)
{
    EXPECT_EQ(got->phaseCount(), gold->phaseCount());   // 361
    EXPECT_EQ(got->magnitudeCount(), gold->magnitudeCount());   // 121
    EXPECT_EQ(got->phaseRange(), gold->phaseRange()); // (-360, 0)
    EXPECT_EQ(got->magnitudeRange(), gold->magnitudeRange()); // (-60, 60)

    ASSERT_EQ(got->openFlags().size(), 5u);
    for (std::size_t f = 0; f < 5; ++f) {
        EXPECT_FALSE(got->openFlags().at(f));
        EXPECT_FALSE(got->upperFlags().at(f));
    }
}

TEST_F(BoundariesGolden, TracesMatchTheGoldenInGridIndices)
{
    const qftbx::BoundarySet & gotB = got->boundaries();
    const qftbx::BoundarySet & goldB = gold->boundaries();
    ASSERT_EQ(gotB.size(), 5u);
    ASSERT_EQ(goldB.size(), 5u);

    const int expectedTraces[] = {5, 1, 2, 4, 5};

    for (int f = 0; f < 5; ++f) {
        const auto & gotMap = gotB.at(static_cast<std::size_t>(f));
        const auto & goldMap = goldB.at(static_cast<std::size_t>(f));
        const auto foundGot = gotMap.find(std::string("Tracking"));
        ASSERT_NE(foundGot, gotMap.end()) << "frequency " << f;
        ASSERT_EQ(goldMap.size(), 1u);

        const qftbx::TraceSet & gotTraces = foundGot->second;
        const qftbx::TraceSet & goldTraces = goldMap.begin()->second;
        ASSERT_EQ(static_cast<int>(goldTraces.size()), expectedTraces[f]) << "frequency " << f;
        ASSERT_EQ(gotTraces.size(), goldTraces.size()) << "frequency " << f;

        for (int t = 0; t < static_cast<int>(gotTraces.size()); ++t) {
            const qftbx::Trace & gotTrace = gotTraces.at(static_cast<std::size_t>(t));
            const qftbx::Trace & goldTrace = goldTraces.at(static_cast<std::size_t>(t));
            ASSERT_EQ(gotTrace.size(), goldTrace.size())
                << "frequency " << f << " trace " << t;

            // Core: current = [synthetic, core..., synthetic]; the legacy
            // golden = [synthetic(last+1), synthetic(first-1), core...].
            const int n = static_cast<int>(gotTrace.size());
            for (int k = 0; k < n - 2; ++k) {
                const GridPoint a = currentToGrid(gotTrace.at(1 + k));
                const GridPoint b = goldenToGrid(goldTrace.at(2 + k));
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
    const qftbx::UnionTraces & reun = got->unionBoundaries();
    ASSERT_EQ(reun.size(), 5u);

    const qftbx::BoundarySet & gotB = got->boundaries();
    for (std::size_t f = 0; f < 5; ++f) {
        const qftbx::TraceSet & traces = gotB.at(f).at(std::string("Tracking"));

        std::size_t total = 0;
        for (const qftbx::Trace & t : traces) {
            total += t.size();
        }
        ASSERT_EQ(reun.at(f).size(), total) << "frequency " << f;

        std::size_t idx = 0;
        for (const qftbx::Trace & t : traces) {
            for (const qftbx::NicholsPoint& p : t) {
                ASSERT_EQ(reun.at(f).at(idx), p)
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
        std::string(QFTBX_TEST_DATA_DIR "/multivaluados.qft"));

    BoundaryEngine engine2;
    engine2.compute(parser2.omega()->values(), parser2.plant(),
                          parser2.templates(), parser2.specifications(),
                          qftbx::Range(-360.0, 0.0), 361, qftbx::Range(-60.0, 60.0), 121,
                          -1.0, false);
    const BoundaryData other = engine2.boundaryData();

    const qftbx::BoundarySet & a = got->boundaries();
    const qftbx::BoundarySet & b = other.boundaries();
    ASSERT_EQ(a.size(), b.size());
    for (std::size_t f = 0; f < a.size(); ++f) {
        EXPECT_EQ(a.at(f).at(std::string("Tracking")),
                  b.at(f).at(std::string("Tracking"))) << "frequency " << f;
    }
}

TEST_F(BoundariesGolden, ReunionHashIsSortedDeduplicatedAndInRange)
{
    // Fixed (C7): the per-phase buckets are built from ALL reunion points
    // (the first one used to be skipped), the border bucket no longer
    // indexes out of range, and each bucket is sorted ascending by
    // magnitude with duplicate magnitudes dropped - the semantics of the
    // layer buckets and of the historical files.
    const qftbx::UnionBuckets & hash = got->unionBuckets();
    ASSERT_EQ(hash.size(), 5u);

    const qftbx::UnionTraces & reun = got->unionBoundaries();

    for (std::size_t f = 0; f < 5; ++f) {
        ASSERT_EQ(hash.at(f).size(), 361u) << "frequency " << f;

        std::size_t total = 0;
        for (const qftbx::Trace & bucket : hash.at(f)) {
            for (std::size_t k = 0; k < bucket.size(); ++k) {
                EXPECT_NE(std::find(reun.at(f).begin(), reun.at(f).end(), bucket.at(k)),
                          reun.at(f).end()) << "frequency " << f;
                if (k > 0) {
                    EXPECT_GT(bucket.at(k).magnitude, bucket.at(k - 1).magnitude)
                        << "frequency " << f << " bucket not strictly sorted";
                }
            }
            total += bucket.size();
        }
        EXPECT_GT(total, 0u) << "frequency " << f;
        EXPECT_LE(total, reun.at(f).size()) << "frequency " << f;
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
        std::string(QFTBX_TEST_DATA_DIR "/acc90.qft"));

    LtiSystem* plant = controller.plant();
    std::vector<double>* omega = controller.omega()->values();
    const qftbx::CloudSet & templates = controller.templates();

    const std::complex<double> L(std::pow(10.0, 0.0 / 20.0) * std::cos(-180.0 * M_PI / 180.0),
                                std::pow(10.0, 0.0 / 20.0) * std::sin(-180.0 * M_PI / 180.0));

    for (int i = 0; i < omega->size(); ++i) {
        const std::complex<double> p0 = plant->evaluate(omega->at(i));

        double worst = -std::numeric_limits<double>::infinity();
        for (const std::complex<double>& p : templates.at(static_cast<std::size_t>(i))) {
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
    const double threshold = 1.75;
    EXPECT_FALSE(std::nan("") > threshold);
    EXPECT_TRUE(std::numeric_limits<double>::infinity() > threshold);
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
        std::string(QFTBX_TEST_DATA_DIR "/acc90.qft"));

    // Undamped version of the fixture's plant, swept exactly at a resonance.
    // The parameter is named 'ev' as in the fixture: 'e' is Euler's number
    // in muParserX.
    std::vector<Parameter> numerator;
    numerator.push_back(Parameter(std::string("ev"), Range(0.5, 2.0), 1.0,
                                    std::string("ev")));
    std::vector<Parameter> denominator;
    denominator.push_back(Parameter(std::string("ev"), Range(0.5, 2.0), 1.0,
                                      std::string("ev")));

    LtiSystem* undamped = new qftbx::FreeForm(
        std::string("undamped"), numerator, denominator,
        Parameter(1.0), Parameter(0.0),
        std::string("ev"), std::string("s^2*(s^2 + 2*ev)"));

    controller.setPlant(std::unique_ptr<LtiSystem>(undamped));

    const std::vector<double> frequencies{1.0};   // exact resonance of ev = 0.5
    controller.setOmega(std::make_unique<Omega>(frequencies.at(0), frequencies.at(0), 1,
                                               frequencies, Omega::Manual));

    const std::vector<double> epsilon{10.0};
    // sqrt(2*0.5) = 1 exactly: |P| = inf at that frequency
    qftbx::ParameterGrids grids{{std::string("ev"), {0.5}}};

    try {
        controller.computeTemplates(epsilon, grids, false);
        FAIL() << "an undamped resonance must be reported, not swept under";
    } catch (const qftbx::ComputationError & error) {
        const std::string message = std::string(error.what());
        EXPECT_TRUE(message.find("1 rad/s") != std::string::npos) << error.what();
        EXPECT_TRUE(message.find("largest |P|") != std::string::npos) << error.what();
        EXPECT_TRUE(message.find("resonance") != std::string::npos) << error.what();
    }

}

} // namespace
