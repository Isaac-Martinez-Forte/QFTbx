/**
 * @file
 * @brief Golden tests of the template computation against `planta2.qft`.
 *
 * The fixture ships the full clouds and the epsilon-hull contours of six
 * frequencies over a 10x10 grid of the two uncertain parameters with epsilon
 * 10. The clouds must match the stored ones to a relative 1e-5, with two hard
 * anchors fixing the sweep order; the contours are compared as cycles, same
 * sequence and direction from any starting point, since the faithful
 * `EPSHULL.M` walk closes on its first point, and every contour point must
 * belong to its cloud and start at the extreme the hybrid rule names. The
 * frequencies stay aligned with their templates whatever the thread count,
 * the input vectors survive the computation, a loaded project can recompute
 * its contour, and a missing grid or missing templates are refused.
 */

#include <gtest/gtest.h>

#include <string>

#include <vector>

#include <algorithm>

#include <complex>

#include "src/app/project_controller.h"
#include "src/core/common/exception.h"
#include "src/core/templates/template_engine.h"
#include "src/core/system/lti_system.h"
#include "src/core/system/parameter.h"
#include "src/core/frequencies/omega.h"
#include "src/core/math/sequence_vectors.h"
#include "src/core/math/sequences.h"
#include "src/persistence/project_reader.h"

using namespace qftbx;

namespace {

using Complex = std::complex<double>;

void expectNear(Complex actual, Complex expected, const char* where)
{
    const double tolR = std::max(1e-9, 1e-5 * std::abs(expected.real()));
    const double tolI = std::max(1e-9, 1e-5 * std::abs(expected.imag()));
    EXPECT_NEAR(actual.real(), expected.real(), tolR) << where;
    EXPECT_NEAR(actual.imag(), expected.imag(), tolI) << where;
}

class TemplatesGolden : public ::testing::Test
{
protected:
    void SetUp() override
    {
        parser.load(
            std::string(QFTBX_TEST_DATA_DIR "/planta2.qft"));
        plant = parser.plant();
        ASSERT_NE(plant, nullptr);

        mapa.clear();
        mapa[plant->numerator()[0].name()] = qftbx::math::linspace(1.0, 10.0, 10);
        mapa[plant->gain().name()] = qftbx::math::linspace(1.0, 10.0, 10);

        omegaCopy = *parser.omega()->values();
        epsilon = std::vector<double>(6, 10.0);

        templates.setEpsilon(epsilon);
        templates.setGrids(mapa);
        templates.compute(plant, &omegaCopy, false);
    }

