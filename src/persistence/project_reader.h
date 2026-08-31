#ifndef QFTBX_PROJECT_READER_H
#define QFTBX_PROJECT_READER_H

#include <complex>

#include <QString>
#include <QVector>

#include "src/core/system/lti_system.h"
#include "src/core/boundaries/boundary_data.h"
#include "Modelo/EstructurasDatos/dbnd.h"
#include "Modelo/EstructurasDatos/datosloopshaping.h"
#include "Modelo/Objetos/omega.h"

namespace qftbx {

/**
 * @brief Loads a .qft project file (pugixml DOM).
 *
 * Reads the legacy dialect (Spanish tags, no version attribute) and, from
 * version 2 on, the English dialect. Sections may appear in any order and
 * any subset; load() reports which ones were found. Malformed content
 * throws qftbx::ParseError with the offending line; a missing or unreadable
 * file throws qftbx::FileError.
 *
 * Ownership is transitional, mirroring the old parser: the caller takes the
 * loaded objects through the section getters.
 */
class ProjectReader
{
public:
    ProjectReader();

    /**
     * @brief Parses the file and returns the section-presence flags, in the
     * historical order: plant, specifications, omega, templates, boundaries,
     * controller, loop shaping, template contour.
     */
    QVector <bool> * load(const QString & filePath);

    LtiSystem * plant() const { return m_plant; }
    QVector <tools::dBND *> * specifications() const { return m_specifications; }
    Omega * omega() const { return m_omega; }
    QVector <QVector <std::complex<qreal>> * > * templates() const { return m_templates; }
    QVector <QVector <std::complex<qreal>> * > * contour() const { return m_contour; }
    QVector <qreal> * epsilon() const { return m_epsilon; }
    BoundaryData * boundaries() const { return m_boundaries; }
    LtiSystem * controller() const { return m_controller; }
    DatosLoopShaping * loopShaping() const { return m_loopShaping; }

private:
    LtiSystem * m_plant = nullptr;
    QVector <tools::dBND *> * m_specifications = nullptr;
    Omega * m_omega = nullptr;
    QVector <QVector <std::complex<qreal>> * > * m_templates = nullptr;
    QVector <QVector <std::complex<qreal>> * > * m_contour = nullptr;
    QVector <qreal> * m_epsilon = nullptr;
    BoundaryData * m_boundaries = nullptr;
    LtiSystem * m_controller = nullptr;
    DatosLoopShaping * m_loopShaping = nullptr;
};

} // namespace qftbx

//Transitional: consumers still refer to the class unqualified.
using qftbx::ProjectReader;

#endif // QFTBX_PROJECT_READER_H
