// The template engine refuses a miscounted epsilon before its parallel
// loop: an exception thrown inside an OpenMP region terminates the
// process, so the check has to happen outside it.

#include <gtest/gtest.h>

#include <complex>

#include "src/core/common/exception.h"
#include "src/core/templates/template_engine.h"

using namespace qftbx;

namespace {


CloudSet twoCloudsWithinReach()
{
    // Two points of each cloud within 0.1 of each other: an epsilon of 0.1
    // closes the hull.
    return CloudSet{
        {std::complex<double>(0.0, 0.0), std::complex<double>(0.05, 0.0)},
        {std::complex<double>(1.0, 0.0), std::complex<double>(1.05, 0.0)},
    };
}

TEST(TemplateEngineGuards, AMiscountedEpsilonIsRefusedBeforeTheContourWalk)
{
    TemplateEngine engine;
    engine.setClouds(twoCloudsWithinReach());

    EXPECT_THROW(engine.computeContours({0.1}), qftbx::InvalidInput);
    EXPECT_THROW(engine.computeContours({0.1, 0.1, 0.1}), qftbx::InvalidInput);
}

TEST(TemplateEngineGuards, OneEpsilonPerCloudWalksEveryContour)
{
    TemplateEngine engine;
    engine.setClouds(twoCloudsWithinReach());

    ASSERT_TRUE(engine.computeContours({0.1, 0.1}));
    ASSERT_EQ(engine.contours().size(), 2u);
    EXPECT_FALSE(engine.contours().at(0).empty());
    EXPECT_FALSE(engine.contours().at(1).empty());
}

} // namespace
