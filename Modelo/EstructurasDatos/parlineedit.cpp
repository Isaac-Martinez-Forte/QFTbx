#include "parlineedit.h"

ParLineEdit::ParLineEdit()
{
    x = new QLineEdit();
    y = new QLineEdit();
    nominal = new QLineEdit();
}

ParLineEdit::ParLineEdit(QLineEdit * x, QLineEdit*  y, QLineEdit *nominal) : ParLineEdit(){
    this->x = x;
    this->y = y;
    this->nominal = nominal;
}

ParLineEdit::~ParLineEdit(){
    delete x;
    delete y;
    delete nominal;
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

void ParLineEdit::setNominal(QLineEdit *nominal){
    delete this->nominal;

    this->nominal = nominal;
}

QLineEdit *ParLineEdit::getNominal(){
    return nominal;
}

