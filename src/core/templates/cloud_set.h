#ifndef QFTBX_CLOUD_SET_H
#define QFTBX_CLOUD_SET_H

#include <complex>
#include <vector>

namespace qftbx {

/// The value set of the plant at ONE design frequency: one point per
/// combination of the parameter grids, in the complex plane.
using ComplexCloud = std::vector<std::complex<double>>;

/**
 * @brief One cloud per design frequency: a template set, or a contour set.
 *
 * Held BY VALUE. This travelled as
 * `QVector<QVector<std::complex<qreal>> *> *` - a pointer to a vector of
 * pointers to vectors - through the engine, the project store, the facade,
 * the persistence and the viewers, and at every hop the question of who
 * frees it had a different answer. The engine computed them and kept a
 * pointer; the store took ownership when the facade published them; the
 * engine's own destructor was empty, so a computation whose result nobody
 * published leaked the lot; and the tests that drive the engine directly
 * leaked it every single time.
 *
 * A vector of vectors answers all of that by not asking: the owner is
 * whoever holds the object.
 */
using CloudSet = std::vector<ComplexCloud>;

} // namespace qftbx

#endif // QFTBX_CLOUD_SET_H
