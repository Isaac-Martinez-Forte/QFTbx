/**
 * @file
 * @brief Clouds kept on a card, as the core sees them.
 *
 * An engine that sweeps on a card leaves the clouds there, and one that
 * reads them on the same card takes them instead of uploading them again.
 * The core only hands them from one to the other: what they hold is the
 * GPU code's, and they say only how many frequencies and plants they have.
 */

#ifndef QFTBX_RESIDENT_CLOUDS_H
#define QFTBX_RESIDENT_CLOUDS_H

#include <cstddef>

namespace qftbx {

class ResidentClouds
{
public:
    virtual ~ResidentClouds() = default;

    virtual std::size_t frequencies() const = 0;
    virtual std::size_t plants() const = 0;
};

}

#endif
