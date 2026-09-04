// Tests for the QFT specification record (qftbx::SpecificationRecord), its
// conversion to qftbx::Specification and its persistence.

#include <gtest/gtest.h>

#include <string>

#include <memory>

#include <cmath>
#include <complex>


#include "src/core/specifications/specification_record.h"
#include "src/core/exception.h"
#include "src/core/specifications/specification.h"
#include "src/core/project_data.h"
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
    spec.height = linearHeight; // linear magnitude, not dB
    spec.omegaStart = 0.1;
    spec.omegaEnd = 100.0;
    return spec;
}

// 120 / (s^3 + 17 s^2 + 82 s + 120): the tracking specification plant of
// tests/data/planta2.qft.
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

    const double expected = 20.0 * std::log10(1.2); // 1.5836249...
    const qftbx::Specification bound = qftbx::toSpecification(spec, qftbx::SpecificationType::Stability);
    EXPECT_NEAR(bound.boundDb(0.5), expected, 1e-12);
    EXPECT_NEAR(bound.boundDb(50.0), expected, 1e-12); // omega is ignored
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
    EXPECT_NEAR(bound.boundDb(1.0), -0.76398, 1e-4); // hand-checked anchor
}

TEST(Specification, ZeroHeightIsRefused)
{
    // A zero height (the record's default) used to give a -inf bound, so
    // every grid point passed the contour threshold and the boundary
    // silently degenerated to the window frame. It is refused where it is
    // validated.
    qftbx::SpecificationRecord spec = makeConstantStability(0.0);
    EXPECT_THROW(qftbx::toSpecification(spec, qftbx::SpecificationType::Stability),
                 qftbx::InvalidInput);
}

TEST(Specification, NegativeHeightIsRefused)
{
    // A negative height used to give a NaN bound and a boundary that silently
    // came out empty - this test pinned that as a known bug. The raw record's
    // own dB conversion is gone; the bound is asked of the validated
    // Specification, and the validation refuses the height.
    qftbx::SpecificationRecord spec = makeConstantStability(-1.0);
    EXPECT_THROW(qftbx::toSpecification(spec, qftbx::SpecificationType::Stability),
                 qftbx::InvalidInput);
}

TEST(SpecificationDao, OwnsReplacesAndToleratesIdentity)
{
    // The store owns the seven records and their embedded plants. This used
    // to be a pointer to a QVector of pointers, and the test had to check
    // that replacing it deep-deleted the previous set and that handing back
    // the very vector it held was a no-op. Held by value, neither situation
    // can arise: the set replaced is destroyed with its plants, and there is
    // no pointer identity left to confuse.
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
    data.setSpecifications(std::move(second));   // frees 'first' and its plant

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
    EXPECT_EQ(lower.name, std::string("TrackingLower")); // "seguimiento" in the file, mapped on load
    EXPECT_TRUE(lower.used);
    EXPECT_FALSE(lower.constant);
    EXPECT_DOUBLE_EQ(lower.omegaStart, 1.0);
    EXPECT_DOUBLE_EQ(lower.omegaEnd, 18.0);
    ASSERT_NE(lower.system, nullptr);
    EXPECT_EQ(lower.system->type(), LtiSystem::SystemType::PolynomialForm);
    EXPECT_EQ(lower.system->numerator().size(), 2);

    const qftbx::SpecificationRecord & upper = specs->at(1);
    EXPECT_EQ(upper.name, std::string("TrackingUpper")); // "seguimiento_1" in the file, mapped on load
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
    // The recovered plant must evaluate like the analytic reference.
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

    EXPECT_TRUE(specs->at(4).used);   // RPS
    EXPECT_FALSE(specs->at(4).constant);
}

TEST(SpecificationPersistence, WrongSpecificationCountThrowsParseError)
{
    // Hardened: the set is positional with exactly 7 slots and consumers
    // index blindly; a shorter file used to crash out of range downstream.
    ProjectReader parser;
    EXPECT_THROW(parser.load(
                     std::string(QFTBX_TEST_DATA_DIR "/corrupt_specs.qft")),
                 qftbx::ParseError);
}

TEST(QftbxUnits, DbLinearConversionsRoundTrip)
{
    EXPECT_NEAR(qftbx::linearToDb(2.0), 6.02059991, 1e-7);
    EXPECT_NEAR(qftbx::dbToLinear(6.02059991), 2.0, 1e-9);
    EXPECT_NEAR(qftbx::dbToLinear(qftbx::linearToDb(1.2)), 1.2, 1e-12);
    EXPECT_DOUBLE_EQ(qftbx::linearToDb(1.0), 0.0);
}

// ---------------------------------------------------------------------------
// qftbx::Specification - the validated replacement being introduced
// ---------------------------------------------------------------------------

TEST(QftbxSpecification, FactoriesValidateTheirInvariants)
{
    using qftbx::Specification;
    using qftbx::SpecificationType;

    EXPECT_THROW(Specification::constant(SpecificationType::Stability, 0.0, 0.1, 10.0),
                 qftbx::InvalidInput); // the old silent -inf
    EXPECT_THROW(Specification::constant(SpecificationType::Stability, -1.0, 0.1, 10.0),
                 qftbx::InvalidInput); // the old silent NaN
    EXPECT_THROW(Specification::constant(SpecificationType::Stability, 1.2, 10.0, 0.1),
                 qftbx::InvalidInput); // inverted band, previously mute
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
    EXPECT_TRUE(spec.appliesAt(0.1));   // inclusive lower edge
    EXPECT_TRUE(spec.appliesAt(10.0));  // inclusive upper edge
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
    // both destructors run at scope end: leak/double-free checked by ASan
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

    // T_U - T_L in dB: 20log10(2) - 20log10(1) = 6.0206. This is the sign
    // three consumers compute as b-a and contour2 (wrongly) as a-b.
    EXPECT_NEAR(set.trackingSpreadDb(1.0), 20.0 * std::log10(2.0), 1e-12);
}

} // namespace
