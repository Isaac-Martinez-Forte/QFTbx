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

std::shared_ptr<Sistema> Controlador::getPlanta(){
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

void Controlador::setPlanta(std::shared_ptr<Sistema> planta){
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
        templates = new Templates();
    }
    paso4 = true;

    templatedao->setTemplates(temp);

    if (isContorno)
        templatedao->setContorno(contorno);


}

void Controlador::setContorno(QVector<QVector<std::complex<qreal> > *> *contorno){
    templatedao->setContorno(contorno);
}

void Controlador::setBoundaries(std::shared_ptr<DatosBound> dbound){

    if (!paso5){
        this->bound = new Boundaries();
        bounddao = dao->getBoundDAO();
    }
    paso5 = true;

    bounddao->setBound(dbound);

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

bool Controlador::calcularTemplates(QVector <qreal> * epsilon, QHash <Var *, QVector<qreal> *> *mapa, bool cuda){

    if(!paso4){
        templatedao = dao->getTemplateDAO();
        templates = new Templates();
    }
    paso4 = true;

    templates->setEpsilon(epsilon);
    templates->setMapa(mapa);

    templates->lanzarCalculo(getPlanta(),getOmega()->getValores(), cuda);

    getOmega()->setOmega(templates->getOmega());
    templatedao->setEpsilon(templates->getEpsilon());

    QVector <QVector <std::complex <qreal> > * > * temp = templates->getTemplates();
    QVector <QVector <std::complex <qreal> > * > * cont = templates->getContorno();

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
    templates->lanzarCalculoContorno(epsilon);
    QVector <QVector <std::complex <qreal> > * > * aux = templates->getContorno();

    setContorno(aux);
    getOmega()->setOmega(templates->getOmega());
    templatedao->setEpsilon(templates->getEpsilon());

    return aux;
}

bool Controlador::calcularBoundaries(QPointF datosFas, qint32 puntosFas, QPointF datosMag,
                                     qint32 puntosMag, qreal infinito, bool contorno, bool cuda){

    if (!paso5){
        bound = new Boundaries();
        bounddao = dao->getBoundDAO();
    }
    paso5 = true;

    if (contorno){
        bound->lanzarCalculo(omegadao->getFrecuencias(),plantadao->getPlanta(), templatedao->getContorno()
                         ,especdao->getEspecificaciones(), datosFas,puntosFas,datosMag,puntosMag, infinito, cuda);
    } else {
        bound->lanzarCalculo(omegadao->getFrecuencias(),plantadao->getPlanta(), templatedao->getTemplates()
                         ,especdao->getEspecificaciones(), datosFas,puntosFas,datosMag,puntosMag, infinito, cuda);
    }

    setBoundaries(bound->getBoundaries());

    getOmega()->setOmega(bound->getOmega());

    return true;
}

std::shared_ptr<DatosBound> Controlador::getBound(){
    return bounddao->getBound();
}

QVector< QVector<QPointF> * > * Controlador::getBoundariesReun(){
    return bounddao->getBound()->getBoundariesReun();
}

QVector< QVector <QVector<QPointF> * > * > * Controlador::getBoundariesReunHash(){
    return bounddao->getBound()->getBoundariesReunHash();
}

bool Controlador::setControlador(std::shared_ptr<Sistema> controlador){

    if (!paso6){
        controladordao = dao->getControladorDAO();
    }

    paso6 = true;

    controladordao->setControlador(controlador);

    return true;
}

std::shared_ptr<Sistema> Controlador::getControlador(){
    if (!paso6){
        return NULL;
    }

    return controladordao->getControlador();
}

bool Controlador::calcularLoopShaping(qreal epsilon, tools::alg_loop_shaping seleccionado, QPointF rango, qreal nPuntos,
                                      bool depuracion, qreal delta, qint32 inicializacion, bool hilos, bool bisection_avanced, bool deteccion_avanced,
                                      bool a){

    if (!paso7){
        loopShaping = new LoopShaping();
        loopshapingdao = dao->getLoopShapingDAO();
    }

    paso7 = true;

    bool re = loopShaping->iniciar(plantadao->getPlanta(), controladordao->getControlador(), omegadao->getFrecuencias(), bounddao->getBound(),
                           epsilon, seleccionado, depuracion, delta, templatedao->getContorno(), especdao->getEspecificaciones(),
                                   inicializacion, hilos, bisection_avanced, deteccion_avanced, a);

    if (re){
        loopshapingdao->setDatos(std::make_shared<DatosLoopShaping>(loopShaping->getControlador(), rango, nPuntos));
        return true;
    }

    return false;
}

void Controlador::setLoopShaping(std::shared_ptr<DatosLoopShaping>datos){
    if (!paso7){
        loopShaping = new LoopShaping();
        loopshapingdao = dao->getLoopShapingDAO();
    }

    paso7 = true;

    loopshapingdao->setDatos(datos);
}

std::shared_ptr<DatosLoopShaping> Controlador::getLoopShaping(){
    return loopshapingdao->getLoopShaping();
}

bool Controlador::guardarSistema(QString fichero){

    bool retorno = true;

    DatosPlanta * datosPlanta = new DatosPlanta();

    if (paso1)
        datosPlanta->setPlanta(plantadao);

    if (paso2)
        datosPlanta->setEspecificaciones(especdao);

    if (paso3)
        datosPlanta->setOmega(omegadao);

    if (paso4)
        datosPlanta->setTemplates(templatedao);

    if (paso5)
        datosPlanta->setBoundaries(bounddao);

    if (paso6)
        datosPlanta->setControlador(controladordao);

    if (paso7)
        datosPlanta->setLoopShaping(loopshapingdao);


    XmlParserSave * parser = new XmlParserSave();
    parser->guardarXMLDatos(fichero, datosPlanta);

    delete parser;
    delete datosPlanta;

    return retorno;
}

QVector <bool> * Controlador::cargarSistema(QString fichero){

    XmlParserLoad * leer = new XmlParserLoad();

    QVector <bool> * retorno =  leer->recuperarXmlDatos(fichero);

    if (retorno->value(0))
        setPlanta(leer->getPlanta());

    if (retorno->value(1))
        setEspecificaciones(leer->getEspecificaciones());

    if (retorno->value(2))
        setOmega(leer->getOmega());

    if (retorno->value(3)){
        if (retorno->value(7)){
            setTemplate(leer->getTemplates(), leer->getContorno(), true);
        }else {
            setTemplate(leer->getTemplates(), NULL, false);
        }
        templatedao->setEpsilon(leer->getEpsilon());
    }

    if (retorno->value(4))
        setBoundaries(leer->getBoundaries());

    if (retorno->value(5)){
        setControlador(leer->getControlador());
    }

    if (retorno->value(6)){
        setLoopShaping(leer->getLoopShaping());
    }

    return retorno;

}
