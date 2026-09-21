/**
 * @file
 * @brief The execution stages of the MC family of algorithms.
 *
 * Area bisection, then tree bisection, then cuts disabled, as the thesis
 * prescribes in section 4.4.
 */

#ifndef QFTBX_LOOPSHAPING_STAGES_H
#define QFTBX_LOOPSHAPING_STAGES_H

namespace qftbx {

enum class Stage {
    Initial, Intermediate, Final
};

}

#endif
