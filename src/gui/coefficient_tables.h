#ifndef QFTBX_GUI_COEFFICIENT_TABLES_H
#define QFTBX_GUI_COEFFICIENT_TABLES_H

#include <QString>
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

/// One polynomial slot's coefficients.
using CoefficientRow = QVector<QString>;

/// The slots of one system, in dialog order.
using CoefficientTable = QVector<CoefficientRow>;

/// Whether each coefficient of one slot is an uncertain parameter.
using UncertainRow = QVector<bool>;

/// The uncertainty flags of one system, aligned with a CoefficientTable.
using UncertainTable = QVector<UncertainRow>;

#endif // QFTBX_GUI_COEFFICIENT_TABLES_H
