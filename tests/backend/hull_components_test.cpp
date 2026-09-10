// The epsilon-hull of a cloud that is not epsilon-connected.
//
// The walk of Prune looks only within epsilon of the current point, so it can
// never leave the component it started in; Prune is defined for an
// epsilon-connected set (Gutman, Nordin and Cohen 2007, section 3, "let now V
// be epsilon-connected"). Applied to a cloud of two blocks farther apart than
// epsilon, the engine used to return the contour of the seed's block alone -
// 17 of 50 points, no fallback flagged, reported as a valid contour - and the
// boundaries were then computed over a value set missing the other block,
// permissively. Whether that happened loudly or silently depended on where
// the seed fell: a seed on an isolated point found no second point and
// failed; a seed on a walkable block closed and lost the rest.
//
// The components are now found first (union-find over the neighbour grid)
// and each is walked; a single-component cloud takes exactly the historical
// path, which is what keeps every golden where it is.

#include <gtest/gtest.h>

#include <complex>
#include <set>
#include <vector>

#include "src/core/templates/template_engine.h"

using namespace qftbx;

namespace {

//A square block of points, spaced 'step', centred on 'centre'.
void addBlock(ComplexCloud & cloud, std::complex<double> centre, double step, int side)
{
    for (int i = 0; i < side; ++i) {
        for (int j = 0; j < side; ++j) {
            cloud.push_back(centre + std::complex<double>((i - side / 2) * step, (j - side / 2) * step));
        }
    }
}

std::size_t inBox(const ComplexCloud & cloud, std::complex<double> centre, double half)
{
    std::size_t n = 0;
    for (const std::complex<double> & p : cloud) {
        if (std::abs(p.real() - centre.real()) <= half && std::abs(p.imag() - centre.imag()) <= half) {
            ++n;
        }
    }
    return n;
}

} // namespace

TEST(HullComponents, TwoBlocksFartherApartThanEpsilonBothReachTheContour)
{
    TemplateEngine engine;

    //Two 5x5 blocks of unit spacing, 40 apart: within a block the nearest
    //neighbour is 1, between blocks about 36.
    ComplexCloud cloud;
    addBlock(cloud, {0.0, 0.0}, 1.0, 5);
    addBlock(cloud, {40.0, 0.0}, 1.0, 5);

    bool fellBack = false, truncated = false;
    std::vector<std::size_t> starts;
    const ComplexCloud contour = engine.epsilonHull(cloud, 1.5, &fellBack, &truncated, &starts);

    ASSERT_EQ(starts.size(), 2u) << "two epsilon-connected components";
    EXPECT_EQ(starts[0], 0u);
    EXPECT_GT(inBox(contour, {40.0, 0.0}, 10.0), 0u) << "the seed's block (rightmost) comes first";
    EXPECT_GT(inBox(contour, {0.0, 0.0}, 10.0), 0u) << "the other block used to be dropped";
    EXPECT_FALSE(truncated);

    //Each block's contour is its 16 border points, closed on the seed pair.
    const std::size_t first = starts[1];
    std::set<std::pair<double, double>> a, b;
    for (std::size_t i = 0; i < first; ++i) a.insert({contour[i].real(), contour[i].imag()});
    for (std::size_t i = first; i < contour.size(); ++i) b.insert({contour[i].real(), contour[i].imag()});
    EXPECT_EQ(a.size(), 16u);
    EXPECT_EQ(b.size(), 16u);
}

TEST(HullComponents, ThreeBlocksGiveThreeComponentsInRightmostOrder)
{
    TemplateEngine engine;

    ComplexCloud cloud;
    addBlock(cloud, {0.0, 0.0}, 1.0, 4);
    addBlock(cloud, {30.0, 0.0}, 1.0, 4);
    addBlock(cloud, {60.0, 25.0}, 1.0, 4);

    std::vector<std::size_t> starts;
    const ComplexCloud contour = engine.epsilonHull(cloud, 1.5, nullptr, nullptr, &starts);

    ASSERT_EQ(starts.size(), 3u);
    EXPECT_GT(inBox(contour, {0.0, 0.0}, 8.0), 0u);
    EXPECT_GT(inBox(contour, {30.0, 0.0}, 8.0), 0u);
    EXPECT_GT(inBox(contour, {60.0, 25.0}, 8.0), 0u);

    //Rightmost component first: the block at real 60, then 30, then 0.
    EXPECT_NEAR(contour[starts[0]].real(), 60.0, 3.0);
    EXPECT_NEAR(contour[starts[1]].real(), 30.0, 3.0);
    EXPECT_NEAR(contour[starts[2]].real(), 0.0, 3.0);
}

TEST(HullComponents, AnIsolatedPointIsKeptAsItsOwnComponent)
{
    TemplateEngine engine;

    ComplexCloud cloud;
    addBlock(cloud, {0.0, 0.0}, 1.0, 4);
    cloud.push_back({50.0, 50.0});   //nothing within epsilon of it

    std::vector<std::size_t> starts;
    const ComplexCloud contour = engine.epsilonHull(cloud, 1.5, nullptr, nullptr, &starts);

    ASSERT_EQ(starts.size(), 2u);
    //It cannot be walked (no second point), so it enters as the point it is.
    EXPECT_EQ(inBox(contour, {50.0, 50.0}, 0.5), 1u);
}

TEST(HullComponents, AConnectedBlockIsOneComponentAndTooSmallAnEpsilonFailsLoudly)
{
    TemplateEngine engine;

    ComplexCloud cloud;
    addBlock(cloud, {0.0, 0.0}, 1.0, 9);   //unit spacing: border of 32 points

    std::vector<std::size_t> starts;
    const ComplexCloud connected = engine.epsilonHull(cloud, 1.2, nullptr, nullptr, &starts);
    ASSERT_EQ(starts.size(), 1u) << "one component: the historical path";
    std::set<std::pair<double, double>> distinct;
    for (const std::complex<double> & p : connected) distinct.insert({p.real(), p.imag()});
    EXPECT_EQ(distinct.size(), 32u);

    //Below the spacing every point is its own component and none can be
    //walked: that is the historical failure and it stays loud - empty, which
    //the engine reports as an error - rather than the cloud handed back as a
    //contour of isolated points.
    const ComplexCloud scattered = engine.epsilonHull(cloud, 0.99, nullptr, nullptr, &starts);
    EXPECT_TRUE(scattered.empty());
    EXPECT_TRUE(starts.empty());
}
