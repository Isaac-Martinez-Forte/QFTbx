#ifndef TOOLS_H
#define TOOLS_H

#include <vector>

#include <QString>
#include <QVector>

//Transitional re-exports: these types moved to their own homes; consumers
//will include them directly as each module is migrated.
#include "Modelo/EstructurasDatos/dbnd.h"
#include "src/core/loopshaping/loop_shaping_types.h"

/**
 * @namespace tools
 * @brief Remaining free helpers, pending relocation as their consumer
 * modules are migrated (see REFACTOR notes in each function).
 */
namespace tools{

//Transitional wrappers over qftbx::math (src/core/math/sequences.h).
QVector <qreal> * linspace(qreal a, qreal b, qint32 N);
QVector <qreal> * logspace (qreal a, qreal b, qint32 N);

//Float variant kept verbatim for the CUDA path (deferred).
std::vector <float> linspace1(qreal a, qreal b, qint32 N);

/// Splits a string into its whitespace-separated tokens.
QVector <QString> * srtovectorString (QString cadena);

/// Parses whitespace-separated reals; returns null on any invalid token.
QVector <qreal> * srtovectorReal (QString cadena);

}

#endif // TOOLS_H
