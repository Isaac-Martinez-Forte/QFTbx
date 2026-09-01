#include "src/core/frequencies/omega.h"
#include "src/core/text_tokens.h"

#include <QFile>
#include <QTextStream>

#include "src/core/exception.h"

Omega::Omega(qreal start, qreal end, qint32 pointCount, QVector<qreal> * values, GenerationType type)
{
    Q_UNUSED(pointCount);

    if (values == nullptr || values->isEmpty()){
        delete values;
        throw qftbx::InvalidInput("A frequency set needs at least one value.");
    }

    m_start = start;
    m_end = end;
    //Invariant: m_pointCount == m_values->size() always. The parameter is
    //deliberately ignored: old files carry a desynchronised <nPuntos>.
    m_pointCount = values->size();
    m_values = values;
    m_type = type;
}

Omega::~Omega(){
    delete m_values;
}

qreal Omega::start(){
    return m_start;
}

qreal Omega::end(){
    return m_end;
}

qint32 Omega::pointCount(){
    return m_pointCount;
}

QVector<qreal> * Omega::values(){
    return m_values;
}

Omega::GenerationType Omega::type(){
    return m_type;
}

void Omega::setValues(QVector<qreal> * values){

    if (values == nullptr || values->isEmpty()){
        if (values != m_values){
            delete values;
        }
        throw qftbx::InvalidInput("A frequency set needs at least one value.");
    }

    //Templates/Boundaries sometimes hand back the very vector we own.
    if (values == m_values){
        m_pointCount = m_values->size();
        return;
    }

    delete m_values;
    m_pointCount = values->size();
    m_values = values;
}

QVector<qreal> * Omega::valuesFromFile(QString path){

    QFile file (path);

    if (!file.open(QIODevice::ReadOnly)){
        throw qftbx::FileError("Cannot open frequencies file: " + path.toStdString());
    }

    QTextStream in (&file);
    QVector<qreal> * values = qftbx::text::reals(in.readAll());

    if (values == nullptr || values->isEmpty()){
        delete values;
        throw qftbx::FileError("The frequencies file contains no valid values: "
                               + path.toStdString());
    }

    return values;
}
