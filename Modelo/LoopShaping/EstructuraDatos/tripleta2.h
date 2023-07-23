#ifndef TRIPLETA2
#define TRIPLETA2

#include "cinterval.hpp"
#include "Modelo/Herramientas/tools.h"
#include "Modelo/EstructuraSistema/sistema.h"
#include "tripleta.h"
#include "etapas.h"
#include "listaordenada.h"
#include "Modelo/LoopShaping/EstructuraDatos/data_box.h"

using namespace cxsc;
using namespace tools;

class Tripleta2 : public Tripleta {


public:

    Tripleta2() {}

    Tripleta2(qreal index, std::shared_ptr<Sistema> sistema, flags_box flags = ambiguous);
    
    Tripleta2(qreal index, std::shared_ptr<Sistema> sistema,  QVector<data_box *> *datos);
    ~Tripleta2();

    void setRecorteActivado (bool recorteActivado);

    bool isRecorteActivado ();

    void setEtapas (Etapas e);

    Etapas getEtapas ();

    QVector <cinterval> * getTerNume();

    void setTerNume (QVector <cinterval> * terNume);

    QVector <cinterval> * getTerDeno();
    void setTerDeno (QVector <cinterval> * terDeno);

    cinterval getTerK();

    void setTerK (cinterval terK);

    void setLados (qreal m, qreal f);

    qreal getAnchoFas ();
    qreal getAnchoMag ();

    QVector <ListaOrdenada *> * getNumeradorInfMag ();
    void setNumeradorInfMag (QVector <ListaOrdenada *> * m);

    QVector <ListaOrdenada *> * getNumeradorInfFas ();
    void setNumeradorInfFas (QVector <ListaOrdenada *> * m);

    QVector <ListaOrdenada *> * getNumeradorSupMag ();
    void setNumeradorSupMag (QVector <ListaOrdenada *> * m);

    QVector <ListaOrdenada *> * getNumeradorSupFas ();
    void setNumeradorSupFas (QVector <ListaOrdenada *> * m);

    QVector <ListaOrdenada *> * getDenominadorInfMag ();
    void setDenominadorInfMag (QVector <ListaOrdenada *> * m);

    QVector <ListaOrdenada *> * getDenominadorInfFas ();
    void setDenominadorInfFas (QVector <ListaOrdenada *> * m);

    QVector <ListaOrdenada *> * getDenominadorSupMag ();
    void setDenominadorSupMag (QVector <ListaOrdenada *> * m);

    QVector <ListaOrdenada *> * getDenominadorSupFas ();
    void setDenominadorSupFas (QVector <ListaOrdenada *> * m);

    ListaOrdenada * getKInf ();
    void setKInf (ListaOrdenada * lista) ;

    ListaOrdenada * getKSup ();
    void setKSup (ListaOrdenada * lista) ;

    QVector<data_box *> * getDatosCortesBoundaries () ;
    void setDatosCortesBoundaries (QVector<data_box *> * datos);

    void addFrecuenciaFeasible (qreal pos, qreal frec);
    bool isFrecueciaFeasible (qreal key);
    qreal getFrecuenciaFeasible (qreal key);
    qreal getPosFrecuenciaFeasible (qreal value);
    void setFrecuenciasFeasible (QHash <qreal, qreal> * frecuenciasFeasible);
    QHash <qreal, qreal> * getFrecuenciasFeasible ();


protected:
    bool recorteActivado;
    Etapas etapa;
    qreal anchoFas, anchoMag;

    QVector <cinterval> * terNume = nullptr;

    QVector <cinterval> * terDeno = nullptr;

    cinterval terK;

    QVector <ListaOrdenada *> * numeradorInfMag = nullptr;
    QVector <ListaOrdenada *> * numeradorSupMag = nullptr;
    QVector <ListaOrdenada *> * numeradorInfFas = nullptr;
    QVector <ListaOrdenada *> * numeradorSupFas = nullptr;

    QVector <ListaOrdenada *> * denominadorInfMag = nullptr;
    QVector <ListaOrdenada *> * denominadorSupMag = nullptr;
    QVector <ListaOrdenada *> * denominadorInfFas = nullptr;
    QVector <ListaOrdenada *> * denominadorSupFas = nullptr;

    ListaOrdenada * kSup = nullptr;
    ListaOrdenada * kInf = nullptr;

    QVector<data_box *> * datosCortesBoundaries = nullptr;

    QHash <qreal, qreal> * frecuenciasFeasible = nullptr;
};

#endif // TRIPLETA
