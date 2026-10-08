#ifndef QFTBX_GUI_COEFFICIENT_TABLES_H
#define QFTBX_GUI_COEFFICIENT_TABLES_H

#include <QString>
#include <string>
#include <vector>

#include <QVector>

/**
 * @file
 * @brief The coefficient tables the plant and controller forms read out of
 * their line edits and the uncertainty dialog edits.
 *
 * Three parallel tables with one ROW per polynomial slot, in the order the
 * forms fill them (numerator, denominator, gain, delay) and one entry per
 * coefficient: the values, the expressions the user typed, and whether each
 * coefficient is an uncertain parameter. An UncertainTable is aligned with
 * its CoefficientTable.
 *
 * By value, so that no caller carries a helper to walk and free them. The
 * texts are QString and not std::string on purpose: the rows are filled
 * from line edits and read back into them all over the forms, so the
 * conversion belongs where the core is entered, not here.
 */

namespace qftbx {

using CoefficientRow = std::vector<QString>;

using CoefficientTable = std::vector<CoefficientRow>;

using UncertainRow = std::vector<bool>;

using UncertainTable = std::vector<UncertainRow>;

}

#endif
