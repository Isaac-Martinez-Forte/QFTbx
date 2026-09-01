#include "parameter.h"

using namespace mup;
using namespace std;

namespace qftbx {

Parameter::Parameter(QString name, Range range, qreal nominal, QString exp)
{
    m_name = name;

    m_range = range.ordered();

    m_nominal = nominal;
    m_uncertain = true;

    if (exp == nullptr || exp.isEmpty()) {
        //Without a reparametrisation the expression is the parameter
        //itself, as in the three-argument constructor.
        m_expression = name;
        m_hasExpression = false;
    } else {
        m_expression = exp;
        m_hasExpression = true;
    }
}

Parameter::Parameter(QString name, Range range, qreal nominal){
    m_name = name;

    m_range = range.ordered();

    m_nominal = nominal;
    m_expression = name;

    m_uncertain = true;


    m_hasExpression = false;
}

Parameter::Parameter (Range range){
    m_range = range.ordered();

    m_nominal = 0;
    m_uncertain = false;
    m_hasExpression = false;
}

Parameter::Parameter(const Parameter &obj){
    m_name = obj.m_name;
    m_range = obj.m_range;
    m_nominal = obj.m_nominal;
    m_uncertain = obj.m_uncertain;
    m_expression = obj.m_expression;
    this->m_hasExpression = obj.m_hasExpression;
}

Parameter::Parameter() {
   m_nominal = 0;
   m_uncertain = false;
   m_hasExpression = false;
}

Parameter::Parameter (qreal value){
    m_nominal = value;
    m_name = QString::number(m_nominal);
    m_uncertain = false;
    m_range = Range(m_nominal, m_nominal);
    m_expression = m_name;
}

Parameter::Parameter (QString name, qreal value){
    m_nominal = value;
    m_name = name;
    m_uncertain = false;
    m_range = Range(m_nominal, m_nominal);
    m_expression = name;
}


bool Parameter::isUncertain(){
    return m_uncertain;
}

void Parameter::setUncertain(bool a) {
    m_uncertain = a;
}

QString Parameter::name(){
    return m_name;
}

Range Parameter::range(){

    if (!m_uncertain){
        return m_range;
    }

    if (!m_hasExpression){
        return m_range;
    }

    Range point;

    //The Values must be declared before the parser: it stores pointers to
    //them (Variable(&v)) and destruction runs in reverse declaration order.
    Value v(m_range.min);
    Value v2(m_range.max);

    mup::ParserX p;

    p.SetExpr(m_expression.toStdString());
    p.DefineVar(m_name.toStdString(), Variable(&v));

    point.min = p.Eval().GetFloat();

    p.RemoveVar(m_name.toStdString());
    p.DefineVar(m_name.toStdString(), Variable(&v2));

    point.max = p.Eval().GetFloat();

    return point;
}

qreal Parameter::nominal(){

    if (!m_uncertain){
        return m_nominal;
    }

    if (!m_hasExpression){
        return m_nominal;
    }

    //Value before the parser: see the comment in range().
    Value v(m_nominal);

    mup::ParserX p;

    p.SetExpr(m_expression.toStdString());
    p.DefineVar(m_name.toStdString(), Variable(&v));

    return p.Eval().GetFloat();
}

void Parameter::setName(QString name){
    m_name = name;
}

void Parameter::setRange(Range range){
    m_range = range;
}

void Parameter::setNominal(qreal nominal){
    m_nominal = nominal;
}

QString Parameter::expression(){
    return m_expression;
}

Range Parameter::rawRange(){
    return m_range;
}

qreal Parameter::rawNominal(){
    return m_nominal;
}



} // namespace qftbx
