#include "tools.h"

#include <QRegularExpression>

#include "src/core/math/sequences.h"

using namespace std;



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