    ProjectReader parser;
    LtiSystem* plant = nullptr;
    qftbx::ParameterGrids mapa;
    std::vector<double> omegaCopy;
    std::vector<double> epsilon;
    TemplateEngine templates;
};

TEST_F(TemplatesGolden, BruteForceMatchesFixture)
{
    const qftbx::CloudSet & computed = templates.clouds();
    const qftbx::CloudSet & expected = parser.templates();
    ASSERT_EQ(static_cast<int>(computed.size()), static_cast<int>(expected.size()));

    for (int f = 0; f < static_cast<int>(computed.size()); ++f) {
        ASSERT_EQ(computed.at(f).size(), expected.at(f).size())
            << "frequency " << f;
        for (int p = 0; p < computed.at(f).size(); ++p) {
            expectNear(computed.at(f).at(p), expected.at(f).at(p),
                       "template point");
        }
    }

    expectNear(computed.at(0).at(0), Complex(-0.990099, -9.90099), "t[0][0]");
    expectNear(computed.at(0).at(1), Complex(-0.498753, -9.97506), "t[0][1]");
}

TEST_F(TemplatesGolden, ContourMatchesFixtureAsACycle)
{
    const qftbx::CloudSet & computed = templates.contours();
    const qftbx::CloudSet & expected = parser.contour();
    ASSERT_EQ(static_cast<int>(computed.size()), static_cast<int>(expected.size()));

    const auto asCycle = [](const qftbx::ComplexCloud & contour) {
        std::vector<Complex> cycle(contour.begin(), contour.end());
        if (cycle.size() > 1 && cycle.front() == cycle.back()) {
            cycle.pop_back();
        }
        return cycle;
    };

    const std::size_t expectedSizes[] = {30, 28, 28, 28, 28, 28};
    for (int f = 0; f < static_cast<int>(computed.size()); ++f) {
        const std::vector<Complex> golden = asCycle(expected.at(f));
        ASSERT_EQ(golden.size(), expectedSizes[f]) << "frequency " << f;

        std::vector<Complex> cycle = asCycle(computed.at(f));
        ASSERT_EQ(cycle.size(), golden.size()) << "frequency " << f;

        std::size_t offset = 0;
        double best = std::abs(cycle.at(0) - golden.at(0));
        for (std::size_t i = 1; i < cycle.size(); ++i) {
            const double d = std::abs(cycle.at(i) - golden.at(0));
            if (d < best) {
                best = d;
                offset = i;
            }
        }

        for (std::size_t p = 0; p < cycle.size(); ++p) {
            expectNear(cycle.at((offset + p) % cycle.size()),
                       golden.at(p), "contour point");
        }
    }
}

TEST_F(TemplatesGolden, ContourStartHonoursTheHybridRule)
{
    const qftbx::CloudSet & temps = templates.clouds();
    const qftbx::CloudSet & conts = templates.contours();

    int fallbacks = 0;
    for (int f = 0; f < static_cast<int>(conts.size()); ++f) {
        const qftbx::ComplexCloud & c = conts.at(static_cast<std::size_t>(f));
        const bool faithful = c.size() > 1 && c.front() == c.back();
        if (!faithful) {
            ++fallbacks;
        }
        Complex extreme = temps.at(f).at(0);
        for (const Complex& p : temps.at(static_cast<std::size_t>(f))) {
            const bool better = faithful ? p.real() > extreme.real()
                                         : p.imag() > extreme.imag();
            if (better) {
                extreme = p;
            }
        }
        EXPECT_EQ(c.at(0), extreme)
            << "frequency " << f << (faithful ? " (faithful)" : " (fallback)");
    }

    EXPECT_EQ(fallbacks, 0);
}

TEST_F(TemplatesGolden, ContourIsSubsetOfTemplate)
{
    const qftbx::CloudSet & temps = templates.clouds();
    const qftbx::CloudSet & conts = templates.contours();

    for (int f = 0; f < static_cast<int>(conts.size()); ++f) {
        const qftbx::ComplexCloud & cloud = temps.at(static_cast<std::size_t>(f));
        for (const Complex& p : conts.at(static_cast<std::size_t>(f))) {
            EXPECT_TRUE(std::find(cloud.begin(), cloud.end(), p) != cloud.end())
                << "contour point not in template, frequency " << f;
        }
    }
}

TEST_F(TemplatesGolden, FrequencyAlignmentPreserved)
{
    const std::vector<double> original{0.1, 0.5, 1.0, 2.0, 15.0, 100.0};
    const std::vector<double> & omegaOut = templates.omega();
    ASSERT_EQ(omegaOut.size(), original.size());
    for (int i = 0; i < original.size(); ++i) {
        EXPECT_DOUBLE_EQ(omegaOut.at(i), original.at(i)) << "index " << i;
    }
}

TEST_F(TemplatesGolden, InputVectorsSurviveTheComputation)
{
    ASSERT_EQ(omegaCopy.size(), 6);
    EXPECT_DOUBLE_EQ(omegaCopy.at(0), 0.1);
    EXPECT_DOUBLE_EQ(omegaCopy.at(5), 100.0);
    ASSERT_EQ(epsilon.size(), 6);
    EXPECT_DOUBLE_EQ(epsilon.at(0), 10.0);
}

TEST(TemplatesReload, RecalculateContourAfterLoadingAProject)
{
    ProjectController controller;
    controller.load(
        std::string(QFTBX_TEST_DATA_DIR "/planta2.qft"));

    const qftbx::CloudSet & contornos = controller.recomputeContour(std::vector<double>(6, 10.0));
    ASSERT_EQ(contornos.size(), 6u);
    for (const qftbx::ComplexCloud & c : contornos) {
        EXPECT_FALSE(c.empty());
    }

    const qftbx::CloudSet & contornos2 = controller.recomputeContour(std::vector<double>(6, 8.0));
    ASSERT_EQ(contornos2.size(), 6u);
}

TEST(TemplatesValidation, MissingSweepGridThrowsInvalidInput)
{
    ProjectReader parser;
    parser.load(
        std::string(QFTBX_TEST_DATA_DIR "/planta2.qft"));
    LtiSystem* plant = parser.plant();

    qftbx::ParameterGrids mapa;
    mapa[plant->numerator()[0].name()] = qftbx::math::linspace(1.0, 10.0, 10);

    std::vector<double> omega{0.1, 0.5, 1.0, 2.0, 15.0, 100.0};

    TemplateEngine t;
    t.setEpsilon(std::vector<double>(6, 10.0));
    t.setGrids(mapa);
    EXPECT_THROW(t.compute(plant, &omega, false), qftbx::InvalidInput);
}

TEST(TemplatesValidation, RecontourWithoutTemplatesThrowsInvalidInput)
{
    TemplateEngine t;
    EXPECT_THROW(t.computeContours(std::vector<double>{10.0}), qftbx::InvalidInput);
}

}
