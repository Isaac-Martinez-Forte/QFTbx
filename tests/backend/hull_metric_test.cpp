// The plane the epsilon of the hull is measured in, and the epsilon a cloud
// asks for.
//
// The walk only asks how far apart two points are. Measured in the complex
// plane - the historical reading - an epsilon means one thing where the
// template sits at 40 dB and another where it sits at -40 dB: on example 2
// the epsilon the six templates need spans a factor of ten thousand, and the
// point spacing within one template a factor of seven hundred. Measured in
// the Nichols plane, in degrees and decibels as Nordin's own prune.m does,
// those factors are three and thirteen. The metric is a choice of the
// engine, complex plane by default so that nothing stored moves.
//
// And the epsilon a cloud asks for has an exact answer: the longest edge of
// its minimum spanning tree is the least epsilon that keeps it connected,
// hence loses nothing, and being the least rolls over the fewest concavities.

#include <gtest/gtest.h>

#include <cmath>
#include <complex>
#include <cstdio>
#include <string>
#include <vector>

#include "src/app/project_controller.h"
#include "src/core/templates/template_engine.h"

using namespace qftbx;
using Complex = std::complex<double>;

namespace {

Complex atNichols(double phaseDegrees, double db)
{
    return std::polar(std::pow(10.0, db / 20.0), phaseDegrees * M_PI / 180.0);
}

} // namespace

TEST(HullMetric, TheDefaultIsTheComplexPlane)
{
    TemplateEngine engine;
    EXPECT_EQ(engine.hullMetric(), TemplateEngine::HullMetric::ComplexPlane);
    EXPECT_DOUBLE_EQ(engine.dbPerDegree(), 1.0);
    EXPECT_THROW(engine.setHullMetric(TemplateEngine::HullMetric::Nichols, 0.0), qftbx::InvalidInput);
}

TEST(HullMetric, TheSameCloudIsConnectedInOnePlaneAndNotInTheOther)
{
    //Two points at the same phase, 40 dB apart: 99 units apart in the
    //complex plane, 40 in the Nichols plane at one dB per degree. An
    //epsilon of 45 joins them in Nichols and not in the complex plane.
    ComplexCloud cloud{atNichols(-90.0, 0.0), atNichols(-90.0, 40.0)};
    std::vector<std::size_t> starts;

    TemplateEngine complexPlane;
    const ComplexCloud a = complexPlane.epsilonHull(cloud, 45.0, nullptr, nullptr, &starts);
    EXPECT_TRUE(a.empty()) << "no second point within reach in the complex plane";

    TemplateEngine nichols;
    nichols.setHullMetric(TemplateEngine::HullMetric::Nichols, 1.0);
    const ComplexCloud b = nichols.epsilonHull(cloud, 45.0, nullptr, nullptr, &starts);
    ASSERT_EQ(starts.size(), 1u);
    EXPECT_EQ(b.size(), 3u) << "two points: the minimal closed contour, in the cloud's own units";
    EXPECT_NEAR(std::abs(b.front()), 1.0, 1e-12);

    //Weighing decibels heavier makes the gap larger again.
    nichols.setHullMetric(TemplateEngine::HullMetric::Nichols, 0.5);   //2 degrees per dB
    EXPECT_TRUE(nichols.epsilonHull(cloud, 45.0, nullptr, nullptr, &starts).empty());
}

TEST(HullMetric, TheBranchCutFallsInTheWidestGap)
{
    //Points at -1 and +1 degrees straddle the -360/0 cut of the Nichols
    //branch: read naively they are 358 degrees apart; the cut placed in the
    //cloud's widest gap makes them 2 apart.
    ComplexCloud cloud{atNichols(-1.0, 0.0), atNichols(1.0, 0.0), atNichols(0.0, 1.0)};
    std::vector<std::size_t> starts;

    TemplateEngine nichols;
    nichols.setHullMetric(TemplateEngine::HullMetric::Nichols, 1.0);
    const ComplexCloud hull = nichols.epsilonHull(cloud, 3.0, nullptr, nullptr, &starts);
    ASSERT_EQ(starts.size(), 1u) << "one component across the cut";
    EXPECT_GE(hull.size(), 3u);
}

