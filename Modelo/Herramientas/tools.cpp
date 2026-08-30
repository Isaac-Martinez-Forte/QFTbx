#include "tools.h"

#include <QRegularExpression>

#include "src/core/math/sequences.h"

using namespace std;

void tools::menerror(QString mensaje, QString nombre){

    QMessageBox::critical(nullptr, nombre,mensaje,
                         QMessageBox::Close);
}


//Wrapper transitorio sobre la implementacion canonica de src/core/math/
//(sin deriva de acumulacion, extremo final exacto). Desaparece cuando cada
//llamante migre a qftbx::math directamente.
QVector <qreal> * tools::linspace(qreal a, qreal b, qint32 N) {
    const std::vector<double> values = qftbx::math::linspace(a, b, N > 0 ? N : 0);
    return new QVector<qreal>(values.begin(), values.end());
}


std::vector<float> tools::linspace1(qreal a, qreal b, qint32 N){
    float h = (b - a) / (N-1);
    vector <float> vec;
    vec.reserve(N);

    float val = a;

    for (qint32 i = 0; i < N; i++){ // https://gist.github.com/jmbr/2375233
        vec.push_back(val);
        val+=h;
    }

    return vec;
}


//Wrapper transitorio: ver tools::linspace.
QVector <qreal> * tools::logspace (qreal a, qreal b, qint32 N){
    const std::vector<double> values = qftbx::math::logspace(a, b, N > 0 ? N : 0);
    return new QVector<qreal>(values.begin(), values.end());
}


QVector <QString> * tools::srtovectorString (QString cadena){

    QStringList listacadenas = cadena.split(" ");
    QVector <QString> * cadenas = new QVector <QString> ();
    cadenas->reserve(listacadenas.size());

    foreach (const QString &i, listacadenas){
        if (i.compare("") != 0){
            cadenas->append(i);
        }
    }
    return cadenas;
}


QVector <qreal> * tools::srtovectorReal (QString cadena){

    //Se separa por cualquier blanco (espacios, tabuladores, saltos de
    //linea): los ficheros de frecuencias suelen traer un valor por linea.
    QList <QString> listacadenas = cadena.split(QRegularExpression("\\s+"),
                                                Qt::SkipEmptyParts);
    QList <qreal> listareales;

    bool ok = false;

    foreach (const QString &i, listacadenas){
        if (i.compare("") != 0){
            listareales.append(i.toDouble(&ok));
            if (ok != true){
                return NULL;
            }else{
                ok = false;
            }
        }

    }

    QVector <qreal> *numeros = new QVector <qreal>(listareales.toVector());

    return numeros;
}


QColor tools::ramdonColor (qint32 i){

    qint32 color = i;

    switch (color){
    case 0: return Qt::red;
    case 1: return Qt::darkYellow;
    case 2: return Qt::green;
    case 4: return Qt::magenta;
    case 5: return Qt::darkGreen;
    case 6: return Qt::blue;
    case 7: return Qt::darkBlue;
    case 8: return Qt::darkCyan;
    case 9: return Qt::darkGray;
    case 10: return Qt::darkMagenta;
    case 11: return Qt::yellow;
    case 12: return Qt::darkYellow;
    default: return Qt::cyan;
    }

}

QVector <Parameter *> * tools::clonarVectorVar(QVector<Parameter *> *v){
    QVector <Parameter *> * nuevo = new QVector <Parameter * > ();

    foreach (Parameter * v, *v) {
       nuevo->append(v->clone());
    }

    return nuevo;
}
