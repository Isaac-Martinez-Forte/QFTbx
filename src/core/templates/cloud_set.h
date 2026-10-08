/**
 * @file
 * @brief The clouds of a template computation, one per design frequency.
 *
 * A cloud is the plant values at one frequency as complex numbers, one
 * point per combination of the parameter grids, and a set of them is
 * either the templates themselves or their contours. Clouds and sets are
 * held by value: the owner is whoever holds the object.
 */

#ifndef QFTBX_CLOUD_SET_H
#define QFTBX_CLOUD_SET_H

#include <complex>
#include <vector>

namespace qftbx {

using ComplexCloud = std::vector<std::complex<double>>;

using CloudSet = std::vector<ComplexCloud>;

}

#endif
