#ifndef TRIPLETA
#define TRIPLETA

#include "cinterval.hpp"
#include "Modelo/Herramientas/tools.h"
#include "Modelo/EstructuraSistema/sistema.h"
#include  "n.h"

using namespace tools;

class Tripleta : public N {

public:

    Tripleta() {}

    Tripleta(qreal index, Sistema * sistema, flags_box flags = ambiguous);
    
    Tripleta(qreal index, Sistema * sistema,  QVector <struct datos_caja> * datos);

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

    QVector<datos_caja> *getDatos() const;

    void setDatos(QVector<datos_caja> *value);

    Sistema *getSistema() const;

    void setSistema(Sistema *value);

    void setFeasible(QVector<bool> *value);

    QVector<qreal> *getPuntosCorte() const;

    void setPuntosCorte(QVector<qreal> *value);

    void borrar();
protected:

    Sistema * sistema;
    flags_box flags;

    QVector <qreal> * puntosCorte = nullptr;
    QVector <bool> * feasible = nullptr;

    QVector <struct datos_caja> * datos = nullptr;

    bool bo = false;

};

#endif // TRIPLETA
