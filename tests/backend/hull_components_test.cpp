/**
 * @file
 * @brief Tests of the epsilon-hull of a cloud that is not epsilon-connected.
 *
 * The walk of Prune looks only within epsilon of the current point, so it
 * cannot leave the component it started in; Gutman, Nordin and Cohen 2007,
 * section 3, define it for an epsilon-connected set. The engine therefore
 * finds the components first, by union-find over the neighbour grid, and
 * walks each one, rightmost first, so that no block of the template is lost
 * from the value set the boundaries are computed over. The cases check two
 * and three separated blocks, an isolated point kept as its own component,
 * a single connected block that takes the plain walk, and an epsilon below
 * the spacing, where nothing can be walked and the result is empty rather
 * than a cloud of isolated points handed back as a contour.
 */

#include <gtest/gtest.h>

#include <complex>
#include <set>
#include <vector>

#include "src/core/templates/template_engine.h"

using namespace qftbx;

namespace {

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

}

TEST(HullComponents, TwoBlocksFartherApartThanEpsilonBothReachTheContour)
{
    TemplateEngine engine;

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

    EXPECT_NEAR(contour[starts[0]].real(), 60.0, 3.0);
    EXPECT_NEAR(contour[starts[1]].real(), 30.0, 3.0);
    EXPECT_NEAR(contour[starts[2]].real(), 0.0, 3.0);
}

TEST(HullComponents, AnIsolatedPointIsKeptAsItsOwnComponent)
{
    TemplateEngine engine;

    ComplexCloud cloud;
    addBlock(cloud, {0.0, 0.0}, 1.0, 4);
    cloud.push_back({50.0, 50.0});

    std::vector<std::size_t> starts;
    const ComplexCloud contour = engine.epsilonHull(cloud, 1.5, nullptr, nullptr, &starts);

    ASSERT_EQ(starts.size(), 2u);
    EXPECT_EQ(inBox(contour, {50.0, 50.0}, 0.5), 1u);
}

TEST(HullComponents, AConnectedBlockIsOneComponentAndTooSmallAnEpsilonFailsLoudly)
{
    TemplateEngine engine;

    ComplexCloud cloud;
    addBlock(cloud, {0.0, 0.0}, 1.0, 9);

    std::vector<std::size_t> starts;
    const ComplexCloud connected = engine.epsilonHull(cloud, 1.2, nullptr, nullptr, &starts);
    ASSERT_EQ(starts.size(), 1u) << "one component: the historical path";
    std::set<std::pair<double, double>> distinct;
    for (const std::complex<double> & p : connected) distinct.insert({p.real(), p.imag()});
    EXPECT_EQ(distinct.size(), 32u);

    const ComplexCloud scattered = engine.epsilonHull(cloud, 0.99, nullptr, nullptr, &starts);
    EXPECT_TRUE(scattered.empty());
    EXPECT_TRUE(starts.empty());
}