TEST(HullMetric, TheProposedEpsilonIsTheLongestSpanningEdge)
{
    //A 9x9 unit block: the spanning tree's longest edge is 1, the diameter
    //8 sqrt 2, in the complex plane.
    ComplexCloud block;
    for (int i = 0; i < 9; ++i) for (int j = 0; j < 9; ++j) block.push_back({double(i), double(j)});

    TemplateEngine engine;
    engine.setClouds({block});
    const std::vector<TemplateEngine::EpsilonProposal> p = engine.proposeEpsilon();
    ASSERT_EQ(p.size(), 1u);
    EXPECT_NEAR(p[0].connected, 1.0, 1e-12);
    EXPECT_NEAR(p[0].diameter, 8.0 * std::sqrt(2.0), 1e-12);
    EXPECT_NEAR(p[0].coarseness(), 1.0 / (8.0 * std::sqrt(2.0)), 1e-12);
    //On a regular block the walk closes at the connecting epsilon itself.
    EXPECT_TRUE(p[0].closes);
    EXPECT_DOUBLE_EQ(p[0].epsilon, 1.0);

    //Two blocks 40 apart: the longest edge is the bridge between them.
    ComplexCloud two = block;
    for (const Complex & z : block) two.push_back(z + Complex(40.0, 0.0));
    engine.setClouds({two});
    const TemplateEngine::EpsilonProposal bridge = engine.proposeEpsilon()[0];
    EXPECT_NEAR(bridge.connected, 32.0, 1e-12);
    EXPECT_TRUE(bridge.closes);
    EXPECT_GE(bridge.epsilon, bridge.connected);

    //And with that epsilon the walk finds one component; with less, two.
    std::vector<std::size_t> starts;
    engine.epsilonHull(two, 32.0, nullptr, nullptr, &starts);
    EXPECT_EQ(starts.size(), 1u);
    engine.epsilonHull(two, 31.9, nullptr, nullptr, &starts);
    EXPECT_EQ(starts.size(), 2u);
}

//Example 2 as stored (25x25, epsilon 10 in the complex plane at every
//frequency): what its clouds ask for in each plane. In the complex plane the
//proposal spans a factor of ten thousand across the six frequencies and the
//stored epsilon is orders of magnitude above it at the high ones; in the
//Nichols plane it spans a factor of about three. The figures were measured
//in Python first (datos/scripts/eps2.py) and are pinned here to a per cent.
TEST(HullMetric, Example2AsksForOneEpsilonInNicholsAndTenThousandInTheComplexPlane)
{
    ProjectController controller;
    controller.load(std::string(QFTBX_TEST_DATA_DIR "/qft_toolbox_ex2.qft"));

    TemplateEngine engine;
    engine.setClouds(controller.templates());

    const std::vector<TemplateEngine::EpsilonProposal> complexPlane = engine.proposeEpsilon();
    engine.setHullMetric(TemplateEngine::HullMetric::Nichols, 1.0);
    const std::vector<TemplateEngine::EpsilonProposal> nichols = engine.proposeEpsilon();
    ASSERT_EQ(complexPlane.size(), 6u);
    ASSERT_EQ(nichols.size(), 6u);

    const std::vector<double> & w = *controller.omega()->values();
    double cMin = 1e300, cMax = 0.0, nMin = 1e300, nMax = 0.0;
    for (std::size_t i = 0; i < 6; ++i) {
        std::printf("PROPOSE w=%-6g complex eps*=%.4g (%.1f%% of diameter)   nichols eps*=%.3f (%.1f%%)\n",
                    w[i], complexPlane[i].connected, 100.0 * complexPlane[i].coarseness(),
                    nichols[i].connected, 100.0 * nichols[i].coarseness());
        cMin = std::min(cMin, complexPlane[i].connected); cMax = std::max(cMax, complexPlane[i].connected);
        nMin = std::min(nMin, nichols[i].connected); nMax = std::max(nMax, nichols[i].connected);
    }
    std::fflush(stdout);

    EXPECT_GT(cMax / cMin, 5000.0) << "complex plane: a different epsilon at every frequency";
    EXPECT_LT(nMax / nMin, 4.0) << "Nichols plane: one epsilon serves them all";
    EXPECT_NEAR(complexPlane[0].connected, 3.731, 0.04);
    EXPECT_NEAR(complexPlane[5].connected, 3.714e-4, 4e-6);
    EXPECT_NEAR(nichols[2].connected, 8.973, 0.09);
    EXPECT_NEAR(nichols[5].connected, 2.766, 0.03);
    //The stored epsilon, 10, is 27 000 times what w = 100 needs.
    EXPECT_GT(10.0 / complexPlane[5].epsilon, 20000.0);
}
