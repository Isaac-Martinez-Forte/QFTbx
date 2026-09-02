#include "src/core/math/sequence_vectors.h"

#include <QRegularExpression>

#include "src/core/math/sequences.h"

using namespace std;



//Wrapper over the canonical implementation in src/core/math/ (no
//accumulation drift, exact final endpoint).
QVector <qreal> tools::linspace(qreal a, qreal b, qint32 N) {
    const std::vector<double> values = qftbx::math::linspace(a, b, N > 0 ? N : 0);
    return QVector<qreal>(values.begin(), values.end());
}


std::vector<float> tools::linspace1(qreal a, qreal b, qint32 N){
    //N == 1 divided by zero here. The canonical linspace was fixed for it and
    //this float variant, kept for the CUDA path, was not.
    if (N <= 0){
        return std::vector<float>();
    }
    if (N == 1){
        return std::vector<float>(1, static_cast<float>(a));
    }

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


//See tools::linspace.
QVector <qreal> tools::logspace (qreal a, qreal b, qint32 N){
    const std::vector<double> values = qftbx::math::logspace(a, b, N > 0 ? N : 0);
    return QVector<qreal>(values.begin(), values.end());
}
