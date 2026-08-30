#ifndef TRIPLETA
#define TRIPLETA

#include "cinterval.hpp"
#include "Modelo/Herramientas/tools.h"
#include "src/core/system/lti_system.h"
#include  "n.h"
#include "Modelo/LoopShaping/EstructuraDatos/data_box.h"

using namespace tools;

class Tripleta : public N {

public:

    Tripleta() {}

    Tripleta(qreal index, LtiSystem * sistema, flags_box flags = ambiguous);
    
    Tripleta(qreal index, LtiSystem * sistema,  QVector<data_box *> *datos);

    ~Tripleta();

    Tripleta &operator=(const Tripleta &c) ;

    bool operator==(const Tripleta &c) const;
    bool operator!=(const Tripleta &c) const;
    bool operator<(const Tripleta &c) const;
    bool operator>(const Tripleta &c) const;
    bool operator<=(const Tripleta &c) const;
    bool operator>=(const Tripleta &c) const;

    flags_box getFlags() const;

    void setFlags(const flags_box &value);

    QVector<data_box *> *getDatos() const;

    void setDatos(QVector<data_box *> *value);

    LtiSystem *getSistema() const;

    void setSistema(LtiSystem *value);

    void setFeasible(QVector<bool> *value);

    QVector<qreal> *getPuntosCorte() const;

    void setPuntosCorte(QVector<qreal> *value);

    void releaseOwnership();
    void noBorrar2();
protected:

    LtiSystem * sistema;
    flags_box flags;

    QVector <qreal> * puntosCorte = nullptr;
    QVector <bool> * feasible = nullptr;

    QVector <data_box *> * datos = nullptr;

    bool b = true;
    bool b2 = true;

};

#endif // TRIPLETA
