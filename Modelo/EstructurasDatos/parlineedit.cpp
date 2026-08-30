#include "parlineedit.h"

ParLineEdit::ParLineEdit()
{
    x = new QLineEdit();
    y = new QLineEdit();
    m_nominal = new QLineEdit();
}

ParLineEdit::ParLineEdit(QLineEdit * x, QLineEdit*  y, QLineEdit * nominal) : ParLineEdit(){
    this->x = x;
    this->y = y;
    //Nota: antes el parametro sombreaba al campo y esto era un self-assign
    //(el campo nunca se actualizaba).
    m_nominal = nominal;
}

ParLineEdit::~ParLineEdit(){
    delete x;
    delete y;
    delete m_nominal;
}

void ParLineEdit::setX(QLineEdit *label){
    delete x;

    x = label;
}

QLineEdit * ParLineEdit::getX(){
    return x;
}

void ParLineEdit::setY(QLineEdit *label){
    delete y;

    y = label;
}

QLineEdit *ParLineEdit::getY(){
    return y;
}

void ParLineEdit::setNominal(QLineEdit * nominal){
    //Nota: antes esto borraba el ARGUMENTO y se auto-asignaba (no-op).
    delete m_nominal;

    m_nominal = nominal;
}

QLineEdit *ParLineEdit::nominal(){
    return m_nominal;
}

