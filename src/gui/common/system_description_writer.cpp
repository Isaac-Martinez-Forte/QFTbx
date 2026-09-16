#include "src/gui/common/system_description_writer.h"

#include "src/gui/common/number_text.h"

namespace qftbx {

namespace {

//What the field shows for one coefficient: the name of an uncertain one,
//the value of a fixed one. The RAW nominal, because the field is read back
//through the reparametrisation and not past it.
QString token(const Parameter & parameter)
{
    return parameter.isUncertain() ? QString::fromStdString(parameter.name())
                                   : numberText(parameter.rawNominal());
}

QString coefficients(const std::vector<Parameter> & polynomial)
{
    QString text;
    for (const Parameter & parameter : polynomial) {
        if (!text.isEmpty()) {
            text += QLatin1Char(' ');
        }
        text += token(parameter);
    }

    return text;
}

}

SystemDescription describeSystem(LtiSystem & system)
{
    SystemDescription described;

    described.name = QString::fromStdString(system.name());
    described.type = system.type();

    if (described.type == LtiSystem::SystemType::FreeForm) {
        described.numerator = QString::fromStdString(system.numeratorString());
        described.denominator = QString::fromStdString(system.denominatorString());
    } else {
        described.numerator = coefficients(system.numerator());
        described.denominator = coefficients(system.denominator());
    }

    //The gain and the delay are the ONE place where the field holds the
    //value and not the name: an uncertain one keeps its interval in the
    //uncertainty dialog, and the form reads this field as the nominal.
    described.gain = numberText(system.gain().rawNominal());
    described.delay = numberText(system.delay().rawNominal());

    return described;
}

} // namespace qftbx
