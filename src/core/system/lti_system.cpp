#include "lti_system.h"

using namespace std;

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



} // namespace qftbx
