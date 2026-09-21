/**
 * @file
 * @brief A plant family that crosses the imaginary axis is refused at the sweep.
 *
 * QFT carries the stability of the nominal loop to the family only when every
 * member has the same number of right half-plane poles. On P(s) = 1/(s^2 - a),
 * unstable for a > 0 and with poles on the axis for a < 0, a range of a that
 * straddles zero is refused as invalid input at the sweep, where every member
 * is in hand, and a range on either side sweeps.
 */

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include "src/app/project_controller.h"
#include "src/core/common/exception.h"
#include "src/core/frequencies/omega.h"
#include "src/core/math/range.h"
#include "src/core/math/sequences.h"
#include "src/core/math/sequence_vectors.h"
#include "src/core/system/free_form.h"
#include "src/core/system/parameter.h"

using namespace qftbx;

namespace {

std::unique_ptr<LtiSystem> plantWith(qftbx::Range a, double nominal)
{
    std::vector<Parameter> none;
    std::vector<Parameter> denominator{Parameter(std::string("a"), a, nominal)};
    return std::make_unique<FreeForm>(std::string("P"), none, denominator, Parameter(1.0), Parameter(0.0),
                                      std::string("1"), std::string("s^2-a"));
}

void setUp(ProjectController & controller, qftbx::Range a, double nominal)
{
    controller.setOmega(std::make_unique<Omega>(0.1, 10.0, 3, qftbx::logspace(-1.0, 1.0, 3), Omega::LogSpace));
    controller.setPlant(plantWith(a, nominal));
}

}

TEST(UnstableFamily, AFamilyThatCrossesTheAxisIsRefusedAtTheSweep)
{
    ProjectController controller;
    setUp(controller, qftbx::Range(-1.0, 1.0), 0.5);
    qftbx::ParameterGrids grids;
    grids[std::string("a")] = qftbx::math::linspace(-1.0, 1.0, 3);

    EXPECT_THROW(controller.computeTemplates(std::vector<double>(3, 10.0), grids, false), qftbx::InvalidInput);
}

TEST(UnstableFamily, AFamilyOnOneSideSweeps)
{
    ProjectController unstable;
    setUp(unstable, qftbx::Range(1.0, 2.0), 1.5);
    qftbx::ParameterGrids grids;
    grids[std::string("a")] = qftbx::math::linspace(1.0, 2.0, 3);
    EXPECT_TRUE(unstable.computeTemplates(std::vector<double>(3, 10.0), grids, false));

    ProjectController stable;
    setUp(stable, qftbx::Range(-3.0, -2.0), -2.5);
    grids[std::string("a")] = qftbx::math::linspace(-3.0, -2.0, 3);
    EXPECT_TRUE(stable.computeTemplates(std::vector<double>(3, 10.0), grids, false));
}
