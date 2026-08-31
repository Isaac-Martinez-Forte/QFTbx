#ifndef QFTBX_FREQUENCIES_OMEGA_H
#define QFTBX_FREQUENCIES_OMEGA_H

#include <QString>
#include <QVector>

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

    Omega(qreal start, qreal end, qint32 pointCount, QVector<qreal> * values, GenerationType type);

    /// Reads a frequency file (values separated by whitespace or newlines).
    /// Throws qftbx::FileError when it cannot be opened or holds no valid
    /// frequency.
    static QVector<qreal> * valuesFromFile(QString path);

    ~Omega();

    qreal start();
    qreal end();
    qint32 pointCount();

    QVector<qreal> * values();

    GenerationType type();

    void setValues(QVector<qreal> * values);

private:
    qreal m_start;
    qreal m_end;
    qint32 m_pointCount;
    QVector<qreal> * m_values;
    GenerationType m_type;
};

#endif // QFTBX_FREQUENCIES_OMEGA_H
