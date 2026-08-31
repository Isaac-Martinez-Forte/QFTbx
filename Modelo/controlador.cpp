#include "controlador.h"

using namespace std;


Controlador::Controlador()
{
    dao = new FDAO ();

    paso1 = false;
    paso2 = false;
    paso3 = false;
    paso4 = false;
    paso5 = false;
    paso6 = false;
    paso7 = false;
}

Controlador::~Controlador(){

    delete dao;

    if(paso1){
        delete plantadao;
    }

    if (paso2){
        delete especdao;
    }

    if(paso3){
        delete omegadao;
    }

    if(paso4){
        delete templatedao;
        delete templates;
    }

    if(paso5){
        delete bounddao;
        delete bound;
    }

    if (paso6){
        delete controladordao;
    }

    if(paso7){
        delete loopShaping;
        delete loopshapingdao;
    }
}

LtiSystem *Controlador::getPlanta(){
    if(!paso1)
        return NULL;

    return plantadao->getPlanta();
}

Omega * Controlador::getOmega(){
    if(!paso3)
        return NULL;

    return omegadao->getOmega();
}

QVector <qreal> * Controlador::getFrecuencias(){
    if(!paso3)
        return NULL;

    return omegadao->getFrecuencias();
}


QVector<tools::dBND *> *Controlador::getEspecificaciones(){
    if(!paso2)
        return NULL;

    return especdao->getEspecificaciones();
}

void Controlador::setPlanta(LtiSystem *planta){
    if (!paso1)
        plantadao = dao->getPlantaDAO();
    paso1 = true;
    plantadao->setPlanta(planta);
}

void Controlador::setOmega(Omega *omega){
    if(!paso3)
        omegadao = dao->getOmegaDAO();
    paso3 = true;

    omegadao->setOmega(omega);
}

void Controlador::setEspecificaciones(QVector<tools::dBND *> *espe){
    if(!paso2){
        especdao = dao->getEspecificacionesDAO();
    }

    paso2 = true;

    especdao->setEspecificaciones(espe);
}

void Controlador::setTemplate(QVector<QVector<std::complex<qreal> > *> *temp,
                              QVector<QVector<std::complex<qreal> > *> *contorno, bool isContorno){

    if(!paso4){
        templatedao = dao->getTemplateDAO();
        templates = new TemplateEngine();
    }
    paso4 = true;

    templatedao->setTemplates(temp);

    //Alimentar tambien el motor: sin esto, recalcular el contorno tras
    //CARGAR un proyecto desreferenciaba un puntero nulo.
    templates->setClouds(temp);

    if (isContorno)
        templatedao->setContorno(contorno);


}

void Controlador::setContorno(QVector<QVector<std::complex<qreal> > *> *contorno){
    templatedao->setContorno(contorno);
}

void Controlador::setBoundaries(BoundaryData *bound){

    if (!paso5){
        this->bound = new BoundaryEngine();
        bounddao = dao->getBoundDAO();
    }
    paso5 = true;

    bounddao->setBound(bound);

}

QVector <QVector <std::complex <qreal> > * > * Controlador::getTemplate(){
    if (!paso4)
        return NULL;
    return templatedao->getTemplates();
}

QVector <QVector <std::complex <qreal> > * > * Controlador::getContorno(){
    if (!paso4)
        return NULL;
    return templatedao->getContorno();
}

bool Controlador::calcularTemplates(QVector <qreal> * epsilon, QHash <QString, QVector<qreal> *> *mapa, bool cuda){

    if(!paso4){
        templatedao = dao->getTemplateDAO();
        templates = new TemplateEngine();
    }
    paso4 = true;

    templates->setEpsilon(epsilon);
    templates->setGrids(mapa);

    templates->compute(getPlanta(),getOmega()->getValores(), cuda);

    //El calculo ya no reordena ni sustituye las frecuencias: basta guardar
    //el epsilon usado para la persistencia.
    templatedao->setEpsilon(epsilon);

    QVector <QVector <std::complex <qreal> > * > * temp = templates->clouds();
    QVector <QVector <std::complex <qreal> > * > * cont = templates->contours();

    if (cont != NULL)
        setTemplate(temp,cont,true);
    else
        setTemplate(temp,cont,false);

    if (temp == NULL || cont == NULL)
         return false;

    return true;
}

QVector <qreal> * Controlador::getEpsilon(){
    return templatedao->getEpsilon();
}


QVector <QVector <std::complex <qreal> > * > * Controlador::recalcularContorno(QVector <qreal> * epsilon){
    templates->computeContours(epsilon);
    QVector <QVector <std::complex <qreal> > * > * aux = templates->contours();

    setContorno(aux);
    templatedao->setEpsilon(epsilon);

    return aux;
}

