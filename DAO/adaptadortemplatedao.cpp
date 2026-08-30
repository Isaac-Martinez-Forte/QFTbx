#include "adaptadortemplatedao.h"

using namespace std;

AdaptadorTemplateDAO::AdaptadorTemplateDAO()
{
    contornoExists = false;
}

//El DAO es el DUEÑO de templates, contorno y epsilon: sus setters borran en
//profundidad lo anterior (el motor TemplateEngine y las vistas solo observan).
AdaptadorTemplateDAO::~AdaptadorTemplateDAO(){
    if (templates != NULL){
        qDeleteAll(*templates);
        delete templates;
    }

    if (contorno != NULL){
        qDeleteAll(*contorno);
        delete contorno;
    }

    delete epsilon;
}

void AdaptadorTemplateDAO::setTemplates(QVector<QVector<complex<qreal> > *> * templates){
    if (this->templates == templates)
        return;

    if (this->templates != NULL){
        qDeleteAll(*this->templates);
        delete this->templates;
    }

    this->templates = templates;
}

QVector <QVector <complex <qreal> > * > * AdaptadorTemplateDAO::getTemplates(){
    return templates;
}

void AdaptadorTemplateDAO::setContorno(QVector<QVector<std::complex<qreal> > *> *contorno){
    if (this->contorno == contorno){
        contornoExists = true;
        return;
    }

    if (this->contorno != NULL){
        qDeleteAll(*this->contorno);
        delete this->contorno;
    }

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

    if (this->epsilon == epsilon)
        return;

    delete this->epsilon;

    this->epsilon = epsilon;
}

QVector <qreal>* AdaptadorTemplateDAO::getEpsilon (){
    return epsilon;
}
