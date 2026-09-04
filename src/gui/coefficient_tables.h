#ifndef QFTBX_GUI_COEFFICIENT_TABLES_H
#define QFTBX_GUI_COEFFICIENT_TABLES_H

#include <QString>
#include <string>
#include <vector>

#include <QVector>

/**
 * @file
 * @brief The coefficient tables the plant and controller dialogs read out of
 * their line edits and the uncertainty dialog edits.
 *
 * Three parallel tables with one ROW per polynomial slot, in the order the
 * dialogs fill them (numerator, denominator, gain, delay) and one entry per
 * coefficient: the values, the expressions the user typed, and whether each
 * coefficient is an uncertain parameter.
 *
 * By value. They used to be `QVector<QVector<QString> *> *`, a pointer to a
 * vector of pointers, and three different files carried the same
 * releaseTables() helper to walk and free them - one of which had to skip a
 * table that might be null.
 */

/// The texts of one polynomial slot's coefficients.
///
/// QString and not std::string, having tried it the other way: these rows
/// are filled from QLineEdits and read back into them all over the dialogs,
/// so std::string moved the conversion to a hundred widget calls to save it
/// at eight core calls. The seam belongs where the core is entered.
using CoefficientRow = std::vector<QString>;

/// The slots of one system, in dialog order.
using CoefficientTable = std::vector<CoefficientRow>;

/// Whether each coefficient of one slot is an uncertain parameter.
using UncertainRow = std::vector<bool>;

/// The uncertainty flags of one system, aligned with a CoefficientTable.
using UncertainTable = std::vector<UncertainRow>;

#endif // QFTBX_GUI_COEFFICIENT_TABLES_H