bool Controlador::calcularBoundaries(QPointF datosFas, qint32 puntosFas, QPointF datosMag,
                                     qint32 puntosMag, qreal infinito, bool contorno, bool cuda){

    if (!paso5){
        bound = new BoundaryEngine();
        bounddao = dao->getBoundDAO();
    }
    paso5 = true;

    if (contorno){
        bound->compute(omegadao->getFrecuencias(),plantadao->getPlanta(), templatedao->getContorno()
                         ,especdao->getEspecificaciones(), datosFas,puntosFas,datosMag,puntosMag, infinito, cuda);
    } else {
        bound->compute(omegadao->getFrecuencias(),plantadao->getPlanta(), templatedao->getTemplates()
                         ,especdao->getEspecificaciones(), datosFas,puntosFas,datosMag,puntosMag, infinito, cuda);
    }

    setBoundaries(bound->boundaryData());

    getOmega()->setOmega(bound->omega());

    return true;
}

BoundaryData *Controlador::getBound(){
    return bounddao->getBound();
}

QVector< QVector<QPointF> * > * Controlador::unionBoundaries(){
    return bounddao->getBound()->unionBoundaries();
}

QVector< QVector <QVector<QPointF> * > * > * Controlador::unionBuckets(){
    return bounddao->getBound()->unionBuckets();
}

bool Controlador::setControlador(LtiSystem *controlador){

    if (!paso6){
        controladordao = dao->getControladorDAO();
    }

    paso6 = true;

    controladordao->setControlador(controlador);

    return true;
}

LtiSystem * Controlador::getControlador(){
    if (!paso6){
        return NULL;
    }

    return controladordao->getControlador();
}

bool Controlador::calcularLoopShaping(qreal epsilon, tools::LoopShapingAlgorithm seleccionado, QPointF rango, qreal nPuntos,
                                      qint32 inicializacion){

    if (!paso7){
        loopShaping = new LoopShaping();
        loopshapingdao = dao->getLoopShapingDAO();
    }

    paso7 = true;

    bool re = loopShaping->iniciar(plantadao->getPlanta(), controladordao->getControlador(), omegadao->getFrecuencias(), bounddao->getBound(),
                           epsilon, seleccionado, templatedao->getContorno(), especdao->getEspecificaciones(),
                                   inicializacion);

    if (re){
        loopshapingdao->setDatos(new DatosLoopShaping(loopShaping->getControlador(), rango, nPuntos));
        return true;
    }

    return false;
}

void Controlador::setLoopShaping(DatosLoopShaping *datos){
    if (!paso7){
        loopShaping = new LoopShaping();
        loopshapingdao = dao->getLoopShapingDAO();
    }

    paso7 = true;

    loopshapingdao->setDatos(datos);
}

DatosLoopShaping * Controlador::getLoopShaping(){
    return loopshapingdao->getLoopShaping();
}

bool Controlador::guardarSistema(QString fichero){

    ProjectContent content;

    if (paso1)
        content.plant = plantadao->getPlanta();

    if (paso2)
        content.specifications = especdao->getEspecificaciones();

    if (paso3)
        content.omega = omegadao->getOmega();

    if (paso4){
        content.templates = templatedao->getTemplates();
        content.epsilon = templatedao->getEpsilon();
        if (templatedao->isContorno()){
            content.contour = templatedao->getContorno();
        }
    }

    if (paso5)
        content.boundaries = bounddao->getBound();

    if (paso6)
        content.controller = controladordao->getControlador();

    if (paso7)
        content.loopShaping = loopshapingdao->getLoopShaping();

    ProjectWriter writer;
    writer.save(fichero, content);

    return true;
}

QVector <bool> * Controlador::cargarSistema(QString fichero){

    ProjectReader leer;

    QVector <bool> * retorno = leer.load(fichero);

    if (retorno->value(0))
        setPlanta(leer.plant());

    if (retorno->value(1))
        setEspecificaciones(leer.specifications());

    if (retorno->value(2))
        setOmega(leer.omega());

    if (retorno->value(3)){
        if (retorno->value(7)){
            setTemplate(leer.templates(), leer.contour(), true);
        }else {
            setTemplate(leer.templates(), NULL, false);
        }
        templatedao->setEpsilon(leer.epsilon());
    }

    if (retorno->value(4))
        setBoundaries(leer.boundaries());

    if (retorno->value(5)){
        setControlador(leer.controller());
    }

    if (retorno->value(6)){
        setLoopShaping(leer.loopShaping());
    }

    return retorno;

}
