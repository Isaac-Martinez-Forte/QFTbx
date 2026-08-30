#ifndef QFTBX_SPECIFICATION_H
#define QFTBX_SPECIFICATION_H

#include <array>
#include <cmath>
#include <utility>

#include <QString>

#include "Modelo/Herramientas/exception.h"
#include "src/core/system/lti_system.h"

namespace qftbx {

/// dB -> linear magnitude.
inline qreal dbToLinear(qreal db) { return std::pow(10.0, db / 20.0); }

/// Linear magnitude -> dB.
inline qreal linearToDb(qreal linear) { return 20.0 * std::log10(linear); }

/**
 * @brief The seven QFT specification slots, in the order they have always
 * been stored (positional in the .qft files: do not reorder).
 */
enum class SpecificationType {
    TrackingLower,      // 0: historical "seguimiento"   (T_L)
    TrackingUpper,      // 1: historical "seguimiento_1" (T_U)
    Stability,          // 2: historical "estabilidad"
    SensorNoise,        // 3: historical "ruido"
    OutputDisturbance,  // 4: historical "RPS"
    InputDisturbance,   // 5: historical "RPE"
    ControlEffort       // 6: historical "EC"
};

inline constexpr int kSpecificationCount = 7;

/// Canonical name, matching the historical strings of the .qft files.
inline QString specificationName(SpecificationType type)
{
    switch (type) {
    case SpecificationType::TrackingLower:     return QStringLiteral("seguimiento");
    case SpecificationType::TrackingUpper:     return QStringLiteral("seguimiento_1");
    case SpecificationType::Stability:         return QStringLiteral("estabilidad");
    case SpecificationType::SensorNoise:       return QStringLiteral("ruido");
    case SpecificationType::OutputDisturbance: return QStringLiteral("RPS");
    case SpecificationType::InputDisturbance:  return QStringLiteral("RPE");
    case SpecificationType::ControlEffort:     return QStringLiteral("EC");
    }
    return QString();
}

/**
 * @brief One QFT specification: a magnitude bound over a frequency band.
 *
 * Built only through the validating factories, so its invariants hold by
 * construction: a constant bound has a finite magnitude > 0 (in linear
 * units), a system bound owns a non-null plant, and the band satisfies
 * 0 <= min <= max. boundDb() therefore never returns -inf or NaN for
 * constant bounds (the historical record accepted magnitude 0 - its
 * default! - and the boundary silently degenerated to the window frame).
 */
class Specification
{
public:
    /// An unused slot: no bound, no band, appliesAt() is always false.
    static Specification unused(SpecificationType type)
    {
        return Specification(type);
    }

    /// Constant bound; magnitude in linear units (not dB), > 0.
    static Specification constant(SpecificationType type, qreal magnitude,
                                  qreal minFrequency, qreal maxFrequency)
    {
        if (!(magnitude > 0.0) || !std::isfinite(magnitude)) {
            throw InvalidInput("A constant specification needs a finite magnitude > 0.");
        }
        validateBand(minFrequency, maxFrequency);

        Specification spec(type);
        spec.m_used = true;
        spec.m_constant = true;
        spec.m_magnitude = magnitude;
        spec.m_minFrequency = minFrequency;
        spec.m_maxFrequency = maxFrequency;
        return spec;
    }

    /// Bound given by a transfer function; takes ownership of system.
    static Specification fromSystem(SpecificationType type, LtiSystem* system,
                                    qreal minFrequency, qreal maxFrequency)
    {
        if (system == nullptr) {
            throw InvalidInput("A system specification needs a non-null plant.");
        }
        try {
            validateBand(minFrequency, maxFrequency);
        } catch (...) {
            delete system;
            throw;
        }

        Specification spec(type);
        spec.m_used = true;
        spec.m_constant = false;
        spec.m_system = system;
        spec.m_minFrequency = minFrequency;
        spec.m_maxFrequency = maxFrequency;
        return spec;
    }

    Specification() = default;

    ~Specification() { delete m_system; }

    Specification(Specification&& other) noexcept { *this = std::move(other); }

    Specification& operator=(Specification&& other) noexcept
    {
        if (this != &other) {
            delete m_system;
            m_type = other.m_type;
            m_used = other.m_used;
            m_constant = other.m_constant;
            m_magnitude = other.m_magnitude;
            m_minFrequency = other.m_minFrequency;
            m_maxFrequency = other.m_maxFrequency;
            m_system = other.m_system;
            other.m_system = nullptr;
            other.m_used = false;
        }
        return *this;
    }

    Specification(const Specification&) = delete;
    Specification& operator=(const Specification&) = delete;

    /**
     * @brief The bound in DECIBELS (the unit every consumer must cut at).
     * Constant: 20*log10(magnitude), omega is ignored. System:
     * 20*log10(|H(j*omega)|).
     */
    qreal boundDb(qreal omega) const
    {
        if (m_constant) {
            return 20.0 * std::log10(m_magnitude);
        }
        return 20.0 * std::log10(std::abs(m_system->evaluate(omega)));
    }

    /// used() and minFrequency <= omega <= maxFrequency (closed interval).
    bool appliesAt(qreal omega) const
    {
        return m_used && m_minFrequency <= omega && omega <= m_maxFrequency;
    }

    bool used() const { return m_used; }

    bool isConstant() const { return m_constant; }

    SpecificationType type() const { return m_type; }

    QString name() const { return specificationName(m_type); }

    const LtiSystem* system() const { return m_system; }

    qreal magnitude() const { return m_magnitude; }

    qreal minFrequency() const { return m_minFrequency; }

    qreal maxFrequency() const { return m_maxFrequency; }

private:
    explicit Specification(SpecificationType type) : m_type(type) {}

    static void validateBand(qreal minFrequency, qreal maxFrequency)
    {
        if (!std::isfinite(minFrequency) || !std::isfinite(maxFrequency) ||
            minFrequency < 0.0 || maxFrequency < minFrequency) {
            throw InvalidInput("A specification band needs 0 <= min <= max, finite.");
        }
    }

    SpecificationType m_type = SpecificationType::TrackingLower;
    bool m_used = false;
    bool m_constant = false;
    qreal m_magnitude = 0.0;
    qreal m_minFrequency = 0.0;
    qreal m_maxFrequency = 0.0;
    LtiSystem* m_system = nullptr;
};

/**
 * @brief The fixed set of seven specifications, indexed by type (the
 * historical code held a positional QVector of 7 with no size checks and a
 * magic "seguimiento" string as the type discriminant).
 */
class SpecificationSet
{
public:
    SpecificationSet()
    {
        for (int i = 0; i < kSpecificationCount; ++i) {
            m_slots[i] = Specification::unused(static_cast<SpecificationType>(i));
        }
    }

    const Specification& at(SpecificationType type) const
    {
        return m_slots[static_cast<int>(type)];
    }

    void set(Specification&& specification)
    {
        const int index = static_cast<int>(specification.type());
        m_slots[index] = std::move(specification);
    }

    /**
     * @brief Tracking spread T_U - T_L in dB at omega: the height the
     * tracking boundary cuts at. Centralises the sign that three consumers
     * computed as b-a and one, wrongly, as a-b.
     */
    qreal trackingSpreadDb(qreal omega) const
    {
        return at(SpecificationType::TrackingUpper).boundDb(omega) -
               at(SpecificationType::TrackingLower).boundDb(omega);
    }

private:
    std::array<Specification, kSpecificationCount> m_slots;
};

} // namespace qftbx

#endif // QFTBX_SPECIFICATION_H
