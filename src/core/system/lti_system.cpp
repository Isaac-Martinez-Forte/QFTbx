#include "lti_system.h"

using namespace std;

namespace qftbx {

LtiSystem::LtiSystem(QString name)
{
    m_name = name;
}

void LtiSystem:: setName (QString name){
    m_name = name;
}

QString LtiSystem:: name(){
    return m_name;
}



} // namespace qftbx
