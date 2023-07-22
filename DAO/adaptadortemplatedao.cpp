#include "adaptadortemplatedao.h"

using namespace std;

AdaptadorTemplateDAO::AdaptadorTemplateDAO()
{
    contornoExists = false;
}

AdaptadorTemplateDAO::~AdaptadorTemplateDAO(){
    if (templates != NULL)
        templates->clear();

    if (contorno != NULL)
        contorno->clear();
}

void AdaptadorTemplateDAO::setTemplates(QVector<QVector<complex<qreal> > *> * templates){
    if (this->templates != NULL)
        this->templates->clear();

    this->templates = templates;
}

QVector <QVector <complex <qreal> > * > * AdaptadorTemplateDAO::getTemplates(){
    return templates;
}

void AdaptadorTemplateDAO::setContorno(QVector<QVector<std::complex<qreal> > *> *contorno){
    if (this->contorno != NULL)
        this->contorno->clear();

    this->contorno = contorno;
    contornoExists = true;
}

QVector <QVector <std::complex <qreal> > * > * AdaptadorTemplateDAO::getContorno(){
    return contorno;
}


bool AdaptadorTemplateDAO::isContorno(){
    return contornoExists;
}


void AdaptadorTemplateDAO::setEpsilon (QVector <qreal> * epsilon){

    if (this->epsilon != NULL){
        this->epsilon->clear();
    }

    this->epsilon = epsilon;
}

QVector <qreal>* AdaptadorTemplateDAO::getEpsilon (){
    return epsilon;
}
