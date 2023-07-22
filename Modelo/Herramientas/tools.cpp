#include "tools.h"

using namespace std;

void tools::menerror(QString mensaje, QString nombre){

    QMessageBox::critical(nullptr, nombre,mensaje,
                         QMessageBox::Close);
}


QVector <qreal> * tools::linspace(qreal a, qreal b, qint32 N) {
    qreal h = (b - a) / (N-1);
    QVector <qreal> * vec = new QVector<qreal> ();
    vec->reserve(N);

    qreal val = a;

    for (qint32 i = 0; i < N; i++){ // https://gist.github.com/jmbr/2375233
        vec->append(val);
        val+=h;
    }

    return vec;
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


QVector <qreal> * tools::logspace (qreal a, qreal b, qint32 log){
    QVector <qreal> * potencias = linspace(a,b,log);
    QVector <qreal> * vector = new QVector<qreal> ();
    vector->reserve(log);

    qint32 cont = 0;

    foreach(const qreal &i, *potencias){
        vector->insert(cont, qPow(10,i));
        cont++;
    }
    delete potencias;
    return vector;
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

    QList <QString> listacadenas = cadena.split(" ");
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

QVector <Var *> * tools::clonarVectorVar(QVector<Var *> *v){
    QVector <Var *> * nuevo = new QVector <Var * > ();

    foreach (Var * v, *v) {
       nuevo->append(v->clone());
    }

    return nuevo;
}
