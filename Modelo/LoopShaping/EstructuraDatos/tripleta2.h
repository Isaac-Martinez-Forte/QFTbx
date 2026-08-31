#ifndef TRIPLETA2
#define TRIPLETA2

#include <QHash>

#include "Modelo/Herramientas/tools.h"
#include "src/core/system/lti_system.h"
#include "tripleta.h"
#include "etapas.h"

//Live-list node of algorithm MC (thesis): a Tripleta plus the node
//history of thesis sec. 4.4.4 (execution stage, cut switch and the
//design frequencies the node is certified feasible at). The node owns
//its frequency map; every child receives a copy.
class Tripleta2 : public Tripleta {

public:

    Tripleta2() {}

    Tripleta2(qreal index, LtiSystem * sistema, tools::flags_box flags = tools::ambiguous);

    ~Tripleta2();

    void setRecorteActivado(bool recorteActivado);
    bool isRecorteActivado();

    void setEtapas(Etapas e);
    Etapas getEtapas();

    void addFrecuenciaFeasible(qreal pos, qreal frec);
    bool isFrecueciaFeasible(qreal key);
    void setFrecuenciasFeasible(QHash<qreal, qreal> * frecuenciasFeasible);
    QHash<qreal, qreal> * getFrecuenciasFeasible();

protected:

    bool recorteActivado = true;
    Etapas etapa = Etapas::INICIAL;

    QHash<qreal, qreal> * frecuenciasFeasible = nullptr;
};

#endif // TRIPLETA2
