#ifndef QFTBX_FREQUENCIES_OMEGA_H
#define QFTBX_FREQUENCIES_OMEGA_H

#include <cstdint>
#include <string>
#include <vector>

/**
 * @class Omega
 * @brief The set of design frequencies of a QFT problem.
 *
 * Holds the frequency values and how they were generated (the generation
 * parameters are kept so the GUI can re-open its dialog pre-filled and the
 * persistence can round-trip them).
 *
 * @author Isaac Martínez Forte
 */
class Omega
{
public:

    /// How the design frequencies were generated. The numeric values are
    /// serialized in the .qft files: do not reorder.
    enum GenerationType {LinSpace, LogSpace, Manual, File};

    /**
     * @brief Builds the set. Throws InvalidInput when the values are empty or
     * when any of them is not a finite, strictly positive frequency: a
     * frequency file is user input arriving through a path nobody checks, and
     * strtod reads "nan" and "-1" as numbers.
     *
     * pointCount is accepted and IGNORED: the count is the size of the
     * values, and old files carried one that disagreed with them.
     */
    Omega(double start, double end, std::int32_t pointCount, std::vector<double> values, GenerationType type);

    /// Reads a frequency file (values separated by whitespace or newlines).
    /// Throws qftbx::FileError when it cannot be opened or holds no valid
    /// frequency.
    static std::vector<double> valuesFromFile(std::string path);

    double start() const;
    double end() const;
    std::int32_t pointCount() const;

    /// Observer on the frequencies, which the set holds by value.
    std::vector<double> * values();
    const std::vector<double> * values() const;

    GenerationType type() const;

    void setOmega(std::vector<double> values);

    /**
     * @brief Value equality: the values, and the description they came from.
     *
     * Conservative like LtiSystem::sameAs, and for the same reason - a wrong
     * "equal" leaves templates computed for a DIFFERENT frequency set in
     * place. The start, the end and the type are compared as well as the
     * values, even though the values are what the sweep reads: two sets with
     * the same values but a different generation type are not
     * interchangeable, because bode_viewer re-derives its own sweep from the
     * start, the end and the type.
     */
    bool sameAs(const Omega & other) const;

private:
    double m_start;
    double m_end;
    std::int32_t m_pointCount;
    std::vector<double> m_values;
    GenerationType m_type;
};

#endif // QFTBX_FREQUENCIES_OMEGA_H
