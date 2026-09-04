#include "src/core/system/lti_system.h"

namespace qftbx {

LtiSystem::LtiSystem(std::string name)
{
    m_name = name;
}

void LtiSystem:: setName (std::string name){
    m_name = name;
}

const std::string & LtiSystem::name() const {
    return m_name;
}


bool LtiSystem::sameAs(LtiSystem & other)
{
    if (type() != other.type() || name() != other.name()) {
        return false;
    }

    //The textual forms carry whatever the concrete family adds on top of the
    //parameters - a FreeForm's two expressions, above all.
    if (numeratorString() != other.numeratorString() ||
            denominatorString() != other.denominatorString()) {
        return false;
    }

    if (numerator().size() != other.numerator().size() ||
            denominator().size() != other.denominator().size()) {
        return false;
    }

    for (std::size_t i = 0; i < numerator().size(); ++i) {
        if (numerator()[i] != other.numerator()[i]) {
            return false;
        }
    }

    for (std::size_t i = 0; i < denominator().size(); ++i) {
        if (denominator()[i] != other.denominator()[i]) {
            return false;
        }
    }

    return gain() == other.gain() && delay() == other.delay();
}

} // namespace qftbx
