// The pipeline walked from nothing, in the order the interface drives it.
//
// Everything that reaches the loop shaping in this suite starts from a loaded
// .qft: the golden tests, the benchmarks, the article validations. Nothing
// walked the seven stages the way a user does - publish a plant, then the
// specifications, then the frequencies, compute the templates, the
// boundaries, publish a controller structure, run the search - asserting that
// each stage produces its own artefact and that the one before it survives.
//
// It is the net for splitting ProjectController into a class per stage
// (plan 10.3): the split must not change any of this. Written against the
// CURRENT behaviour on purpose, so a difference means the split changed
// something, not that the test needs adjusting.
//
// The fixture is deliberately tiny - three frequencies, three points per
// grid, a coarse Nichols grid - because what is under test is the sequence,
// not the numbers. The numbers have their own golden tests.

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include "src/core/exception.h"
#include "src/core/frequencies/omega.h"
#include "src/core/math/sequence_vectors.h"
#include "src/core/math/sequences.h"
#include "src/core/project_controller.h"
#include "src/core/range.h"
#include "src/core/specifications/specification_record.h"
#include "src/core/system/parameter.h"
#include "src/core/system/polynomial_form.h"
#include "src/core/system/zero_pole_gain.h"

namespace {

//P(s) = kv / (a*s + 1), both coefficients uncertain so the sweep has a grid
//to walk and the templates come out as areas rather than points.
std::unique_ptr<LtiSystem> makePlant()
{
    std::vector<Parameter> numerator{Parameter(1.0)};
    std::vector<Parameter> denominator{
        Parameter(std::string("a"), qftbx::Range(1.0, 2.0), 1.5),
        Parameter(1.0)};

    return std::make_unique<PolynomialForm>(std::string("P"), numerator, denominator,
                              Parameter(std::string("kv"), qftbx::Range(1.0, 2.0), 1.5),
                              Parameter(0.0));
}

//K(s) = kc * (s + z) / (s + p), with the gain and both roots given as search
//boxes.
//
//Two things this test found by being written, both worth pinning. It is a
//ZeroPoleGain and not a PolynomialForm because the loop shaping refuses
//polynomial and free-form structures: the interval projection of those is not
//implemented. And the gain is "kc" and not "k" because muParserX reserves k
//as its kilo postfix operator - see StageSequence.ReservedParameterNames.
std::unique_ptr<LtiSystem> makeControllerStructure()
{
    std::vector<Parameter> numerator{
        Parameter(std::string("z"), qftbx::Range(0.1, 10.0), 1.0)};
    std::vector<Parameter> denominator{
        Parameter(std::string("p"), qftbx::Range(0.1, 10.0), 1.0)};

    return std::make_unique<ZeroPoleGain>(std::string("K"), numerator, denominator,
                              Parameter(std::string("kc"), qftbx::Range(0.01, 100.0), 1.0),
                              Parameter(0.0));
}

std::unique_ptr<Omega> makeOmega()
{
    return std::make_unique<Omega>(0.1, 10.0, 3, tools::logspace(-1.0, 1.0, 3), Omega::LogSpace);
}

qftbx::ParameterGrids makeGrids()
{
    qftbx::ParameterGrids grids;
    grids[std::string("a")] = qftbx::math::linspace(1.0, 2.0, 3);
    grids[std::string("kv")] = qftbx::math::linspace(1.0, 2.0, 3);
    return grids;
}

//A permissive constant stability bound, in LINEAR magnitude, over the whole
//frequency band.
qftbx::SpecificationRecords makeSpecifications()
{
    qftbx::SpecificationRecords records;

    qftbx::SpecificationRecord & stability =
            records.at(static_cast<std::size_t>(qftbx::SpecificationType::Stability));
    stability.name = qftbx::specificationName(qftbx::SpecificationType::Stability);
    stability.used = true;
    stability.constant = true;
    stability.system = nullptr;
    stability.height = 5.0;
    stability.omegaStart = 0.1;
    stability.omegaEnd = 10.0;

    return records;
}

} // namespace

