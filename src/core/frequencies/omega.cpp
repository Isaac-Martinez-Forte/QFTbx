#include <vector>
#include <cstdint>
#include "src/core/frequencies/omega.h"
#include "src/core/text_tokens.h"

#include <QFile>
#include <QTextStream>

#include "src/core/exception.h"

Omega::Omega(double start, double end, std::int32_t pointCount, std::vector<double> values, GenerationType type)
{
    Q_UNUSED(pointCount);

    if (values.empty()){
        throw qftbx::InvalidInput("A frequency set needs at least one value.");
    }

    m_start = start;
    m_end = end;
    //Invariant: m_pointCount == m_values.size() always. The parameter is
    //deliberately ignored: old files carry a desynchronised <nPuntos>.
    m_pointCount = values.size();
    m_values = std::move(values);
    m_type = type;
}

double Omega::start(){
    return m_start;
}

double Omega::end(){
    return m_end;
}

std::int32_t Omega::pointCount(){
    return m_pointCount;
}

std::vector<double> * Omega::values(){
    return &m_values;
}

Omega::GenerationType Omega::type(){
    return m_type;
}

void Omega::setOmega(std::vector<double> values){

    if (values.empty()){
        throw qftbx::InvalidInput("A frequency set needs at least one value.");
    }

    //Templates/Boundaries sometimes hand back the very frequencies we hold;
    //by value that is a copy made before the assignment, so the aliasing
    //guard the pointer version needed has nothing left to guard.
    m_pointCount = values.size();
    m_values = std::move(values);
}

std::vector<double> Omega::valuesFromFile(QString path){

    QFile file (path);

    if (!file.open(QIODevice::ReadOnly)){
        throw qftbx::FileError("Cannot open frequencies file: " + path.toStdString());
    }

    QTextStream in (&file);
    const std::optional<std::vector<double>> values = qftbx::text::reals(in.readAll());

    if (!values.has_value() || values->empty()){
        throw qftbx::FileError("The frequencies file contains no valid values: "
                               + path.toStdString());
    }

    return values.value();
}
