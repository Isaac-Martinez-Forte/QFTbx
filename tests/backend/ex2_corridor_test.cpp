// The QFT toolbox example 2 at w = 100, phase -127 degrees: the tracking
// floor at -34 dB runs under the wall of the stability boundary, whose inside
// reaches from -32 up to 8.5 dB. The allowed magnitudes are the corridor
// between the two and the region above the curve. The union-and-parity
// classification called the inside of the curve allowed here, and every
// algorithm parked L0(j100) in it (2026-09-07).

#include <gtest/gtest.h>

#include <string>

#include "src/app/project_controller.h"
#include "src/core/loopshaping/common/boundary_violation_detector.h"

using namespace qftbx;

TEST(Ex2Corridor, TheInsideOfTheStabilityBoundaryIsForbiddenOverTheTrackingFloor)
{
    ProjectController controller;
    controller.load(std::string(QFTBX_TEST_DATA_DIR "/qft_toolbox_ex2.qft"));
    const BoundaryData * boundaries = controller.boundaries();
    BoundaryViolationDetector detector;
    const std::size_t w100 = 5;

    EXPECT_EQ(detector.classifyPoint(NicholsPoint(-127.0, -36.0), boundaries, w100), infeasible);   //under the floor
    EXPECT_EQ(detector.classifyPoint(NicholsPoint(-127.0, -33.0), boundaries, w100), feasible);     //the corridor
    EXPECT_EQ(detector.classifyPoint(NicholsPoint(-127.0, -30.0), boundaries, w100), infeasible);   //inside the wall
    EXPECT_EQ(detector.classifyPoint(NicholsPoint(-127.0, 0.0), boundaries, w100), infeasible);     //inside the curve
    EXPECT_EQ(detector.classifyPoint(NicholsPoint(-127.0, 9.0), boundaries, w100), feasible);       //above it

    //At w = 15 the same phase has only the tracking floor at -11.5 dB.
    const std::size_t w15 = 4;
    EXPECT_EQ(detector.classifyPoint(NicholsPoint(-119.5, -12.0), boundaries, w15), infeasible);
    EXPECT_EQ(detector.classifyPoint(NicholsPoint(-119.5, -11.0), boundaries, w15), feasible);
}
