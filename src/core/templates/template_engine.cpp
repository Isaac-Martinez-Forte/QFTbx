#include <chrono>
#include <string>
#include <vector>
#include <cstdint>
#include "template_engine.h"
#include "mpParser.h"


#include "src/core/text_tokens.h"
#include "src/core/exception.h"
#include "src/core/math/parser_warmup.h"

#include <algorithm>
#include <iostream>

using namespace std;

#ifdef CUDA_AVAILABLE
//GPU epsilon-hull (relaxed-walk semantics, see the header).
#include "src/core/gpu/template_contour_cuda.h"
#endif

namespace qftbx {


void TemplateEngine::setGrids(ParameterGrids grids){
    m_grids = std::move(grids);
}

void TemplateEngine::setEpsilon(std::vector<double> epsilon){
    m_epsilon = std::move(epsilon);
}

void TemplateEngine::setClouds(CloudSet templates){
    m_clouds = std::move(templates);
}

bool TemplateEngine::compute(LtiSystem *plant, std::vector<double> *omega, bool cuda){
    m_useCuda = cuda;

    const auto timer = std::chrono::steady_clock::now();
    m_frequencies = omega;

    m_clouds = computeClouds(plant, omega);

    std::cout << "Templates: " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - timer).count() << " milliseconds\n";


    if (m_clouds.empty()){
        throw qftbx::ComputationError("Could not compute the templates.");
    }

    const auto timer2 = std::chrono::steady_clock::now();

    //No try/catch and no freeing: m_clouds owns its data, so a throw from
    //here unwinds and the destructor takes care of it. That whole hand-written
    //rescue was only needed because the clouds were a raw pointer nobody
    //owned.
    const bool result = computeContourSet(cuda);

    std::cout << "Contours: " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - timer2).count() << " milliseconds\n";

    return result;

}

bool TemplateEngine::computeContours(std::vector<double> epsilon){

    if (m_clouds.empty()){
        throw qftbx::InvalidInput("There are no templates to compute contours from.");
    }
    if (epsilon.size() < m_clouds.size()){
        throw qftbx::InvalidInput("Missing epsilon values for the template contours.");
    }

    m_epsilon = std::move(epsilon);
    const auto timer = std::chrono::steady_clock::now();

    bool result = computeContourSet(m_useCuda);

    std::cout << "Contours: " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - timer).count() << " milliseconds\n";

    return result;
}

const CloudSet & TemplateEngine::clouds() const{
    return m_clouds;
}

const CloudSet & TemplateEngine::contours() const{
    return m_contours;
}

const std::vector<double> & TemplateEngine::gridFor(const Parameter & a){

    //Keyed by NAME: pointer identity went stale on every clone() or
    //project reload.
    const auto found = m_grids.find(a.name());

    if (found == m_grids.end()){
        throw qftbx::InvalidInput("Missing sweep grid for the uncertain parameter '"
                                  + a.name() + "'.");
    }

    return found->second;
}

