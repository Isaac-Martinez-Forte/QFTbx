#include "src/core/text_tokens.h"

#include <QRegularExpression>

QVector<QString> * qftbx::text::tokens(const QString & line){

    QStringList parts = line.split(" ");
    QVector<QString> * result = new QVector<QString>();
    result->reserve(parts.size());

    foreach (const QString & part, parts){
        if (!part.isEmpty()){
            result->append(part);
        }
    }

    return result;
}


QVector<qreal> * qftbx::text::reals(const QString & line){

    //Split on any whitespace (spaces, tabs, newlines): frequency files
    //usually carry one value per line.
    QList<QString> parts = line.split(QRegularExpression("\\s+"),
                                                Qt::SkipEmptyParts);
    QList<qreal> values;

    bool ok = false;

    foreach (const QString & part, parts){
        const qreal value = part.toDouble(&ok);

        if (!ok){
            return nullptr;
        }

        values.append(value);
    }

    return new QVector<qreal>(values.begin(), values.end());
}



