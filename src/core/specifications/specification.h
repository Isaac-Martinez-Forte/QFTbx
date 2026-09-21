/**
 * @file
 * @brief One QFT specification and the fixed set of seven.
 *
 * Declares the seven specification slots in the order the .qft files store
 * them positionally, the conversion between dB and linear magnitude, and
 * the specification proper: a magnitude bound, constant or given by a
 * transfer function, over a closed frequency band minus the design
 * frequencies it is told to skip. It is built only through validating
 * factories, so a constant bound is finite and positive, a system bound owns
 * its plant and the band is ordered; a moved-from specification is an
 * unused one. The set indexes the seven by type and gives the tracking
 * spread, upper minus lower in dB, that the tracking boundary is cut at.
 */

#ifndef QFTBX_SPECIFICATION_H
#define QFTBX_SPECIFICATION_H

#include <algorithm>
#include <vector>
#include <array>
#include <cstddef>
#include <cmath>
#include <memory>
#include <utility>

#include <string>

#include "src/core/common/exception.h"
#include "src/core/system/lti_system.h"

namespace qftbx {

/// dB -> linear magnitude.
inline double dbToLinear(double db) { return std::pow(10.0, db / 20.0); }

/// Linear magnitude -> dB.
inline double linearToDb(double linear) { return 20.0 * std::log10(linear); }

/**
 * @brief The seven QFT specification slots, in the order they have always
 * been stored (positional in the .qft files: do not reorder).
 */
enum class SpecificationType {
    TrackingLower,   ///< 0: historical "seguimiento"   (T_L)
    TrackingUpper,   ///< 1: historical "seguimiento_1" (T_U)
    Stability,   ///< 2: historical "estabilidad"
    SensorNoise,   ///< 3: historical "ruido"
    OutputDisturbance,   ///< 4: historical "RPS"
    InputDisturbance,   ///< 5: historical "RPE"
    ControlEffort
};

inline constexpr std::size_t kSpecificationCount = 7;

/// Canonical name, written to the .qft files since the English rename.
inline std::string specificationName(SpecificationType type)
{
    switch (type) {
    case SpecificationType::TrackingLower:     return ("TrackingLower");
    case SpecificationType::TrackingUpper:     return ("TrackingUpper");
    case SpecificationType::Stability:         return ("Stability");
    case SpecificationType::SensorNoise:       return ("SensorNoise");
    case SpecificationType::OutputDisturbance: return ("OutputDisturbance");
    case SpecificationType::InputDisturbance:  return ("InputDisturbance");
    case SpecificationType::ControlEffort:     return ("ControlEffort");
    }
    return std::string();
}

/**
 * @brief One QFT specification: a magnitude bound over a frequency band,
 * minus the frequencies of that band it is told to skip.
 *
 * Built only through the validating factories, so its invariants hold by
 * construction: a constant bound has a finite magnitude > 0 (in linear
 * units), a system bound owns a non-null plant, and the band satisfies
 * 0 <= min <= max. boundDb() therefore never returns -inf or NaN for
 * constant bounds: a magnitude of 0 degenerates the boundary to the window
 * frame, silently.
 *
 * The skipped frequencies are what makes a band expressive enough: a design
 * is done at a discrete set of frequencies, and a requirement that holds
 * everywhere but at one of them is an ordinary thing to ask for. They are
 * an exception list and not the list of frequencies that DO count, so a
 * design frequency added later falls inside the band and counts, which is
 * the answer that does not surprise anybody.
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
    static Specification constant(SpecificationType type, double magnitude,
                                  double minFrequency, double maxFrequency,
                                  std::vector<double> skipped = {})
    {
        if (!(magnitude > 0.0) || !std::isfinite(magnitude)) {
            throw InvalidInput(QFTBX_TR("Core", "A constant specification needs a finite magnitude > 0."));
        }
        validateBand(minFrequency, maxFrequency);

        Specification spec(type);
        spec.m_used = true;
        spec.m_constant = true;
        spec.m_magnitude = magnitude;
        spec.m_minFrequency = minFrequency;
        spec.m_maxFrequency = maxFrequency;
        spec.m_skipped = std::move(skipped);
        return spec;
    }

    /// Bound given by a transfer function, which the specification takes over.
    static Specification fromSystem(SpecificationType type,
                                    std::unique_ptr<LtiSystem> system,
                                    double minFrequency, double maxFrequency,
                                    std::vector<double> skipped = {})
    {
        if (system == nullptr) {
            throw InvalidInput(QFTBX_TR("Core", "A system specification needs a non-null plant."));
        }

        validateBand(minFrequency, maxFrequency);

        Specification spec(type);
        spec.m_used = true;
        spec.m_constant = false;
        spec.m_system = std::move(system);
        spec.m_minFrequency = minFrequency;
        spec.m_maxFrequency = maxFrequency;
        spec.m_skipped = std::move(skipped);
        return spec;
    }

    Specification() = default;

    Specification(Specification&& other) noexcept { *this = std::move(other); }

    /// Hand-written for the one thing the generated version would not do:
    /// a moved-from specification is an UNUSED one.
    Specification& operator=(Specification&& other) noexcept
    {
        if (this != &other) {
            m_type = other.m_type;
            m_used = other.m_used;
            m_constant = other.m_constant;
            m_magnitude = other.m_magnitude;
            m_minFrequency = other.m_minFrequency;
            m_maxFrequency = other.m_maxFrequency;
            m_skipped = std::move(other.m_skipped);
            m_system = std::move(other.m_system);
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
    double boundDb(double omega) const
    {
        if (!m_used) {
            throw InvalidInput(QFTBX_TR("Core", "The %1 specification is not in use, so it has no bound.").arg(name()));
        }
        if (m_constant) {
            return linearToDb(m_magnitude);
        }
        return linearToDb(std::abs(m_system->evaluate(omega)));
    }

    /// used(), minFrequency <= omega <= maxFrequency (closed interval), and
    /// omega not one of the frequencies this specification skips.
    ///
    /// The comparison with the skipped ones is EXACT, and rightly so: they
    /// are design frequencies, chosen from the very vector every consumer
    /// walks, and written to the file at every digit a double has. A
    /// frequency that is not one of those is not one of those.
    bool appliesAt(double omega) const
    {
        if (!m_used || omega < m_minFrequency || omega > m_maxFrequency) {
            return false;
        }

        return std::find(m_skipped.begin(), m_skipped.end(), omega) == m_skipped.end();
    }

    bool used() const { return m_used; }

    bool isConstant() const { return m_constant; }

    SpecificationType type() const { return m_type; }

    std::string name() const { return specificationName(m_type); }

    const LtiSystem* system() const { return m_system.get(); }

    double magnitude() const { return m_magnitude; }

    /// The frequencies of the band this specification does NOT apply at.
    const std::vector<double> & skipped() const { return m_skipped; }

    double minFrequency() const { return m_minFrequency; }

    double maxFrequency() const { return m_maxFrequency; }

private:
    explicit Specification(SpecificationType type) : m_type(type) {}

    static void validateBand(double minFrequency, double maxFrequency)
    {
        if (!std::isfinite(minFrequency) || !std::isfinite(maxFrequency) ||
            minFrequency < 0.0 || maxFrequency < minFrequency) {
            throw InvalidInput(QFTBX_TR("Core", "A specification band needs 0 <= min <= max, finite."));
        }
    }

    SpecificationType m_type = SpecificationType::TrackingLower;
    bool m_used = false;
    bool m_constant = false;
    double m_magnitude = 0.0;
    double m_minFrequency = 0.0;
    double m_maxFrequency = 0.0;

    /// The frequencies of the band this one does not apply at, as an
    /// exception list: empty is the ordinary case and the one every file
    /// written before this existed has.
    std::vector<double> m_skipped;
    std::unique_ptr<LtiSystem> m_system;
};

/**
 * @brief The fixed set of seven specifications, indexed BY TYPE.
 */
class SpecificationSet
{
public:
    SpecificationSet()
    {
        for (std::size_t i = 0; i < kSpecificationCount; ++i) {
            m_slots[i] = Specification::unused(static_cast<SpecificationType>(i));
        }
    }

    const Specification& at(SpecificationType type) const
    {
        return m_slots[static_cast<std::size_t>(type)];
    }

    void set(Specification&& specification)
    {
        const std::size_t index = static_cast<std::size_t>(specification.type());
        m_slots[index] = std::move(specification);
    }

    /**
     * @brief Tracking spread T_U - T_L in dB at omega: the height the
     * tracking boundary cuts at. Centralises the sign that three consumers
     * computed as b-a and one, wrongly, as a-b.
     */
    double trackingSpreadDb(double omega) const
    {
        return at(SpecificationType::TrackingUpper).boundDb(omega) -
               at(SpecificationType::TrackingLower).boundDb(omega);
    }

private:
    std::array<Specification, kSpecificationCount> m_slots;
};

}

#endif
