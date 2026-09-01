#include "template_engine.h"

#include <QStringList>

#include "src/core/exception.h"
#include "src/core/math/parser_warmup.h"

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

void TemplateEngine::setGrids(ParameterGrids grids){
    m_grids = std::move(grids);
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

    //Nobody owns the clouds yet: the facade publishes them only when this
    //returns, so a throw from here would leak everything computeClouds just
    //built. computeClouds frees its own work on its two throw paths; this is
    //the gap between them and the caller.
    bool result = false;

    try {
        result = computeContourSet(cuda);
    } catch (...) {
        qDeleteAll(*m_clouds);
        delete m_clouds;
        m_clouds = nullptr;

        throw;
    }

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

const std::vector<double> & TemplateEngine::gridFor(Parameter & a){

    //Keyed by NAME: pointer identity went stale on every clone() or
    //project reload.
    const auto found = m_grids.find(a.name());

    if (found == m_grids.end()){
        throw qftbx::InvalidInput("Missing sweep grid for the uncertain parameter '"
                                  + a.name().toStdString() + "'.");
    }

    return found->second;
}

QVector<QVector<complex<qreal> > * > * TemplateEngine::computeClouds(LtiSystem *plant, QVector<qreal> *omega){

    //Collect the uncertain parameters (the first of each name) and their
    //grids, in numerator, denominator, gain, delay order. The index in
    //'names' is the odometer digit (0 is the fastest), as historically.
    QVector <QString> names;

    //Pointers INTO the map the engine owns: stable for the whole sweep,
    //because nothing modifies m_grids while it runs.
    std::vector <const std::vector<double> *> grids;

    m_combinationCount = 1;

    auto collect = [&](Parameter & var){
        if (var.isUncertain() && !names.contains(var.name())){
            const std::vector<double> & rejilla = gridFor(var);
            names.append(var.name());
            grids.push_back(&rejilla);
            m_combinationCount *= static_cast<qint32>(rejilla.size());
        }
    };

    for (Parameter & var : plant->numerator())
        collect(var);
    for (Parameter & var : plant->denominator())
        collect(var);
    collect(plant->gain());
    collect(plant->delay());

    const qint32 digitCount = names.size();
    const qint32 frequencyCount = omega->size();

    //Which odometer digit drives each coefficient, and the nominals of the
    //ones no digit drives. Built ONCE and sequentially: Parameter::nominal()
    //can evaluate a reparametrisation, and a coefficient's plan does not
    //change with the frequency or the combination. A name appearing twice
    //shares its digit, which is what makes it one variable.
    const auto slotOf = [&](Parameter & parameter) -> qint32 {
        return parameter.isUncertain() ? names.indexOf(parameter.name()) : -1;
    };

    std::vector<qint32> numeratorSlot, denominatorSlot;
    std::vector<qreal> numeratorNominal, denominatorNominal;

    for (Parameter & parameter : plant->numerator()){
        numeratorSlot.push_back(slotOf(parameter));
        numeratorNominal.push_back(parameter.nominal());
    }
    for (Parameter & parameter : plant->denominator()){
        denominatorSlot.push_back(slotOf(parameter));
        denominatorNominal.push_back(parameter.nominal());
    }

    const qint32 gainSlot = slotOf(plant->gain());
    const qint32 delaySlot = slotOf(plant->delay());
    const qreal gainNominal = plant->gain().nominal();
    const qreal delayNominal = plant->delay().nominal();

    QVector<QVector<complex<qreal> > * > * allClouds = new QVector <QVector<complex<qreal> > * > (frequencyCount);

    //One flag and one error slot per frequency, filled inside the parallel
    //loop below (nothing may be thrown from within it).
    QVector <bool> nonFiniteFrequencies (frequencyCount, false);
    QVector <QString> parserErrors (frequencyCount);

    //Before the threads exist: muParserX's package singletons are built
    //lazily and unsynchronised, and the loop below constructs one parser per
    //frequency. See warmUpExpressionParser().
    qftbx::math::warmUpExpressionParser();

#ifdef OpenMP_AVAILABLE
#pragma omp parallel for
#endif
    for (qint32 u = 0; u < frequencyCount; u++){

        //No parser and no expression TEXT any more: the transfer function is
        //computed directly in complex arithmetic by valueAt(). The text route
        //cost a parse per frequency, constructed a parser inside this loop -
        //racing on muParserX's unsynchronised package singletons - and, being
        //written with QString::number(), evaluated the plant with its
        //constant coefficients AND ITS FREQUENCY rounded to six significant
        //digits. The swept coefficients were exact only because they were
        //bound as variables.
        const qreal w = omega->at(u);

        std::vector<qreal> numeratorValues = numeratorNominal;
        std::vector<qreal> denominatorValues = denominatorNominal;

        std::vector <qreal> digit (digitCount);
        for (qint32 j = 0; j < digitCount; j++){
            digit[j] = (*grids.at(static_cast<std::size_t>(j)))[0];
        }

        QVector <complex<qreal>> * cloud = new QVector <complex<qreal>> ();
        cloud->reserve(m_combinationCount);

        QVector <qint32> counter (digitCount + 1, 0);

        //Non-finite plant values: an undamped resonance inside the
        //uncertainty makes |P(jw)| infinite at some frequency (the ACC'90
        //benchmark, whose resonance sqrt(2e) SWEEPS a whole band with e, is
        //the canonical case). The boundary sweep survives it - the limits
        //of the four closed-loop magnitudes are well defined as |p| grows -
        //but a cloud with infinite points has no magnitude grid that covers
        //it and no epsilon-hull that can walk it, so the contour would come
        //out as noise. The frequency is recorded and reported instead of
        //silently producing that noise (the literature's own answer is to
        //damp the resonance lightly: see the ACC'90 fixture).
        bool nonFinite = false;

        for (qint32 i = 0; i < m_combinationCount; i++){

            complex<qreal> value;

            for (std::size_t c = 0; c < numeratorSlot.size(); c++){
                if (numeratorSlot[c] >= 0){
                    numeratorValues[c] = digit[numeratorSlot[c]];
                }
            }
            for (std::size_t c = 0; c < denominatorSlot.size(); c++){
                if (denominatorSlot[c] >= 0){
                    denominatorValues[c] = digit[denominatorSlot[c]];
                }
            }

            const qreal gainValue = gainSlot >= 0 ? digit[gainSlot] : gainNominal;
            const qreal delayValue = delaySlot >= 0 ? digit[delaySlot] : delayNominal;

            //A free-form plant still evaluates an expression, so it can still
            //reject one (a name colliding with a muParserX constant, a
            //malformed plant). Nothing may be thrown from inside the OpenMP
            //region - that TERMINATES the process - so the message is kept
            //and rethrown after the loop.
            try {
                value = plant->valueAt(w, numeratorValues, denominatorValues,
                                       gainValue, delayValue);
            } catch (mup::ParserError & error) {
                parserErrors.replace(u, QString::fromStdString(error.GetMsg()));
                break;
            } catch (const qftbx::Exception & error) {
                parserErrors.replace(u, QString::fromUtf8(error.what()));
                break;
            }

            nonFinite = nonFinite || !std::isfinite(value.real()) || !std::isfinite(value.imag());
            cloud->append(value);

            counter[0]++;
            for (qint32 j = 0; j < digitCount; j++){
                if (counter.at(j) >= static_cast<qint32>(grids.at(static_cast<std::size_t>(j))->size())){
                    counter[j] = 0;
                    counter[j+1]++;
                    digit[j] = (*grids.at(static_cast<std::size_t>(j)))[0];
                }else {
                    digit[j] = (*grids.at(static_cast<std::size_t>(j)))[static_cast<std::size_t>(counter.at(j))];
                    break;
                }
            }
        }

        if (nonFinite){
            nonFiniteFrequencies.replace(u, true);
        }

        //Every frequency writes at its own index: no critical sections,
        //no permutations.
        allClouds->replace(u, cloud);
    }

    //Reported after the parallel loop, where throwing is safe again.
    for (qint32 u = 0; u < frequencyCount; u++){
        if (!parserErrors.at(u).isEmpty()){
            const std::string message = parserErrors.at(u).toStdString();
            qDeleteAll(*allClouds);
            delete allClouds;

            throw qftbx::InvalidInput(
                    "The plant expression could not be evaluated at "
                    + std::to_string(omega->at(u)) + " rad/s: " + message);
        }
    }

    //Reported after the parallel loop, naming every affected frequency.
    QStringList affected;
    for (qint32 u = 0; u < frequencyCount; u++){
        if (nonFiniteFrequencies.at(u)){
            affected.append(QString::number(omega->at(u)));
        }
    }

    if (!affected.isEmpty()){
        qDeleteAll(*allClouds);
        delete allClouds;

        throw qftbx::InvalidInput(
                "The plant has infinite magnitude at the design frequencies "
                + affected.join(QStringLiteral(", ")).toStdString()
                + " rad/s: an undamped resonance inside the uncertainty. Its "
                  "template cannot be bounded or contoured. Add light damping "
                  "to the resonant poles (the usual answer for the ACC'90 "
                  "benchmark) or move those frequencies out of the set.");
    }

    return allClouds;
}

QVector <qreal> * TemplateEngine::omega(){
    return m_frequencies;
}

QVector <qreal> * TemplateEngine::epsilon(){
    return m_epsilon;
}

//The contours built so far, which nobody owns yet: the facade takes them only
//when the computation returns, so a throw in the middle would leak both the
//row and every contour already in it.
void TemplateEngine::discardContours(){
    if (m_contours == nullptr){
        return;
    }

    qDeleteAll(*m_contours);
    delete m_contours;
    m_contours = nullptr;
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
            discardContours();

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

    //Per-frequency diagnosis of a failure (nothing may be thrown from
    //inside the parallel region), and of the frequencies whose faithful walk
    //did not close.
    QVector <bool> failed (digitCount, false);
    QVector <bool> relaxedFrequencies (digitCount, false);

#ifdef OpenMP_AVAILABLE
#pragma omp parallel for
#endif
    for (qint32 i = 0; i < digitCount; i++){

        bool fellBack = false;
        QVector <complex <qreal> > * cont = epsilonHull(m_clouds->at(i), m_epsilon->at(i), &fellBack);

        if (fellBack){
            relaxedFrequencies.replace(i, true);
        }

        if (cont == NULL){
            cont = new QVector <complex <qreal> >();
            failed.replace(i, true);

#ifdef OpenMP_AVAILABLE
#pragma omp critical
#endif
            {
                succeeded = false;
            }
        }

        m_contours->replace(i, cont);
    }

    //Reported HERE, once and outside the parallel region, and naming the
    //frequencies: from inside the loop it raced on the message handler and
    //produced N identical lines that said nothing about which frequency fell
    //back.
    QStringList relaxed;
    for (qint32 i = 0; i < digitCount; i++){
        if (relaxedFrequencies.at(i) && m_frequencies != nullptr && i < m_frequencies->size()){
            relaxed.append(QString::number(m_frequencies->at(i)));
        }
    }

    if (!relaxed.isEmpty()){
        qWarning("epsilonHull: the faithful walk did not close at w = %s rad/s "
                 "(epsilon-hull limitation on clustered templates); the relaxed "
                 "historical walk was used there, whose coverage is still <= epsilon.",
                 qUtf8Printable(relaxed.join(QStringLiteral(", "))));
    }

    if (!succeeded){
        //The message names the frequencies and their largest magnitude: a
        //resonance inside the uncertainty makes the cloud span astronomical
        //magnitudes (finite ones when the exact singular frequency is not
        //representable), and no epsilon walks a cloud like that. Without the
        //figure the user only saw "could not compute".
        QStringList detail;

        for (qint32 i = 0; i < digitCount; i++){
            if (!failed.at(i)){
                continue;
            }

            qreal largest = 0;
            foreach (const complex<qreal> & value, *m_clouds->at(i)){
                largest = std::max(largest, std::abs(value));
            }

            detail.append(QStringLiteral("%1 rad/s (largest |P| = %2)")
                              .arg(m_frequencies != nullptr && i < m_frequencies->size()
                                       ? QString::number(m_frequencies->at(i))
                                       : QString::number(i))
                              .arg(largest, 0, 'g', 3));
        }

        discardContours();

        throw qftbx::ComputationError(
                "Could not compute the template contour at "
                + detail.join(QStringLiteral("; ")).toStdString()
                + ". A cloud spanning extreme magnitudes has no epsilon-hull: "
                  "check for a resonance inside the plant uncertainty and damp "
                  "it lightly if so.");
    }

    return true;
}


//Faithful port of EPSHULL.M (epsh2, Montoya 1998; the algorithm defined in
//Nordin 1993). Deliberate divergence: with no initial candidate it returns
//NULL instead of an empty contour (the caller treats it as an error).
QVector <complex <qreal> > * TemplateEngine::epsilonHull(QVector<complex<qreal> > *temp, qreal epsilon,
                                                         bool * fellBack){

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
            //Recorded, not warned: see the declaration. The caller names
            //the frequencies once the loop is over.
            if (fellBack != nullptr){
                *fellBack = true;
            }

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
