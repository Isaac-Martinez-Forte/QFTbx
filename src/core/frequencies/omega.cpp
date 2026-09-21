/**
 * @file
 * @brief Construction and file reading of the design frequency set.
 *
 * Every value must be a finite, strictly positive real: the values arrive
 * from a file as often as from a dialog, and strtod reads "nan", "inf" and
 * "-1" as numbers. The point count is always the size of the values; the
 * count passed to the constructor is ignored because files can carry one
 * that disagrees. The frequency file is read whole with the standard
 * library and rejected entirely when any token is not a real.
 */

#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#include <cmath>
#include <cstdint>
#include <optional>
#include "src/core/frequencies/omega.h"
#include "src/core/common/text_tokens.h"

#include "src/core/common/exception.h"

namespace qftbx {

namespace {

void requireUsable(const std::vector<double> & values)
{
    if (values.empty()){
        throw qftbx::InvalidInput(QFTBX_TR("Core", "A frequency set needs at least one value."));
    }

    for (const double value : values) {
        if (!std::isfinite(value) || value <= 0.0) {
            throw qftbx::InvalidInput(QFTBX_TR("Core", "A design frequency must be a finite positive real, and %1 is not.").arg(value));
        }
    }
}

}

Omega::Omega(double start, double end, std::int32_t pointCount, std::vector<double> values, GenerationType type)
{
    (void) pointCount;

    requireUsable(values);

    m_start = start;
    m_end = end;
    m_pointCount = static_cast<std::int32_t>(values.size());
    m_values = std::move(values);
    m_type = type;
}

double Omega::start() const {
    return m_start;
}

double Omega::end() const {
    return m_end;
}

std::int32_t Omega::pointCount() const {
    return m_pointCount;
}

std::vector<double> * Omega::values(){
    return &m_values;
}

const std::vector<double> * Omega::values() const{
    return &m_values;
}

Omega::GenerationType Omega::type() const {
    return m_type;
}

void Omega::setOmega(std::vector<double> values){
    requireUsable(values);

    m_pointCount = static_cast<std::int32_t>(values.size());
    m_values = std::move(values);
}

std::vector<double> Omega::valuesFromFile(std::string path){

    std::ifstream file (path);

    if (!file.is_open()){
        throw qftbx::FileError(QFTBX_TR("Core", "Cannot open frequencies file: %1").arg(path));
    }

    file.seekg(0, std::ios::end);
    std::string contents;
    contents.resize(static_cast<std::size_t>(file.tellg()));
    file.seekg(0, std::ios::beg);
    file.read(&contents[0], static_cast<std::streamsize>(contents.size()));
    contents.resize(static_cast<std::size_t>(file.gcount()));

    const std::optional<std::vector<double>> values = qftbx::text::reals(contents);

    if (!values.has_value() || values->empty()){
        throw qftbx::FileError(QFTBX_TR("Core", "The frequencies file contains no valid values: %1").arg(path));
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

}