CloudSet TemplateEngine::computeClouds(LtiSystem *plant, std::vector<double> *omega){

    //Collect the uncertain parameters (the first of each name) and their
    //grids, in numerator, denominator, gain, delay order. The index in
    //'names' is the odometer digit (0 is the fastest), as historically.
    std::vector <std::string> names;

    //Pointers INTO the map the engine owns: stable for the whole sweep,
    //because nothing modifies m_grids while it runs.
    std::vector <const std::vector<double> *> grids;

    m_combinationCount = 1;

    auto collect = [&](Parameter & var){
        if (var.isUncertain() &&
                std::find(names.begin(), names.end(), var.name()) == names.end()){
            const std::vector<double> & grid = gridFor(var);
            names.push_back(var.name());
            grids.push_back(&grid);
            m_combinationCount *= grid.size();
        }
    };

    for (Parameter & var : plant->numerator())
        collect(var);
    for (Parameter & var : plant->denominator())
        collect(var);
    collect(plant->gain());
    collect(plant->delay());

    const std::size_t digitCount = names.size();
    const std::size_t frequencyCount = omega->size();

    //Which odometer digit drives each coefficient, and the nominals of the
    //ones no digit drives. Built ONCE and sequentially: Parameter::nominal()
    //can evaluate a reparametrisation, and a coefficient's plan does not
    //change with the frequency or the combination. A name appearing twice
    //shares its digit, which is what makes it one variable.
    const auto slotOf = [&](Parameter & parameter) -> std::int32_t {
        if (!parameter.isUncertain()) {
            return -1;
        }
        const auto found = std::find(names.begin(), names.end(), parameter.name());
        return found == names.end()
                ? -1
                : static_cast<std::int32_t>(std::distance(names.begin(), found));
    };

    std::vector<std::int32_t> numeratorSlot, denominatorSlot;
    std::vector<double> numeratorNominal, denominatorNominal;

    for (Parameter & parameter : plant->numerator()){
        numeratorSlot.push_back(slotOf(parameter));
        numeratorNominal.push_back(parameter.nominal());
    }
    for (Parameter & parameter : plant->denominator()){
        denominatorSlot.push_back(slotOf(parameter));
        denominatorNominal.push_back(parameter.nominal());
    }

    const std::int32_t gainSlot = slotOf(plant->gain());
    const std::int32_t delaySlot = slotOf(plant->delay());
    const double gainNominal = plant->gain().nominal();
    const double delayNominal = plant->delay().nominal();

    CloudSet allClouds (frequencyCount);

    //One flag and one error slot per frequency, filled inside the parallel
    //loop below (nothing may be thrown from within it).
    //A byte per flag, not std::vector<bool>: that one packs its elements
    //into bits, and the parallel iterations below writing neighbouring
    //flags would race on the same byte.
    std::vector<char> nonFiniteFrequencies (frequencyCount, 0);
    std::vector <std::string> parserErrors (frequencyCount);

    //Before the threads exist: muParserX's package singletons are built
    //lazily and unsynchronised, and the loop below constructs one parser per
    //frequency. See warmUpExpressionParser().
    qftbx::math::warmUpExpressionParser();

#ifdef OpenMP_AVAILABLE
#pragma omp parallel for
#endif
    for (std::size_t u = 0; u < frequencyCount; u++){

        //No parser and no expression TEXT any more: the transfer function is
        //computed directly in complex arithmetic by valueAt(). The text route
        //cost a parse per frequency, constructed a parser inside this loop -
        //racing on muParserX's unsynchronised package singletons - and, being
        //written with qftbx::text::number(), evaluated the plant with its
        //constant coefficients AND ITS FREQUENCY rounded to six significant
        //digits. The swept coefficients were exact only because they were
        //bound as variables.
        const double w = omega->at(u);

        std::vector<double> numeratorValues = numeratorNominal;
        std::vector<double> denominatorValues = denominatorNominal;

        std::vector <double> digit (digitCount);
        for (std::size_t j = 0; j < digitCount; j++){
            digit[j] = (*grids.at(j))[0];
        }

        ComplexCloud cloud;
        cloud.reserve(m_combinationCount);

        std::vector <std::size_t> counter (digitCount + 1, 0);

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

        for (std::size_t i = 0; i < m_combinationCount; i++){

            complex<double> value;

            for (std::size_t c = 0; c < numeratorSlot.size(); c++){
                if (numeratorSlot[c] >= 0){
                    numeratorValues[c] = digit[static_cast<std::size_t>(numeratorSlot[c])];
                }
            }
            for (std::size_t c = 0; c < denominatorSlot.size(); c++){
                if (denominatorSlot[c] >= 0){
                    denominatorValues[c] = digit[static_cast<std::size_t>(denominatorSlot[c])];
                }
            }

            const double gainValue = gainSlot >= 0 ? digit[static_cast<std::size_t>(gainSlot)] : gainNominal;
            const double delayValue = delaySlot >= 0 ? digit[static_cast<std::size_t>(delaySlot)] : delayNominal;

            //A free-form plant still evaluates an expression, so it can still
            //reject one (a name colliding with a muParserX constant, a
            //malformed plant). Nothing may be thrown from inside the OpenMP
            //region - that TERMINATES the process - so the message is kept
            //and rethrown after the loop.
            try {
                value = plant->valueAt(w, numeratorValues, denominatorValues,
                                       gainValue, delayValue);
            } catch (mup::ParserError & error) {
                parserErrors[u] = (error.GetMsg());
                break;
            } catch (const qftbx::Exception & error) {
                parserErrors[u] = std::string(error.what());
                break;
            }

            nonFinite = nonFinite || !std::isfinite(value.real()) || !std::isfinite(value.imag());
            cloud.push_back(value);

            counter[0]++;
            for (std::size_t j = 0; j < digitCount; j++){
                if (counter.at(j) >= grids.at(j)->size()){
                    counter[j] = 0;
                    counter[j+1]++;
                    digit[j] = (*grids.at(j))[0];
                }else {
                    digit[j] = (*grids.at(j))[counter.at(j)];
                    break;
                }
            }
        }

        if (nonFinite){
            nonFiniteFrequencies[u] = true;
        }

        //Every frequency writes at its own index: no critical sections,
        //no permutations.
        allClouds[u] = std::move(cloud);
    }

    //Reported after the parallel loop, where throwing is safe again.
    for (std::size_t u = 0; u < frequencyCount; u++){
        if (!parserErrors.at(u).empty()){
            const std::string message = parserErrors.at(u);
            throw qftbx::InvalidInput(
                    "The plant expression could not be evaluated at "
                    + qftbx::text::number(omega->at(u)) + " rad/s: " + message);
        }
    }

    //Reported after the parallel loop, naming every affected frequency.
    std::vector<std::string> affected;
    for (std::size_t u = 0; u < frequencyCount; u++){
        if (nonFiniteFrequencies.at(u)){
            affected.push_back(qftbx::text::number(omega->at(u)));
        }
    }

    if (!affected.empty()){
        throw qftbx::InvalidInput(
                "The plant has infinite magnitude at the design frequencies "
                + qftbx::text::join(affected, ", ")
                + " rad/s: an undamped resonance inside the uncertainty. Its "
                  "template cannot be bounded or contoured. Add light damping "
                  "to the resonant poles (the usual answer for the ACC'90 "
                  "benchmark) or move those frequencies out of the set.");
    }

    return allClouds;
}

