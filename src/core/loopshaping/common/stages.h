#ifndef QFTBX_LOOPSHAPING_STAGES_H
#define QFTBX_LOOPSHAPING_STAGES_H

/// Execution stages of the MC family of algorithms (thesis sec. 4.4):
/// area bisection, then tree bisection, then cuts disabled.
namespace qftbx {

enum class Stage {
    Initial, Intermediate, Final
};

} // namespace qftbx

#endif // QFTBX_LOOPSHAPING_STAGES_H
