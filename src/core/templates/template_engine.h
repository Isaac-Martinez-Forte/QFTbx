#ifndef QFTBX_TEMPLATE_ENGINE_H
#define QFTBX_TEMPLATE_ENGINE_H

#include <complex>
#include <limits>

#include <QHash>
#include <QString>
#include <QVector>

#include "src/core/system/lti_system.h"
#include "src/core/system/parameter.h"

#include "mpParser.h"

#ifdef OpenMP_AVAILABLE
    #include <omp.h>
#endif

namespace qftbx {

/**
 * @brief Computes QFT templates (plant value sets) and their contours.
 *
 * For each design frequency the brute-force sweep evaluates the plant over
 * the cartesian product of the uncertain-parameter grids (one bound
 * muParserX expression per frequency), and the contour is extracted with the
 * \f$\varepsilon\f$-hull algorithm (Nordin 1993, Montoya's EPSHULL.M
 * implementation): starting from the rightmost point, the walk repeatedly
 * picks the neighbour within \f$\varepsilon\f$ whose circle of radius
 * \f$\varepsilon/2\f$ sticks out of the covered region (minimum
 * \f$\psi\f$ angle), closing when it returns to the initial pair.
 *
 * Known limitation of the reference algorithm, found while porting: on
 * clouds of clusters spaced about \f$\varepsilon\f$ apart the walk cycles
 * without closing; epsilonHull() then falls back to the relaxed historical
 * walk (a valid \f$\varepsilon\f$-cover, not the canonical hull) with a
 * warning.
 *
 * The engine owns none of the data it is given or produces: grids and
 * epsilon belong to the caller, and clouds/contours become property of the
 * template DAO as soon as the controller hands them over.
 */
class TemplateEngine
{
public:

    TemplateEngine();

    ~TemplateEngine();

    /// Sweeps the plant and extracts every contour. Throws qftbx::Exception
    /// on invalid input or when a computation fails.
    bool compute(LtiSystem *plant, QVector<qreal>* frequencies, bool cuda);

    /// Recomputes only the contours (one epsilon per frequency) over the
    /// current clouds.
    bool computeContours (QVector <qreal> * epsilon);

    /// Brute-force sweep: one cloud per frequency, the cartesian product of
    /// the parameter grids evaluated at s = j*omega.
    QVector<QVector<std::complex<qreal> > *> * computeClouds(LtiSystem *plant, QVector<qreal>* frequencies);

    bool computeContourSet(bool cuda);

    /// Frees the partially built contour row on a failure: see the note on
    /// the definition.
    void discardContours();

    /**
     * @brief Epsilon-hull contour of a point cloud, faithful to EPSHULL.M:
     * unique()d input in MATLAB complex order, max-real starting point, the
     * previous point stays a candidate (spikes are traversed both ways) and
     * the returned contour is closed (last point repeats the first).
     *
     * Returns null when no candidate lies within epsilon of the start; when
     * the reference walk cycles, falls back to the relaxed historical walk
     * (open, deduplicated, max-imaginary start).
     */
    /**
     * @brief The epsilon-hull contour of one cloud.
     *
     * @param fellBack when not null, set to true if the faithful walk did
     * not close and the relaxed historical walk was used instead. Reported
     * by the CALLER, after the parallel loop: warning from inside an OpenMP
     * region raced on the message handler (helgrind), and it is the same
     * non-local action from within a parallel region that once let a
     * muParserX error terminate the process.
     */
    QVector <std::complex <qreal> > * epsilonHull(QVector<std::complex<qreal> > *cloud, qreal epsilon,
                                                  bool * fellBack = nullptr);

    /// Sweep grids keyed by parameter NAME; the caller keeps ownership.
    void setGrids (QHash<QString, QVector<qreal> *> * grids);

    /// One epsilon per frequency; the caller keeps ownership.
    void setEpsilon (QVector <qreal> * epsilon);

    /// Feeds precomputed clouds (e.g. loaded from a project file) so their
    /// contours can be recomputed.
    void setClouds (QVector<QVector<std::complex<qreal> > *> * clouds);

    QVector<QVector<std::complex<qreal> > *> * clouds();

    QVector<QVector<std::complex<qreal> > *> * contours();

    QVector <qreal> * omega();

    QVector <qreal> * epsilon ();

private:
    /// Grid for an uncertain parameter, looked up by name; throws
    /// qftbx::InvalidInput naming the parameter when the grid is missing.
    QVector<qreal> * gridFor(Parameter & a);

    //The engine owns NOTHING below: grids and epsilon belong to the caller,
    //clouds/contours to the template DAO once handed over.
    QHash <QString, QVector<qreal> * > * m_grids = NULL;
    qint32 m_combinationCount = 0;
    QVector <qreal> * m_epsilon = NULL;
    bool m_useCuda = false;

    QVector<QVector<std::complex<qreal> > *> * m_clouds = NULL;
    QVector<QVector<std::complex<qreal> > *> * m_contours = NULL;
    QVector <qreal> * m_frequencies = NULL;

    qint32 findSecond(qint32 b1, QVector<std::complex<qreal> > *cv, qreal epsilon);

    /// excludePrevious = true reproduces the relaxed historical variant;
    /// false is the behaviour faithful to EPSHULL.M.
    qint32 findNext(qint32 previousPoint, qint32 currentPoint, QVector<std::complex<qreal> > *cv, qreal epsilon,
                           bool excludePrevious = false);

    /// Historical PFC walk (divergent from EPSHULL.M): max-imaginary start,
    /// previous point excluded, silent truncation at MAXP, deduplicated
    /// output. Used as the fallback when the reference walk cycles: it
    /// always yields a contour with coverage <= epsilon.
    QVector <std::complex <qreal> > * epsilonHullRelaxed(QVector<std::complex<qreal> > *cloud, qreal epsilon);

};

} // namespace qftbx

//Transitional: unqualified name for consumers not yet migrated
//to the qftbx namespace. Remove when the migration is complete.
using qftbx::TemplateEngine;

#endif // QFTBX_TEMPLATE_ENGINE_H