TEST(StageSequence, TheSevenStagesWalkedFromNothing)
{
    ProjectController controller;

    // --- 1: the plant
    EXPECT_TRUE(controller.setPlant(makePlant()));
    ASSERT_NE(controller.plant(), nullptr);

    // --- 2: the specifications
    controller.setSpecifications(makeSpecifications());
    ASSERT_NE(controller.specifications(), nullptr);

    // --- 3: the design frequencies
    EXPECT_TRUE(controller.setOmega(makeOmega()));
    ASSERT_NE(controller.omega(), nullptr);
    ASSERT_EQ(controller.frequencies()->size(), 3u);

    // --- 4: the templates. One cloud per design frequency, and the plant
    // and the frequencies are still the ones published above.
    ASSERT_TRUE(controller.computeTemplates(std::vector<double>(3, 10.0), makeGrids(), false));
    ASSERT_EQ(controller.templates().size(), 3u);
    for (const qftbx::ComplexCloud & cloud : controller.templates()) {
        EXPECT_FALSE(cloud.empty());
    }
    EXPECT_NE(controller.plant(), nullptr);
    EXPECT_NE(controller.omega(), nullptr);

    // --- 5: the boundaries, over a coarse Nichols grid.
    ASSERT_TRUE(controller.computeBoundaries(qftbx::Range(-360.0, 0.0), 37,
                                             qftbx::Range(-40.0, 40.0), 21,
                                             -1.0, false, false));
    ASSERT_NE(controller.boundaries(), nullptr);
    EXPECT_FALSE(controller.templates().empty())
        << "computing the boundaries must not disturb the templates they came from";

    // --- 6: the controller structure. It invalidates nothing here, because
    // there is no loop shaping yet to invalidate.
    EXPECT_TRUE(controller.setControllerStructure(makeControllerStructure()));
    ASSERT_NE(controller.controllerStructure(), nullptr);
    EXPECT_NE(controller.boundaries(), nullptr);

    // --- 7: the search.
    ASSERT_TRUE(controller.computeLoopShaping(0.5, tools::nt,
                                              qftbx::Range(1e-3, 100.0), 100));
    ASSERT_NE(controller.loopShapingResult(), nullptr);

    LtiSystem * const designed = controller.loopShapingResult()->controller();
    ASSERT_NE(designed, nullptr);

    // The answer is a POINT of the search box, not the box.
    EXPECT_EQ(designed->gain().range().min, designed->gain().range().max);
    EXPECT_GT(designed->gain().range().min, 0.0);

    // And everything the search consumed is still standing.
    EXPECT_NE(controller.plant(), nullptr);
    EXPECT_NE(controller.boundaries(), nullptr);
    EXPECT_FALSE(controller.templates().empty());
}

TEST(StageSequence, ReservedParameterNamesAreRefusedWhenPublished)
{
    // Found by writing the walk above: muParserX reserves six single letters
    // as SI unit postfix operators - n, u, m, k, M and G - and it refuses to
    // bind a variable under any of them. "k" is the one that matters, being
    // what everybody calls a gain.
    //
    // The old failure mode was as bad as it gets: the name was accepted, the
    // plant and the boundaries computed fine, and then the search threw a
    // mup::ParserError from deep inside. That is neither a qftbx::Exception
    // nor a std::exception, and the window catches only the first, so the
    // application terminated. It is refused at publication now, which is
    // once per project and nowhere near the search.
    for (const char * reserved : {"n", "u", "m", "k", "M", "G"}) {
        ProjectController controller;

        std::vector<Parameter> numerator{Parameter(1.0)};
        std::vector<Parameter> denominator{Parameter(1.0)};
        auto plant = std::make_unique<PolynomialForm>(
                    std::string("P"), numerator, denominator,
                    Parameter(std::string(reserved), qftbx::Range(1.0, 2.0), 1.5),
                    Parameter(0.0));

        EXPECT_THROW(controller.setPlant(std::move(plant)), qftbx::InvalidInput)
            << "a parameter named \"" << reserved << "\" has to be refused";
    }
}

TEST(StageSequence, AConstantKeepsItsNumericName)
{
    // The check must not reach constants: Parameter(double) names itself with
    // the number, which is not an identifier at all, and no expression ever
    // binds it as a variable.
    ProjectController controller;

    std::vector<Parameter> numerator{Parameter(1.0)};
    std::vector<Parameter> denominator{Parameter(2.5), Parameter(1.0)};
    auto plant = std::make_unique<PolynomialForm>(
                std::string("P"), numerator, denominator,
                Parameter(1.0), Parameter(0.0));

    EXPECT_NO_THROW(controller.setPlant(std::move(plant)));
    EXPECT_NE(controller.plant(), nullptr);
}
