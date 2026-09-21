/**
 * @file
 * @brief Golden tests of the boundary computation on `multivaluados.qft`.
 *
 * The fixture ships the boundaries of a tracking specification at five
 * frequencies computed by the original sequential build, whose grid mapping
 * scales by point count rather than range and stores its two synthetic
 * endpoints first; comparisons therefore run in grid-index space, where the
 * equality is exact. Further cases check that the union is the concatenation
 * of the traces with sorted, deduplicated phase buckets, that contours and
 * full clouds give the same raw sweep, that the guard near the singular locus
 * moves exactly one trace point at 5 rad/s, that the critical Nichols point
 * violates every finite specification, and that an undamped resonance swept
 * exactly at its frequency is reported with the frequency and the cause.
 */

#include <gtest/gtest.h>

#include <string>

#include <algorithm>
#include <memory>

#include "src/core/math/sequences.h"

#include <vector>

#include "src/core/math/point.h"
#include "src/core/math/range.h"

#include "src/core/boundaries/boundary_engine.h"
#include "src/core/boundaries/boundary_data.h"
#include "src/core/frequencies/omega.h"
#include "src/persistence/project_reader.h"
#include "src/core/system/free_form.h"
#include "src/core/common/exception.h"
#include "src/app/project_controller.h"

using namespace qftbx;

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

        engine.setSingularLocusGuard(false);
        engine.compute(parser.omega()->values(), parser.plant(),
                             parser.contour(), true, parser.specifications(),
                             qftbx::Range(-360.0, 0.0), 361, qftbx::Range(-60.0, 60.0), 121,
                             -1.0, false);

        got = engine.boundaryData();
        gold = parser.boundaries();
        ASSERT_NE(gold, nullptr);
    }

    ProjectReader parser;
    BoundaryEngine engine;
    std::optional<BoundaryData> got;
    const BoundaryData* gold = nullptr;
};

TEST_F(BoundariesGolden, GridMetadataMatches)
{
    EXPECT_EQ(got->phaseCount(), gold->phaseCount());
    EXPECT_EQ(got->magnitudeCount(), gold->magnitudeCount());
    EXPECT_EQ(got->phaseRange(), gold->phaseRange());
    EXPECT_EQ(got->magnitudeRange(), gold->magnitudeRange());

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
    ProjectReader parser2;
    parser2.load(
        std::string(QFTBX_TEST_DATA_DIR "/multivaluados.qft"));

    BoundaryEngine engine2;
    engine2.setSingularLocusGuard(false);
    engine2.compute(parser2.omega()->values(), parser2.plant(),
                          parser2.templates(), false, parser2.specifications(),
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

TEST(BoundariesGuard, GuardedSweepDeviatesOnlyNextToTheSingularLocus)
{
    ProjectReader parser;
    parser.load(std::string(QFTBX_TEST_DATA_DIR "/multivaluados.qft"));

    BoundaryEngine raw, guarded;
    raw.setSingularLocusGuard(false);
    for (BoundaryEngine * engine : {&raw, &guarded}) {
        engine->compute(parser.omega()->values(), parser.plant(), parser.contour(), true,
                        parser.specifications(), qftbx::Range(-360.0, 0.0), 361,
                        qftbx::Range(-60.0, 60.0), 121, -1.0, false);
    }
    const BoundaryData a = raw.boundaryData();
    const BoundaryData b = guarded.boundaryData();

    std::size_t differing = 0;
    for (std::size_t f = 0; f < 5; ++f) {
        const qftbx::TraceSet & ta = a.boundaries().at(f).at(std::string("Tracking"));
        const qftbx::TraceSet & tb = b.boundaries().at(f).at(std::string("Tracking"));
        std::size_t pa = 0, pb = 0;
        for (const qftbx::Trace & t : ta) pa += t.size();
        for (const qftbx::Trace & t : tb) pb += t.size();
        if (ta != tb) {
            ++differing;
            EXPECT_EQ(f, 2u) << "only the boundary at 5 rad/s is expected to move";
            EXPECT_EQ(pb + 1, pa) << "by one trace point fewer";
        }
    }
    EXPECT_EQ(differing, 1u);
}

TEST(BoundaryCriticalPoint, CriticalCellViolatesEverySpecification)
{
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

        EXPECT_GT(20.0 * std::log10(worst), 200.0)
            << "frequency " << omega->at(i);
    }
}

TEST(BoundaryCriticalPoint, NanSheetValueWouldReadAsAllowed)
{
    const double threshold = 1.75;
    EXPECT_FALSE(std::nan("") > threshold);
    EXPECT_TRUE(std::numeric_limits<double>::infinity() > threshold);
}

TEST(BoundaryCriticalPoint, UndampedResonanceIsRejectedWithAdvice)
{
    ProjectController controller;
    controller.load(
        std::string(QFTBX_TEST_DATA_DIR "/acc90.qft"));

    std::vector<Parameter> numerator;
    numerator.push_back(Parameter(std::string("ev"), Range(0.5, 2.0), 1.0,
                                    std::string("ev")));
    std::vector<Parameter> denominator;
    denominator.push_back(Parameter(std::string("ev"), Range(0.5, 2.0), 1.0,
                                      std::string("ev")));

    auto undamped = std::make_unique<qftbx::FreeForm>(
        std::string("undamped"), numerator, denominator,
        Parameter(1.0), Parameter(0.0),
        std::string("ev"), std::string("s^2*(s^2 + 2*ev)"));

    controller.setPlant(std::move(undamped));

    const std::vector<double> frequencies{1.0};
    controller.setOmega(std::make_unique<Omega>(frequencies.at(0), frequencies.at(0), 1,
                                               frequencies, Omega::Manual));

    const std::vector<double> epsilon{10.0};
    qftbx::ParameterGrids grids{{std::string("ev"), {0.5}}};

    try {
        controller.computeTemplates(epsilon, grids, false);
        FAIL() << "an undamped resonance must be reported, not swept under";
    } catch (const qftbx::Exception & error) {
        const std::string message = std::string(error.what());
        EXPECT_TRUE(message.find("1 rad/s") != std::string::npos) << error.what();
        EXPECT_TRUE(message.find("resonance") != std::string::npos) << error.what();
    }

}

}
