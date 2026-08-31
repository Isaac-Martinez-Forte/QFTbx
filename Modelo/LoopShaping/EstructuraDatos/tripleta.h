#ifndef TRIPLETA
#define TRIPLETA

#include "cinterval.hpp"
#include "Modelo/Herramientas/tools.h"
#include "src/core/system/lti_system.h"
#include  "n.h"

using namespace tools;

class Tripleta : public N {

public:

    Tripleta() {}

    Tripleta(qreal index, LtiSystem * sistema, flags_box flags = ambiguous);

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

    LtiSystem *getSistema() const;

    void setSistema(LtiSystem *value);

    void releaseOwnership();
    void noBorrar2();
protected:

    LtiSystem * sistema;
    flags_box flags;

    bool b = true;
    bool b2 = true;

};

#endif // TRIPLETA
