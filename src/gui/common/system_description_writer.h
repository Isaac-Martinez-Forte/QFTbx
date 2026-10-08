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

struct SystemDescription
{
    QString name;
    LtiSystem::SystemType type = LtiSystem::SystemType::PolynomialForm;
    QString numerator;
    QString denominator;
    QString gain;
    QString delay;
};

SystemDescription describeSystem(LtiSystem & system);

}

#endif
