#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#include <cstdint>
#include "src/core/frequencies/omega.h"
#include "src/core/text_tokens.h"


#include "src/core/exception.h"

Omega::Omega(double start, double end, std::int32_t pointCount, std::vector<double> values, GenerationType type)
{
    (void) pointCount;   //old files carry a desynchronised point count

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

std::vector<double> Omega::valuesFromFile(std::string path){

    //std::ifstream and not QFile: this is the core, and reading a text file
    //of numbers needs nothing from Qt.
    std::ifstream file (path);

    if (!file.is_open()){
        throw qftbx::FileError("Cannot open frequencies file: " + path);
    }

    file.seekg(0, std::ios::end);
    std::string contents;
    contents.resize(static_cast<std::size_t>(file.tellg()));
    file.seekg(0, std::ios::beg);
    file.read(&contents[0], static_cast<std::streamsize>(contents.size()));
    contents.resize(static_cast<std::size_t>(file.gcount()));

    const std::optional<std::vector<double>> values = qftbx::text::reals(contents);

    if (!values.has_value() || values->empty()){
        throw qftbx::FileError("The frequencies file contains no valid values: "
                               + path);
    }

    return values.value();
}

bool Omega::sameAs(const Omega & other) const
{
    return m_type == other.m_type &&
            m_start == other.m_start &&
            m_end == other.m_end &&
            m_pointCount == other.m_pointCount &&
            m_values == other.m_values;
}

