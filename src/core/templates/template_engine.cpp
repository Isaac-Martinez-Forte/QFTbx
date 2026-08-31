#include "template_engine.h"

#include "Modelo/Herramientas/exception.h"

#include <QDebug>
#include <QElapsedTimer>
#include <algorithm>
#include <iostream>

using namespace std;
using namespace mup;

#ifdef CUDA_AVAILABLE
//GPU epsilon-hull (relaxed-walk semantics, see the header).
#include "src/core/gpu/template_contour_cuda.h"
#endif

namespace qftbx {


TemplateEngine::TemplateEngine()
{
}

TemplateEngine::~TemplateEngine(){
}

void TemplateEngine::setGrids(QHash<QString, QVector<qreal> *> *mapa){
    m_grids = mapa;
}

void TemplateEngine::setEpsilon(QVector<qreal> *epsilon){
    m_epsilon = epsilon;
}

void TemplateEngine::setClouds(QVector<QVector<std::complex<qreal> > *> *templates){
    m_clouds = templates;
}

bool TemplateEngine::compute(LtiSystem *plant, QVector<qreal> *omega, bool cuda){
    m_useCuda = cuda;

    QElapsedTimer timer;
    timer.start();
    m_frequencies = omega;

    m_clouds = computeClouds(plant, omega);

    qDebug() << "Calcular plantilla: " << timer.elapsed() << "milliseconds";


    if (m_clouds == NULL){
        throw qftbx::ComputationError("Could not compute the templates.");
    }

    QElapsedTimer timer2;
    timer2.start();

    bool result = computeContourSet(cuda);

    qDebug() << "Calcular contorno: " << timer2.elapsed() << "milliseconds";

    return result;

}

bool TemplateEngine::computeContours(QVector<qreal> *epsilon){

    if (m_clouds == NULL){
        throw qftbx::InvalidInput("There are no templates to compute contours from.");
    }
    if (epsilon == NULL || epsilon->size() < m_clouds->size()){
        throw qftbx::InvalidInput("Missing epsilon values for the template contours.");
    }

    m_epsilon = epsilon;
    QElapsedTimer timer;
    timer.start();

    bool result = computeContourSet(m_useCuda);

    qDebug() << "Calcular contorno: " << timer.elapsed() << "milliseconds";

    return result;
}

QVector<QVector<complex<qreal> > *> * TemplateEngine::clouds(){
    return m_clouds;
}

QVector<QVector<complex<qreal> > *> * TemplateEngine::contours(){
    return m_contours;
}

QVector<qreal> * TemplateEngine::gridFor(Parameter * a){

    //Keyed by NAME: pointer identity went stale on every clone() or
    //project reload.
    QVector<qreal> * values = m_grids->value(a->name());

    if (values == NULL){
        throw qftbx::InvalidInput("Missing sweep grid for the uncertain parameter '"
                                  + a->name().toStdString() + "'.");
    }

    return values;
}

QVector<QVector<complex<qreal> > * > * TemplateEngine::computeClouds(LtiSystem *plant, QVector<qreal> *omega){

    //Collect the uncertain parameters (the first of each name) and their
    //grids, in numerator, denominator, gain, delay order. The index in
    //'names' is the odometer digit (0 is the fastest), as historically.
    QVector <QString> names;
    QVector <const QVector <qreal> *> grids;

    m_combinationCount = 1;

    auto collect = [&](Parameter * var){
        if (var->isUncertain() && !names.contains(var->name())){
            const QVector <qreal> * rejilla = gridFor(var);
            names.append(var->name());
            grids.append(rejilla);
            m_combinationCount *= rejilla->size();
        }
    };

    foreach (Parameter * var, *plant->numerator())
        collect(var);
    foreach (Parameter * var, *plant->denominator())
        collect(var);
    collect(plant->gain());
    collect(plant->delay());

    const qint32 digitCount = names.size();
    const qint32 frequencyCount = omega->size();

    QVector<QVector<complex<qreal> > * > * allClouds = new QVector <QVector<complex<qreal> > * > (frequencyCount);

#ifdef OpenMP_AVAILABLE
#pragma omp parallel for
#endif
    for (qint32 u = 0; u < frequencyCount; u++){

        //One parser per frequency: the expression is parsed ONCE and every
        //parameter is BOUND to a mup::Value mutated by the odometer (the
        //old loop re-parsed the whole expression per combination, plus one
        //parsed assignment per digit).
        ParserX parser (pckALL_COMPLEX);

        std::vector <mup::Value> values (digitCount);
        for (qint32 j = 0; j < digitCount; j++){
            values[j] = mup::Value(grids.at(j)->at(0));
            parser.DefineVar(names.at(j).toStdString(), mup::Variable(&values[j]));
        }

        parser.SetExpr(plant->expression(omega->at(u)).toStdString());

        QVector <complex<qreal>> * cloud = new QVector <complex<qreal>> ();
        cloud->reserve(m_combinationCount);

        QVector <qint32> counter (digitCount + 1, 0);

        for (qint32 i = 0; i < m_combinationCount; i++){

            cloud->append(parser.Eval().GetComplex());

            counter[0]++;
            for (qint32 j = 0; j < digitCount; j++){
                if (counter.at(j) >= grids.at(j)->size()){
                    counter[j] = 0;
                    counter[j+1]++;
                    values[j] = grids.at(j)->at(0);
                }else {
                    values[j] = grids.at(j)->at(counter.at(j));
                    break;
                }
            }
        }

        //Every frequency writes at its own index: no critical sections,
        //no permutations.
        allClouds->replace(u, cloud);
    }

    return allClouds;
}

QVector <qreal> * TemplateEngine::getOmega(){
    return m_frequencies;
}

QVector <qreal> * TemplateEngine::getEpsilon(){
    return m_epsilon;
}

bool TemplateEngine::computeContourSet(bool cuda __attribute__((unused))){

    bool succeeded = true;
    qint32 digitCount = m_clouds->size();

#ifdef CUDA_AVAILABLE
    if (cuda){
        //GPU path (relaxed-walk semantics: the parity reference is
        //epsilonHullRelaxed, not the faithful walk - see the header).
        m_contours = new QVector <QVector <complex <qreal> > * > ();

        for (qint32 i = 0; i < digitCount; i++){

            const vector <complex <double> > hull = epsilonHullCuda(
                std::vector<complex<double>>(m_clouds->at(i)->begin(), m_clouds->at(i)->end()),
                m_epsilon->at(i));

            if (hull.empty()){
                succeeded = false;
            }
            m_contours->append(new QVector <complex <qreal> > (hull.begin(), hull.end()));
        }

        if (!succeeded){
            throw ComputationError("Could not compute the template contours.");
        }

        return true;
    }
#endif

    //CPU path, shared with and without OpenMP. Every iteration writes at
    //the index of ITS frequency: a shared counter used to permute the
    //contours in thread-arrival order (desynchronising them from the
    //clouds), then 'repaired' it by clearing the live vectors of Omega and
    //of the GUI (aliasing). Without the permutation they stay intact.
    m_contours = new QVector <QVector <complex <qreal> > * > (digitCount);

#ifdef OpenMP_AVAILABLE
#pragma omp parallel for
#endif
    for (qint32 i = 0; i < digitCount; i++){

        QVector <complex <qreal> > * cont = epsilonHull(m_clouds->at(i), m_epsilon->at(i));

        if (cont == NULL){
            cont = new QVector <complex <qreal> >();

#ifdef OpenMP_AVAILABLE
#pragma omp critical
#endif
            {
                succeeded = false;
            }
        }

        m_contours->replace(i, cont);
    }
    if (!succeeded){
        throw qftbx::ComputationError("Could not compute the template contours.");
    }

    return true;
}


//Faithful port of EPSHULL.M (epsh2, Montoya 1998; the algorithm defined in
//Nordin 1993). Deliberate divergence: with no initial candidate it returns
//NULL instead of an empty contour (the caller treats it as an error).
QVector <complex <qreal> > * TemplateEngine::epsilonHull(QVector<complex<qreal> > *temp, qreal epsilon){

    if (temp == NULL || temp->isEmpty()){
        return NULL;
    }

    //unique(cv): deduplicated and sorted with MATLAB's ordering for complex
    //values (modulus, then phase). This order also resolves the psi ties
    //exactly like the reference.
    QVector <complex <qreal> > cv = *temp;
    std::sort(cv.begin(), cv.end(),
              [](const complex<qreal> & a, const complex<qreal> & b){
                  const qreal absA = abs(a);
                  const qreal absB = abs(b);
                  if (absA != absB){
                      return absA < absB;
                  }
                  return arg(a) < arg(b);
              });
    cv.erase(std::unique(cv.begin(), cv.end()), cv.end());

    const qint32 pointCount = cv.size();
    const qint32 MAXP = 3 * pointCount;

    //First point: the largest real part (the rightmost one), as in
    //EPSHULL.M and in the PFC text. Ties: the first in unique order wins.
    qint32 b1 = 0;
    for (qint32 i = 1; i < pointCount; i++){
        if (real(cv.at(i)) > real(cv.at(b1))){
            b1 = i;
        }
    }

    qint32 b2 = findSecond(b1, &cv, epsilon);

    if (b2 < 0)
        return NULL;

    QVector <qint32> walk;
    walk.append(b1);
    walk.append(b2);

    qint32 previousPoint = b1;
    qint32 currentPoint = b2;

    qint32 nextPoint = findNext(b1, b2, &cv, epsilon);
    if (nextPoint < 0)
        return NULL;

    qint32 counter = 2;

    //Stops when the walk returns to the initial (b1, b2) pair. Points may
    //repeat (out-and-back over template spikes): they are kept, like in
    //the MATLAB, as real geometric information of the contour.
    while (b1 != currentPoint || b2 != nextPoint){

        walk.append(nextPoint);
        counter++;

        if (counter > MAXP){
            //The reference (EPSHULL.M) cycles without closing on clouds of
            //clusters spaced about epsilon apart ('max_puntos_excedido').
            //Documented fallback to the historical walk, which always
            //yields a contour with coverage <= epsilon even if it is not
            //the canonical epsilon-hull.
            qWarning("epsilonHull: the reference walk did not close (epsilon-hull "
                     "limitation on clustered templates); falling back to the "
                     "relaxed historical walk for this frequency.");
            return epsilonHullRelaxed(temp, epsilon);
        }

        previousPoint = currentPoint;
        currentPoint = nextPoint;

        nextPoint = findNext(previousPoint, currentPoint, &cv, epsilon);

        if (nextPoint < 0){
            return NULL;
        }
    }

    QVector <complex <qreal> > * result = new QVector <complex <qreal> > ();
    result->reserve(walk.size());

    foreach (const qint32 var, walk) {
        result->append(cv.at(var));
    }

    return result;
}

QVector <complex <qreal> > * TemplateEngine::epsilonHullRelaxed(QVector<complex<qreal> > *temp, qreal epsilon){

    qint32 pointCount = temp->size();
    qint32 MAXP = 3 * pointCount;

    qint32 b1 = 0;
    qreal numDe = -numeric_limits<qreal>::infinity();

    for(qint32 i = 0;i < pointCount ; i++){   //first point: largest imaginary part.
        if (imag(temp->at(i)) > numDe){
            b1 = i;
            numDe = imag(temp->at(i));
        }
    }

    qint32 b2 = findSecond(b1, temp, epsilon);

    if (b2 < 0)
        return NULL;

    QVector <qint32> walk;
    walk.append(b1);
    walk.append(b2);

    qint32 previousPoint = b1;
    qint32 currentPoint = b2;

    qint32 nextPoint = findNext(b1, b2, temp, epsilon, true);
    if (nextPoint < 0)
        return NULL;

    qint32 counter = 2;

    while (b1 != currentPoint || b2 != nextPoint){

        walk.append(nextPoint);
        counter++;

        if (counter > MAXP)
            break;      //silent truncation: partial contour (historical behaviour).

        previousPoint = currentPoint;
        currentPoint = nextPoint;

        nextPoint = findNext(previousPoint, currentPoint, temp, epsilon, true);

        if (nextPoint < 0){
            return NULL;
        }
    }

    //Output deduplication (historical behaviour).
    QVector <qint32> uniqueIdx;
    foreach (qint32 idx, walk) {
        if (!uniqueIdx.contains(idx)){
            uniqueIdx.append(idx);
        }
    }

    QVector <complex <qreal> > * result = new QVector <complex <qreal> > ();
    result->reserve(uniqueIdx.size());

    foreach (const qint32 idx, uniqueIdx) {
        result->append(temp->at(idx));
    }

    return result;
}

qint32 TemplateEngine::findSecond(qint32 b1, QVector<complex<qreal> > *cv, qreal epsilon){

    qreal dist = 0;
    complex <qreal> firstPoint = cv->at(b1);

    qreal fmin = numeric_limits<qreal>::infinity();
    qint32 pmin = -1;
    qreal dmax = 0;

    complex <qreal> candidate;

    qreal fas = 0;

    for (qint32 i = 0; i < cv->size(); i++){    //recorremos todo el vector de puntos.

        candidate = cv->at(i);
        dist = abs(firstPoint - candidate); //calculamos el valor absoluto de la resta.

        if (dist > 0 && dist <= epsilon){    //candidates within epsilon.

            fas = arg (candidate - firstPoint); //phase of the difference

            if (fas < 0)        // si dicha fase es menor que cero le sumamos 2*PI
                fas += 2 * M_PI;

            //subtract from the phase the arccosine of distance over epsilon.
            fas -= qAcos(dist / epsilon);

            if (fas < fmin){   //keep the minimum phase
                fmin = fas;
                pmin = i;
                dmax = dist;

            }else if (fas == fmin && dist > dmax){ //si son iguales guardamos aquella que su distancia sea mayor.
                pmin = i;
                dmax = dist;
            }
        }
    }

    return pmin;
}

qint32 TemplateEngine::findNext(qint32 previousPoint, qint32 currentPoint,
                                  QVector<complex<qreal> > *cv, qreal epsilon,
                                  bool excludePrevious){

    complex <qreal> current = cv->at(currentPoint);
    complex <qreal> previous = cv->at(previousPoint);

    qreal aco2 = qAcos(abs(previous-current) / epsilon);

    qreal phase = 0;

    qreal aco1 = 0;
    qreal dmax = 0;

    qreal psi = 0;


    qreal psiMin = numeric_limits<qreal>::infinity();
    qint32 bestIndex = -1;

    complex <qreal> candidate;
    qreal distance;

    for (qint32 i = 0; i < cv->size(); i++){

        candidate = cv->at(i);
        distance = abs(candidate - current); //Calculamos el valor absoluto de la distancia del punto al punto current.


        //Candidatos: todo punto a distancia (0, epsilon] del actual, INCLUIDO
        //el anterior (en EPSHULL.M su exclusion esta deliberadamente
        //comentada): entra por el caso fas==0 y es lo que permite volver
        //atras por puas y ramas finas del template. La variante relajada
        //historica lo excluye.
        if (distance > 0 && distance <= epsilon &&
                !(excludePrevious && (candidate == previous || candidate == current))){

            //--------------------------------------------

            //calculamos la fase entre los dos puntos normalizada.
            phase = arg((candidate - current) / (previous - current));

            if(phase < 0) // si es menor que cero le sumamos 2*PI.
                phase +=  2 * M_PI;

            //------------------------------------------------------------

            aco1 = qAcos(distance / epsilon );  //calculamos la arcosecante de dicho valor absoluto

            //------------------------------------------------------------

            if(phase == 0){  //psi has three cases, as in EPSHULL.M
                psi =  2 * M_PI - aco1 - aco2;
            }else if (phase > 0 && phase < aco2){
                psi = phase + aco1- aco2;
            }else{
                psi = phase - aco1 - aco2;
            }

            if (psi < 0)                   // Si alguno es negativo se sumamos 2*PI
                psi +=  2 * M_PI;

            //------------------------------------------------------------

            if (psi < psiMin) {       //keep the minimum psi
                psiMin = psi;         //como puede haber iguales nos quedamos con el de la
                bestIndex = i;                   //distancia mas grande.
                dmax = distance;
            }else if (psi == psiMin &&
                      (distance > dmax)){
                bestIndex = i;
                dmax = distance;
            }
        }
    }

    return bestIndex;

}

} // namespace qftbx
