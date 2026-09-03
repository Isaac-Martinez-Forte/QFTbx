#ifndef QFTBX_FREQUENCIES_OMEGA_H
#define QFTBX_FREQUENCIES_OMEGA_H

#include <cstdint>
#include <QString>
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

    Omega(double start, double end, std::int32_t pointCount, std::vector<double> values, GenerationType type);

    /// Reads a frequency file (values separated by whitespace or newlines).
    /// Throws qftbx::FileError when it cannot be opened or holds no valid
    /// frequency.
    static std::vector<double> valuesFromFile(QString path);

    double start();
    double end();
    std::int32_t pointCount();

    /// Observer on the frequencies, which the set holds by value.
    std::vector<double> * values();

    GenerationType type();

    void setOmega(std::vector<double> values);

private:
    double m_start;
    double m_end;
    std::int32_t m_pointCount;
    std::vector<double> m_values;
    GenerationType m_type;
};

#endif // QFTBX_FREQUENCIES_OMEGA_H
