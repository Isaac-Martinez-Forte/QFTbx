/**
 * @file
 * @brief One row of the uncertainty table: the line edits of a parameter's minimum, maximum and nominal.
 *
 * A plain aggregate of observers. The widgets belong to the form that
 * created them; the row only remembers where they are so the form can read
 * a parameter back without walking the layout. Nothing here frees a widget:
 * a setter that deleted the one it replaces would leave the layout with a
 * dangling child.
 */

#ifndef PARLABEL_H
#define PARLABEL_H

#include <QVector>
#include <QLineEdit>

/**
 * @brief One row of the uncertainty table: the three line edits that hold a
 * parameter's minimum, maximum and nominal value.
 *
 * A plain aggregate of OBSERVERS. The widgets belong to their Qt parent -
 * the form that created them - and this row only remembers where they
 * are, so that the form can read a parameter back without walking the
 * layout. A setter here must NOT delete the widget it replaces: it is not
 * this class's to free, and freeing it leaves the layout with a dangling
 * child.
 *
 * @author Isaac Martínez Forte
 */

namespace qftbx {

class ParLineEdit
{
public:

    /// Default constructible so a row can be held by value, and empty
    /// until its widgets exist.
    ParLineEdit() = default;

    /**
     * @brief Row over three existing line edits.
     *
     * @param x the minimum of the parameter.
     * @param y its maximum.
     * @param nominal its nominal value.
     */
    ParLineEdit(QLineEdit * x, QLineEdit * y, QLineEdit * nominal);

    /// @param label the line edit holding the minimum.
    void setX (QLineEdit * label);

    /// The line edit holding the minimum, or nullptr on an empty row.
    QLineEdit *getX() const;

    /// @param label the line edit holding the maximum.
    void setY (QLineEdit * label);

    /// The line edit holding the maximum, or nullptr on an empty row.
    QLineEdit *getY() const;

    /// @param nominal the line edit holding the nominal value.
    void setNominal (QLineEdit *  nominal);

    /// The line edit holding the nominal value, or nullptr on an empty row.
    QLineEdit * nominal() const;

private:
    QLineEdit *x = nullptr;
    QLineEdit *y = nullptr;
    QLineEdit * m_nominal = nullptr;
};

}

#endif
