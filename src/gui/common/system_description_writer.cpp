/**
 * @file
 * @brief Formatting of the coefficients, gain and delay of a system.
 *
 * A coefficient is written from its raw nominal at full precision, because
 * the field is read back through the reparametrisation and not past it. The
 * gain and the delay are the one place where the field holds the value and
 * not the name even when uncertain: the form reads them as the nominal and
 * the interval stays in the uncertainty dialog.
 */

#include "src/gui/common/system_description_writer.h"

#include "src/gui/common/number_text.h"

namespace qftbx {

namespace {

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

    described.gain = numberText(system.gain().rawNominal());
    described.delay = numberText(system.delay().rawNominal());

    return described;
}

}
