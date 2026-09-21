/**
 * @file
 * @brief A system written back as the text it would be typed as.
 *
 * The inverse of the reader: a project carries systems, not the text that
 * described them, so a form opened over a loaded project shows what it was
 * given by writing it out again. An uncertain coefficient comes back as its
 * name, since its interval lives in the uncertainty dialog, and a fixed one
 * as its value; a free-form system gives back its two expressions.
 */

#ifndef QFTBX_GUI_SYSTEM_DESCRIPTION_WRITER_H
#define QFTBX_GUI_SYSTEM_DESCRIPTION_WRITER_H

#include <QString>

#include "src/core/system/lti_system.h"

namespace qftbx {

/**
 * @brief A system written back as the text it would be typed as: the
 * inverse of SystemDescriptionReader.
 *
 * A project carries systems, not the text somebody typed to describe them,
 * so a form opened over a loaded project can only show what it was given by
 * writing it out again. An uncertain coefficient comes back as its NAME,
 * which is what the field holds - its interval lives in the uncertainty
 * dialog - and a fixed one as its value.
 */
struct SystemDescription
{
    QString name;
    LtiSystem::SystemType type = LtiSystem::SystemType::PolynomialForm;
    QString numerator;
    QString denominator;
    QString gain;
    QString delay;
};

/// The description of a system. Free-form systems give back the two
/// expressions they were written with, the rest their coefficients.
SystemDescription describeSystem(LtiSystem & system);

}

#endif
