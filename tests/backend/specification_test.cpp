/**
 * @file
 * @brief Tests of the specification record, its validated form and its file.
 *
 * A constant bound is a linear magnitude converted to dB and independent of
 * frequency; a bound given as a system evaluates to its magnitude in dB,
 * checked against the analytic value of 120/(s^3 + 17 s^2 + 82 s + 120), the
 * tracking model of `planta2.qft`. A zero or negative height and an inverted
 * band are refused where the record is validated, since a silent -inf or NaN
 * bound would let the boundary degenerate to the window frame. The store holds
 * the seven records by value. The fixtures must yield their tracking and
 * stability records by slot, a shorter list fills only the slots it has, and
 * the tracking spread is the upper bound over the lower one in dB.
 */

#include <gtest/gtest.h>

#include <string>

#include <memory>

#include <cmath>
#include <complex>

#include "src/core/specifications/specification_record.h"
#include "src/core/common/exception.h"
#include "src/core/specifications/specification.h"
#include "src/core/project/project_data.h"
#include "src/core/system/polynomial_form.h"
#include "src/core/system/parameter.h"
#include "src/persistence/project_reader.h"

using namespace qftbx;

namespace {

using Complex = std::complex<double>;

qftbx::SpecificationRecord makeConstantStability(double linearHeight)
{
    qftbx::SpecificationRecord spec{};
    spec.name = std::string("estabilidad");
    spec.used = true;
    spec.constant = true;
    spec.system = nullptr;
    spec.height = linearHeight;
    spec.omegaStart = 0.1;
    spec.omegaEnd = 100.0;
    return spec;
}

std::unique_ptr<LtiSystem> makeTrackingPlant()
{
    std::vector<Parameter> numerator{Parameter(120.0)};
    std::vector<Parameter> denominator{Parameter(1.0), Parameter(17.0),
                                       Parameter(82.0), Parameter(120.0)};

    return std::make_unique<PolynomialForm>(std::string("seguimiento"),
                                            std::move(numerator), std::move(denominator),
                                            Parameter(1.0), Parameter(0.0));
}

double analyticTrackingDb(double w)
{
    const Complex s(0.0, w);
    const Complex value = 120.0 / (s * s * s + 17.0 * s * s + 82.0 * s + 120.0);
    return 20.0 * std::log10(std::abs(value));
}

TEST(Specification, ConstantHeightIsDbAndIgnoresOmega)
{
    qftbx::SpecificationRecord spec = makeConstantStability(1.2);

    const double expected = 20.0 * std::log10(1.2);
    const qftbx::Specification bound = qftbx::toSpecification(spec, qftbx::SpecificationType::Stability);
    EXPECT_NEAR(bound.boundDb(0.5), expected, 1e-12);
    EXPECT_NEAR(bound.boundDb(50.0), expected, 1e-12);
}

TEST(Specification, SystemHeightMatchesTheAnalyticValue)
{
    qftbx::SpecificationRecord spec{};
    spec.name = std::string("seguimiento");
    spec.used = true;
    spec.constant = false;
    spec.system = makeTrackingPlant();
    spec.omegaStart = 0.1;
    spec.omegaEnd = 10.0;

    const qftbx::Specification bound = qftbx::toSpecification(spec, qftbx::SpecificationType::TrackingLower);
    EXPECT_NEAR(bound.boundDb(1.0), analyticTrackingDb(1.0), 1e-9);
    EXPECT_NEAR(bound.boundDb(1.0), -0.76398, 1e-4);
}

TEST(Specification, ZeroHeightIsRefused)
{
    qftbx::SpecificationRecord spec = makeConstantStability(0.0);
    EXPECT_THROW(qftbx::toSpecification(spec, qftbx::SpecificationType::Stability),
                 qftbx::InvalidInput);
}

TEST(Specification, NegativeHeightIsRefused)
{
    qftbx::SpecificationRecord spec = makeConstantStability(-1.0);
    EXPECT_THROW(qftbx::toSpecification(spec, qftbx::SpecificationType::Stability),
                 qftbx::InvalidInput);
}

TEST(SpecificationDao, OwnsReplacesAndToleratesIdentity)
{
    qftbx::SpecificationRecords first;
    first.at(0).used = true;
    first.at(0).constant = false;
    first.at(0).system = makeTrackingPlant();

    qftbx::ProjectData data;
    data.setSpecifications(std::move(first));

    ASSERT_NE(data.specifications(), nullptr);
    EXPECT_TRUE(data.specifications()->at(0).used);
    ASSERT_NE(data.specifications()->at(0).system, nullptr);

    qftbx::SpecificationRecords second;
    second.at(1).used = true;
    data.setSpecifications(std::move(second));

    ASSERT_NE(data.specifications(), nullptr);
    EXPECT_FALSE(data.specifications()->at(0).used);
    EXPECT_TRUE(data.specifications()->at(1).used);
}

TEST(SpecificationPersistence, MultivaluadosSpecificationsRoundTrip)
{
    ProjectReader parser;
    parser.load(
        std::string(QFTBX_TEST_DATA_DIR "/multivaluados.qft"));

    const qftbx::SpecificationRecords * specs = parser.specifications();
    ASSERT_NE(specs, nullptr);

    const qftbx::SpecificationRecord & lower = specs->at(0);
    EXPECT_EQ(lower.name, std::string("TrackingLower"));
    EXPECT_TRUE(lower.used);
    EXPECT_FALSE(lower.constant);
    EXPECT_DOUBLE_EQ(lower.omegaStart, 1.0);
    EXPECT_DOUBLE_EQ(lower.omegaEnd, 18.0);
    ASSERT_NE(lower.system, nullptr);
    EXPECT_EQ(lower.system->type(), LtiSystem::SystemType::PolynomialForm);
    EXPECT_EQ(lower.system->numerator().size(), 2);

    const qftbx::SpecificationRecord & upper = specs->at(1);
    EXPECT_EQ(upper.name, std::string("TrackingUpper"));
    EXPECT_TRUE(upper.used);
    ASSERT_NE(upper.system, nullptr);
    EXPECT_EQ(upper.system->numerator().size(), 3);

    for (std::size_t i = 2; i < 7; ++i) {
        EXPECT_FALSE(specs->at(i).used) << "index " << i;
    }
}

TEST(SpecificationPersistence, Planta2RecoversBothTrackingPlants)
{
    ProjectReader parser;
    parser.load(
        std::string(QFTBX_TEST_DATA_DIR "/planta2.qft"));

    const qftbx::SpecificationRecords * specs = parser.specifications();
    ASSERT_NE(specs, nullptr);

    const qftbx::SpecificationRecord & lower = specs->at(0);
    ASSERT_NE(lower.system, nullptr);
    EXPECT_EQ(lower.system->type(), LtiSystem::SystemType::PolynomialForm);
    EXPECT_DOUBLE_EQ(lower.omegaStart, 0.1);
    EXPECT_DOUBLE_EQ(lower.omegaEnd, 10.0);
    EXPECT_NEAR(qftbx::toSpecification(lower, qftbx::SpecificationType::TrackingLower).boundDb(1.0),
                analyticTrackingDb(1.0), 1e-9);

    const qftbx::SpecificationRecord & upper = specs->at(1);
    ASSERT_NE(upper.system, nullptr);
    EXPECT_EQ(upper.system->type(), LtiSystem::SystemType::FreeForm);
}

TEST(SpecificationPersistence, Planta1RecoversTheConstantStability)
{
    ProjectReader parser;
    parser.load(
        std::string(QFTBX_TEST_DATA_DIR "/planta1.qft"));

    const qftbx::SpecificationRecords * specs = parser.specifications();
    ASSERT_NE(specs, nullptr);

    const qftbx::SpecificationRecord & stability = specs->at(2);
    EXPECT_TRUE(stability.used);
    EXPECT_TRUE(stability.constant);
    EXPECT_DOUBLE_EQ(stability.height, 1.2);
    EXPECT_NEAR(qftbx::toSpecification(stability, qftbx::SpecificationType::Stability).boundDb(3.0),
                20.0 * std::log10(1.2), 1e-12);

    EXPECT_TRUE(specs->at(4).used);
    EXPECT_FALSE(specs->at(4).constant);
}

TEST(SpecificationPersistence, AShorterSpecificationListFillsTheSlotsItHas)
{
    ProjectReader parser;
    ASSERT_NO_THROW(parser.load(std::string(QFTBX_TEST_DATA_DIR "/short_specs.qft")));

    const qftbx::SpecificationRecords * specs = parser.specifications();
    ASSERT_NE(specs, nullptr);
    EXPECT_EQ(specs->size(), qftbx::kSpecificationCount);

    EXPECT_EQ(specs->at(0).name, "TrackingLower");
    EXPECT_EQ(specs->at(2).name, "Stability");
    for (const qftbx::SpecificationRecord & record : *specs) {
        EXPECT_FALSE(record.used);
    }
    EXPECT_TRUE(specs->at(6).name.empty()) << "a slot the file does not have is untouched";
}

TEST(QftbxUnits, DbLinearConversionsRoundTrip)
{
    EXPECT_NEAR(qftbx::linearToDb(2.0), 6.02059991, 1e-7);
    EXPECT_NEAR(qftbx::dbToLinear(6.02059991), 2.0, 1e-9);
    EXPECT_NEAR(qftbx::dbToLinear(qftbx::linearToDb(1.2)), 1.2, 1e-12);
    EXPECT_DOUBLE_EQ(qftbx::linearToDb(1.0), 0.0);
}

TEST(QftbxSpecification, FactoriesValidateTheirInvariants)
{
    using qftbx::Specification;
    using qftbx::SpecificationType;

    EXPECT_THROW(Specification::constant(SpecificationType::Stability, 0.0, 0.1, 10.0),
                 qftbx::InvalidInput);
    EXPECT_THROW(Specification::constant(SpecificationType::Stability, -1.0, 0.1, 10.0),
                 qftbx::InvalidInput);
    EXPECT_THROW(Specification::constant(SpecificationType::Stability, 1.2, 10.0, 0.1),
                 qftbx::InvalidInput);
    EXPECT_THROW(Specification::fromSystem(SpecificationType::TrackingLower, nullptr, 0.1, 10.0),
                 qftbx::InvalidInput);
}

TEST(QftbxSpecification, BoundDbMatchesTheHistoricalSemantics)
{
    using qftbx::Specification;
    using qftbx::SpecificationType;

    Specification stability =
        Specification::constant(SpecificationType::Stability, 1.2, 0.1, 100.0);
    EXPECT_NEAR(stability.boundDb(0.5), 20.0 * std::log10(1.2), 1e-12);
    EXPECT_NEAR(stability.boundDb(50.0), 20.0 * std::log10(1.2), 1e-12);

    Specification tracking = Specification::fromSystem(
        SpecificationType::TrackingLower, makeTrackingPlant(), 0.1, 10.0);
    EXPECT_NEAR(tracking.boundDb(1.0), analyticTrackingDb(1.0), 1e-9);
}

TEST(QftbxSpecification, AppliesAtIsAClosedIntervalAndUnusedNeverApplies)
{
    using qftbx::Specification;
    using qftbx::SpecificationType;

    Specification spec =
        Specification::constant(SpecificationType::Stability, 1.2, 0.1, 10.0);
    EXPECT_TRUE(spec.appliesAt(0.1));
    EXPECT_TRUE(spec.appliesAt(10.0));
    EXPECT_TRUE(spec.appliesAt(2.0));
    EXPECT_FALSE(spec.appliesAt(0.0999));
    EXPECT_FALSE(spec.appliesAt(10.001));

    Specification idle = Specification::unused(SpecificationType::ControlEffort);
    EXPECT_FALSE(idle.appliesAt(1.0));
    EXPECT_FALSE(idle.used());
}

TEST(QftbxSpecification, MoveTransfersOwnership)
{
    using qftbx::Specification;
    using qftbx::SpecificationType;

    Specification original = Specification::fromSystem(
        SpecificationType::TrackingLower, makeTrackingPlant(), 0.1, 10.0);
    const LtiSystem* plant = original.system();

    Specification moved = std::move(original);
    EXPECT_EQ(moved.system(), plant);
    EXPECT_EQ(original.system(), nullptr);
    EXPECT_FALSE(original.used());
}

TEST(QftbxSpecificationSet, DefaultsUnusedAndCentralisesTheTrackingSpread)
{
    using qftbx::Specification;
    using qftbx::SpecificationSet;
    using qftbx::SpecificationType;

    SpecificationSet set;
    for (int i = 0; i < qftbx::kSpecificationCount; ++i) {
        EXPECT_FALSE(set.at(static_cast<SpecificationType>(i)).used());
    }

    set.set(Specification::constant(SpecificationType::TrackingLower, 1.0, 0.1, 10.0));
    set.set(Specification::constant(SpecificationType::TrackingUpper, 2.0, 0.1, 10.0));

    EXPECT_NEAR(set.trackingSpreadDb(1.0), 20.0 * std::log10(2.0), 1e-12);
}

}