std::vector <double> * TemplateEngine::omega(){
    return m_frequencies;
}

const std::vector <double> & TemplateEngine::epsilon() const{
    return m_epsilon;
}

bool TemplateEngine::computeContourSet([[maybe_unused]] bool cuda){

    bool succeeded = true;
    const std::size_t digitCount = m_clouds.size();

#ifdef CUDA_AVAILABLE
    if (cuda){
        //GPU path (relaxed-walk semantics: the parity reference is
        //epsilonHullRelaxed, not the faithful walk - see the header).
        m_contours.clear();

        for (std::size_t i = 0; i < digitCount; i++){

            const ComplexCloud hull = epsilonHullCuda(
                m_clouds[i],
                m_epsilon.at(i));

            if (hull.empty()){
                succeeded = false;
            }
            m_contours.push_back(ComplexCloud(hull.begin(), hull.end()));
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
    m_contours = CloudSet(digitCount);

    //Per-frequency diagnosis of a failure (nothing may be thrown from
    //inside the parallel region), and of the frequencies whose faithful walk
    //did not close.
    //Bytes, not std::vector<bool>: see computeClouds().
    std::vector<char> failed (digitCount, 0);
    std::vector<char> relaxedFrequencies (digitCount, 0);

#ifdef OpenMP_AVAILABLE
#pragma omp parallel for
#endif
    for (std::size_t i = 0; i < digitCount; i++){

        bool fellBack = false;
        ComplexCloud cont = epsilonHull(m_clouds[i],
                                        m_epsilon.at(i), &fellBack);

        if (fellBack){
            relaxedFrequencies[i] = true;
        }

        //Empty means the hull could not be built, the same signal the CUDA
        //path already used. A hull of a non-empty cloud always has points.
        if (cont.empty()){
            failed[i] = true;

#ifdef OpenMP_AVAILABLE
#pragma omp critical
#endif
            {
                succeeded = false;
            }
        }

        m_contours[i] = std::move(cont);
    }

    //Reported HERE, once and outside the parallel region, and naming the
    //frequencies: from inside the loop it raced on the message handler and
    //produced N identical lines that said nothing about which frequency fell
    //back.
    std::vector<std::string> relaxed;
    for (std::size_t i = 0; i < digitCount; i++){
        if (relaxedFrequencies.at(i) && m_frequencies != nullptr && i < m_frequencies->size()){
            relaxed.push_back(qftbx::text::number(m_frequencies->at(i)));
        }
    }

    if (!relaxed.empty()){
        std::cerr << "epsilonHull: the faithful walk did not close at w = "
                  << qftbx::text::join(relaxed, ", ")
                  << " rad/s (epsilon-hull limitation on clustered templates); "
                     "the relaxed historical walk was used there, whose coverage "
                     "is still <= epsilon." << std::endl;
    }

    if (!succeeded){
        //The message names the frequencies and their largest magnitude: a
        //resonance inside the uncertainty makes the cloud span astronomical
        //magnitudes (finite ones when the exact singular frequency is not
        //representable), and no epsilon walks a cloud like that. Without the
        //figure the user only saw "could not compute".
        std::vector<std::string> detail;

        for (std::size_t i = 0; i < digitCount; i++){
            if (!failed.at(i)){
                continue;
            }

            double largest = 0;
            for (const complex<double> & value : m_clouds[i]){
                largest = std::max(largest, std::abs(value));
            }

            const std::string where = m_frequencies != nullptr && i < m_frequencies->size()
                    ? qftbx::text::number(m_frequencies->at(i))
                    : std::to_string(i);

            detail.push_back(where + " rad/s (largest |P| = "
                             + qftbx::text::number(largest) + ")");
        }

        throw qftbx::ComputationError(
                "Could not compute the template contour at "
                + qftbx::text::join(detail, "; ")
                + ". A cloud spanning extreme magnitudes has no epsilon-hull: "
                  "check for a resonance inside the plant uncertainty and damp "
                  "it lightly if so.");
    }

    return true;
}


//Faithful port of EPSHULL.M (epsh2, Montoya 1998; the algorithm defined in
//Nordin 1993). Deliberate divergence: with no initial candidate it returns
//NULL instead of an empty contour (the caller treats it as an error).
ComplexCloud TemplateEngine::epsilonHull(const ComplexCloud & temp, double epsilon,
                                                         bool * fellBack){

    if (temp.empty()){
        return {};
    }

    //unique(cv): deduplicated and sorted with MATLAB's ordering for complex
    //values (modulus, then phase). This order also resolves the psi ties
    //exactly like the reference.
    ComplexCloud cv = temp;
    std::sort(cv.begin(), cv.end(),
              [](const complex<double> & a, const complex<double> & b){
                  const double absA = abs(a);
                  const double absB = abs(b);
                  if (absA != absB){
                      return absA < absB;
                  }
                  return arg(a) < arg(b);
              });
    cv.erase(std::unique(cv.begin(), cv.end()), cv.end());

    const std::size_t pointCount = cv.size();
    const std::size_t MAXP = 3 * pointCount;

    //First point: the largest real part (the rightmost one), as in
    //EPSHULL.M and in the PFC text. Ties: the first in unique order wins.
    std::int32_t b1 = 0;
    for (std::size_t i = 1; i < pointCount; i++){
        if (real(cv.at(i)) > real(cv.at(static_cast<std::size_t>(b1)))){
            b1 = i;
        }
    }

    std::int32_t b2 = findSecond(b1, cv, epsilon);

    if (b2 < 0)
        return {};

    std::vector <std::int32_t> walk;
    walk.push_back(b1);
    walk.push_back(b2);

    std::int32_t previousPoint = b1;
    std::int32_t currentPoint = b2;

    std::int32_t nextPoint = findNext(b1, b2, cv, epsilon);
    if (nextPoint < 0)
        return {};

    std::size_t counter = 2;

    //Stops when the walk returns to the initial (b1, b2) pair. Points may
    //repeat (out-and-back over template spikes): they are kept, like in
    //the MATLAB, as real geometric information of the contour.
    while (b1 != currentPoint || b2 != nextPoint){

        walk.push_back(nextPoint);
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

        nextPoint = findNext(previousPoint, currentPoint, cv, epsilon);

        if (nextPoint < 0){
            return {};
        }
    }

    ComplexCloud result;
    result.reserve(walk.size());

    for (const std::int32_t var : walk) {
        result.push_back(cv.at(static_cast<std::size_t>(var)));
    }

    return result;
}

ComplexCloud TemplateEngine::epsilonHullRelaxed(const ComplexCloud & temp, double epsilon){

    std::size_t pointCount = temp.size();
    std::size_t MAXP = 3 * pointCount;

    std::int32_t b1 = 0;
    double numDe = -numeric_limits<double>::infinity();

    for(std::size_t i = 0;i < pointCount ; i++){   //first point: largest imaginary part.
        if (imag(temp.at(i)) > numDe){
            b1 = i;
            numDe = imag(temp.at(i));
        }
    }

    std::int32_t b2 = findSecond(b1, temp, epsilon);

    if (b2 < 0)
        return {};

    std::vector <std::int32_t> walk;
    walk.push_back(b1);
    walk.push_back(b2);

    std::int32_t previousPoint = b1;
    std::int32_t currentPoint = b2;

    std::int32_t nextPoint = findNext(b1, b2, temp, epsilon, true);
    if (nextPoint < 0)
        return {};

    std::size_t counter = 2;

    while (b1 != currentPoint || b2 != nextPoint){

        walk.push_back(nextPoint);
        counter++;

        if (counter > MAXP)
            break;      //silent truncation: partial contour (historical behaviour).

        previousPoint = currentPoint;
        currentPoint = nextPoint;

        nextPoint = findNext(previousPoint, currentPoint, temp, epsilon, true);

        if (nextPoint < 0){
            return {};
        }
    }

    //Output deduplication (historical behaviour).
    std::vector <std::int32_t> uniqueIdx;
    for (std::int32_t idx : walk) {
        if (std::find(uniqueIdx.begin(), uniqueIdx.end(), idx) == uniqueIdx.end()){
            uniqueIdx.push_back(idx);
        }
    }

    ComplexCloud result;
    result.reserve(uniqueIdx.size());

    for (const std::int32_t idx : uniqueIdx) {
        result.push_back(temp.at(static_cast<std::size_t>(idx)));
    }

    return result;
}

std::int32_t TemplateEngine::findSecond(std::int32_t b1, const ComplexCloud & cv, double epsilon){

    double dist = 0;
    complex <double> firstPoint = cv.at(static_cast<std::size_t>(b1));

    double fmin = numeric_limits<double>::infinity();
    std::int32_t pmin = -1;
    double dmax = 0;

    complex <double> candidate;

    double fas = 0;

    for (std::size_t i = 0; i < cv.size(); ++i){    //every point of the cloud.

        candidate = cv.at(i);
        dist = abs(firstPoint - candidate); //distance to the starting point.

        if (dist > 0 && dist <= epsilon){    //candidates within epsilon.

            fas = arg (candidate - firstPoint); //phase of the difference

            if (fas < 0)        //brought into [0, 2*PI)
                fas += 2 * M_PI;

            //subtract from the phase the arccosine of distance over epsilon.
            fas -= std::acos(dist / epsilon);

            if (fas < fmin){   //keep the minimum phase
                fmin = fas;
                pmin = i;
                dmax = dist;

            }else if (fas == fmin && dist > dmax){ //on a tie, keep the farthest.
                pmin = i;
                dmax = dist;
            }
        }
    }

    return pmin;
}

std::int32_t TemplateEngine::findNext(std::int32_t previousPoint, std::int32_t currentPoint,
                                  const ComplexCloud & cv, double epsilon,
                                  bool excludePrevious){

    complex <double> current = cv.at(static_cast<std::size_t>(currentPoint));
    complex <double> previous = cv.at(static_cast<std::size_t>(previousPoint));

    double aco2 = std::acos(abs(previous-current) / epsilon);

    double phase = 0;

    double aco1 = 0;
    double dmax = 0;

    double psi = 0;


    double psiMin = numeric_limits<double>::infinity();
    std::int32_t bestIndex = -1;

    complex <double> candidate;
    double distance;

    for (std::size_t i = 0; i < cv.size(); ++i){

        candidate = cv.at(i);
        distance = abs(candidate - current); //distance to the current point.


        //Candidates: every point at a distance in (0, epsilon] of the
        //current one, INCLUDING the previous one - its exclusion is
        //deliberately commented out in EPSHULL.M. It comes in through the
        //phase == 0 case, and it is what lets the walk go back along the
        //spikes and the thin branches of a template. The relaxed
        //historical variant excludes it.
        if (distance > 0 && distance <= epsilon &&
                !(excludePrevious && (candidate == previous || candidate == current))){

            //--------------------------------------------

            //phase between the two points, normalised by the incoming leg.
            phase = arg((candidate - current) / (previous - current));

            if(phase < 0) //brought into [0, 2*PI)
                phase +=  2 * M_PI;

            //------------------------------------------------------------

            aco1 = std::acos(distance / epsilon );  //half-angle subtended by the step

            //------------------------------------------------------------

            if(phase == 0){  //psi has three cases, as in EPSHULL.M
                psi =  2 * M_PI - aco1 - aco2;
            }else if (phase > 0 && phase < aco2){
                psi = phase + aco1- aco2;
            }else{
                psi = phase - aco1 - aco2;
            }

            if (psi < 0)                   //brought into [0, 2*PI)
                psi +=  2 * M_PI;

            //------------------------------------------------------------

            if (psi < psiMin) {       //keep the minimum psi, and on a tie
                psiMin = psi;         //the farthest candidate: ties are
                bestIndex = i;        //possible and the longer step wins.
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
