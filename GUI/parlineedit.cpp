#include "parlineedit.h"

//Groups three QLineEdits WITHOUT taking ownership: the line edits are Qt
//children of their row widget and die with it. The old delegating
//constructor created three QLineEdits that leaked immediately, and the
//destructor deleted widgets it did not own (double delete with the Qt
//parent).
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

