/**
 * @file
 * @brief One row of the uncertainty table: the line edits of a parameter's
 * minimum, maximum and nominal.
 *
 * A plain aggregate of observers: x is the minimum, y the maximum. The
 * widgets belong to the form that created them; the row only remembers
 * where they are so the form can read a parameter back without walking the
 * layout. Nothing here frees a widget: a setter that deleted the one it
 * replaces would leave the layout with a dangling child. A default row is
 * empty, and its getters return nullptr until its widgets exist.
 */

#ifndef PARLABEL_H
#define PARLABEL_H

#include <QVector>
#include <QLineEdit>

namespace qftbx {

class ParLineEdit
{
public:

    ParLineEdit() = default;

    ParLineEdit(QLineEdit * x, QLineEdit * y, QLineEdit * nominal);

    void setX (QLineEdit * label);

    QLineEdit *getX() const;

    void setY (QLineEdit * label);

    QLineEdit *getY() const;

    void setNominal (QLineEdit *  nominal);

    QLineEdit * nominal() const;

private:
    QLineEdit *x = nullptr;
    QLineEdit *y = nullptr;
    QLineEdit * m_nominal = nullptr;
};

}

#endif
