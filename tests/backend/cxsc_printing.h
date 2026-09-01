#ifndef QFTBX_TESTS_CXSC_PRINTING_H
#define QFTBX_TESTS_CXSC_PRINTING_H

// GoogleTest prints the operands of a FAILED comparison. For a cxsc::real it
// reached C-XSC's own number formatting (r_outpx -> b_out), which dereferences
// a null pointer and takes the process down with SIGSEGV: a failing interval
// assertion killed the binary instead of reporting what it found, which is the
// worst possible moment to lose the message. Reproduced natively, not only
// under valgrind.
//
// These overloads are found by GoogleTest through argument-dependent lookup,
// so every current and future test that compares C-XSC values prints through
// _double() instead. The crash inside libcxsc is a separate matter, noted for
// the C-XSC review (phase 8c); this keeps the suite able to report.

#include <ostream>

#include "cinterval.hpp"
#include "complex.hpp"
#include "interval.hpp"
#include "real.hpp"

namespace cxsc {

inline void PrintTo(const real & value, std::ostream * out)
{
    *out << _double(value);
}

inline void PrintTo(const interval & value, std::ostream * out)
{
    *out << "[" << _double(Inf(value)) << ", " << _double(Sup(value)) << "]";
}

inline void PrintTo(const complex & value, std::ostream * out)
{
    *out << "(" << _double(Re(value)) << " + " << _double(Im(value)) << "i)";
}

inline void PrintTo(const cinterval & value, std::ostream * out)
{
    *out << "([" << _double(InfRe(value)) << ", " << _double(SupRe(value)) << "] + ["
         << _double(InfIm(value)) << ", " << _double(SupIm(value)) << "]i)";
}

} // namespace cxsc

#endif // QFTBX_TESTS_CXSC_PRINTING_H
