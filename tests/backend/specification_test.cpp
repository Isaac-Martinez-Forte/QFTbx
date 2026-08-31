// Characterisation tests for the QFT specification record (tools::dBND, in
// its transitional home) and its persistence, pinning the current behaviour
// before the phase-4 move to qftbx::Specification. "// BUG:" cases document
// known defects and must be flipped by the fix that closes them.

#include <gtest/gtest.h>

#include <cmath>
#include <complex>

#include <QString>
#include <QVector>

#include "Modelo/EstructurasDatos/dbnd.h"
#include "src/core/exception.h"
#include "src/core/specifications/specification.h"
#include "DAO/adaptadorespecificacionesdao.h"
#include "src/core/system/polynomial_form.h"
#include "src/core/system/parameter.h"
#include "src/persistence/project_reader.h"

namespace {

using Complex = std::complex<qreal>;

tools::dBND makeConstantStability(qreal linearHeight)
{
    tools::dBND spec{};
    spec.nombre = QStringLiteral("estabilidad");
    spec.utilizado = true;
    spec.constante = true;
    spec.sistema = nullptr;
    spec.altura = linearHeight; // linear magnitude, not dB
    spec.frecinicio = 0.1;
    spec.frecfinal = 100.0;
    return spec;
}

// 120 / (s^3 + 17 s^2 + 82 s + 120): the tracking specification plant of
// tests/data/planta2.qft.
LtiSystem* makeTrackingPlant()
{
    auto* num = new QVector<Parameter*>{new Parameter(120.0)};
    auto* den = new QVector<Parameter*>{new Parameter(1.0), new Parameter(17.0),
                                        new Parameter(82.0), new Parameter(120.0)};
    return new PolynomialForm(QStringLiteral("seguimiento"), num, den,
                              new Parameter(1.0), new Parameter(0.0));
}

qreal analyticTrackingDb(qreal w)
{
    const Complex s(0.0, w);
    const Complex value = 120.0 / (s * s * s + 17.0 * s * s + 82.0 * s + 120.0);
    return 20.0 * std::log10(std::abs(value));
}

TEST(Specification, ConstantHeightIsDbAndIgnoresOmega)
{
    tools::dBND spec = makeConstantStability(1.2);

    const qreal expected = 20.0 * std::log10(1.2); // 1.5836249...
    EXPECT_NEAR(spec.getAltura(0.5), expected, 1e-12);
    EXPECT_NEAR(spec.getAltura(50.0), expected, 1e-12); // omega is ignored
}

TEST(Specification, SystemHeightMatchesTheAnalyticValue)
{
    tools::dBND spec{};
    spec.nombre = QStringLiteral("seguimiento");
    spec.utilizado = true;
    spec.constante = false;
    spec.sistema = makeTrackingPlant();
    spec.frecinicio = 0.1;
    spec.frecfinal = 10.0;

    EXPECT_NEAR(spec.getAltura(1.0), analyticTrackingDb(1.0), 1e-9);
    EXPECT_NEAR(spec.getAltura(1.0), -0.76398, 1e-4); // hand-checked anchor
    delete spec.sistema;
}

TEST(Specification, ZeroHeightYieldsMinusInfinity)
{
    // BUG: altura == 0 (the default!) gives -inf, and every grid point then
    // passes the contour threshold: the boundary silently degenerates to
    // the window frame. Will become an InvalidInput at construction.
    tools::dBND spec = makeConstantStability(0.0);
    EXPECT_TRUE(std::isinf(spec.getAltura(1.0)));
}

TEST(Specification, NegativeHeightYieldsNaN)
{
    // BUG: altura < 0 gives NaN and the boundary silently comes out empty.
    tools::dBND spec = makeConstantStability(-1.0);
    EXPECT_TRUE(std::isnan(spec.getAltura(1.0)));
}

TEST(SpecificationDao, OwnsReplacesAndToleratesIdentity)
{
    // Fixed: the DAO now owns the records and their embedded plants (deep
    // deletes on replacement and destruction, leak-checked under ASan);
    // handing it the vector it already holds is a no-op.
    auto* first = new QVector<tools::dBND*>();
    for (int i = 0; i < 7; ++i) {
        auto* spec = new tools::dBND{};
        if (i == 0) {
            spec->utilizado = true;
            spec->constante = false;
            spec->sistema = makeTrackingPlant(); // deep-owned
        }
        first->append(spec);
    }

    AdaptadorEspecificacionesDAO dao;
    dao.setEspecificaciones(first);
    dao.setEspecificaciones(first); // identity: must not double-delete
    EXPECT_EQ(dao.getEspecificaciones(), first);

    auto* second = new QVector<tools::dBND*>();
    second->append(new tools::dBND{});
    dao.setEspecificaciones(second); // deep-deletes 'first' and its plant
    EXPECT_EQ(dao.getEspecificaciones(), second);
    // 'second' is deep-deleted by the DAO's destructor.
}

TEST(SpecificationPersistence, MultivaluadosSpecificationsRoundTrip)
{
    ProjectReader parser;
    delete parser.load(
        QStringLiteral(QFTBX_TEST_DATA_DIR "/multivaluados.qft"));

    QVector<tools::dBND*>* specs = parser.specifications();
    ASSERT_NE(specs, nullptr);
    ASSERT_EQ(specs->size(), 7);

    tools::dBND* lower = specs->at(0);
    EXPECT_EQ(lower->nombre, QStringLiteral("TrackingLower")); // "seguimiento" in the file, mapped on load
    EXPECT_TRUE(lower->utilizado);
    EXPECT_FALSE(lower->constante);
    EXPECT_DOUBLE_EQ(lower->frecinicio, 1.0);
    EXPECT_DOUBLE_EQ(lower->frecfinal, 18.0);
    ASSERT_NE(lower->sistema, nullptr);
    EXPECT_EQ(lower->sistema->type(), LtiSystem::SystemType::PolynomialForm);
    EXPECT_EQ(lower->sistema->numerator()->size(), 2);

    tools::dBND* upper = specs->at(1);
    EXPECT_EQ(upper->nombre, QStringLiteral("TrackingUpper")); // "seguimiento_1" in the file, mapped on load
    EXPECT_TRUE(upper->utilizado);
    ASSERT_NE(upper->sistema, nullptr);
    EXPECT_EQ(upper->sistema->numerator()->size(), 3);

    for (int i = 2; i < 7; ++i) {
        EXPECT_FALSE(specs->at(i)->utilizado) << "index " << i;
    }
}

TEST(SpecificationPersistence, Planta2RecoversBothTrackingPlants)
{
    ProjectReader parser;
    delete parser.load(
        QStringLiteral(QFTBX_TEST_DATA_DIR "/planta2.qft"));

    QVector<tools::dBND*>* specs = parser.specifications();
    ASSERT_NE(specs, nullptr);
    ASSERT_EQ(specs->size(), 7);

    tools::dBND* lower = specs->at(0);
    ASSERT_NE(lower->sistema, nullptr);
    EXPECT_EQ(lower->sistema->type(), LtiSystem::SystemType::PolynomialForm);
    EXPECT_DOUBLE_EQ(lower->frecinicio, 0.1);
    EXPECT_DOUBLE_EQ(lower->frecfinal, 10.0);
    // The recovered plant must evaluate like the analytic reference.
    EXPECT_NEAR(lower->getAltura(1.0), analyticTrackingDb(1.0), 1e-9);

    tools::dBND* upper = specs->at(1);
    ASSERT_NE(upper->sistema, nullptr);
    EXPECT_EQ(upper->sistema->type(), LtiSystem::SystemType::FreeForm);
}

TEST(SpecificationPersistence, Planta1RecoversTheConstantStability)
{
    ProjectReader parser;
    delete parser.load(
        QStringLiteral(QFTBX_TEST_DATA_DIR "/planta1.qft"));

    QVector<tools::dBND*>* specs = parser.specifications();
    ASSERT_NE(specs, nullptr);
    ASSERT_EQ(specs->size(), 7);

    tools::dBND* stability = specs->at(2);
    EXPECT_TRUE(stability->utilizado);
    EXPECT_TRUE(stability->constante);
    EXPECT_DOUBLE_EQ(stability->altura, 1.2);
    EXPECT_NEAR(stability->getAltura(3.0), 20.0 * std::log10(1.2), 1e-12);

    EXPECT_TRUE(specs->at(4)->utilizado);   // RPS
    EXPECT_FALSE(specs->at(4)->constante);
}

TEST(SpecificationPersistence, WrongSpecificationCountThrowsParseError)
{
    // Hardened: the set is positional with exactly 7 slots and consumers
    // index blindly; a shorter file used to crash out of range downstream.
    ProjectReader parser;
    EXPECT_THROW(parser.load(
                     QStringLiteral(QFTBX_TEST_DATA_DIR "/corrupt_specs.qft")),
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
