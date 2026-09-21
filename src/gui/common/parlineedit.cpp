/**
 * @file
 * @brief Accessors of the uncertainty row.
 *
 * Pointers are stored and returned. There is no destructor because the line
 * edits die with their Qt parent and freeing them here would be a double
 * delete.
 */

#include "src/gui/common/parlineedit.h"

namespace qftbx {

ParLineEdit::ParLineEdit(QLineEdit * x, QLineEdit*  y, QLineEdit * nominal){
    this->x = x;
    this->y = y;
    m_nominal = nominal;
}

void ParLineEdit::setX(QLineEdit *label){
    x = label;
}

QLineEdit * ParLineEdit::getX() const {
    return x;
}

void ParLineEdit::setY(QLineEdit *label){
    y = label;
}

QLineEdit *ParLineEdit::getY() const {
    return y;
}

void ParLineEdit::setNominal(QLineEdit * nominal){
    m_nominal = nominal;
}

QLineEdit *ParLineEdit::nominal() const {
    return m_nominal;
}

}
