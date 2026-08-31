#include "parlineedit.h"

//Agrupa tres QLineEdit SIN tomar propiedad: los line edits son hijos Qt del
//widget de su fila y mueren con el. El constructor delegado anterior creaba
//tres QLineEdit que se fugaban de inmediato, y el destructor los borraba a
//pesar de no ser suyos (doble borrado junto al padre Qt).
ParLineEdit::ParLineEdit(QLineEdit * x, QLineEdit*  y, QLineEdit * nominal){
    this->x = x;
    this->y = y;
    m_nominal = nominal;
}

void ParLineEdit::setX(QLineEdit *label){
    x = label;
}

QLineEdit * ParLineEdit::getX(){
    return x;
}

void ParLineEdit::setY(QLineEdit *label){
    y = label;
}

QLineEdit *ParLineEdit::getY(){
    return y;
}

void ParLineEdit::setNominal(QLineEdit * nominal){
    m_nominal = nominal;
}

QLineEdit *ParLineEdit::nominal(){
    return m_nominal;
}

