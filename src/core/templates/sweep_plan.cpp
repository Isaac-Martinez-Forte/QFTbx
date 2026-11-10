/**
 * @file
 * @brief The plan of a sweep and its run on the processor.
 *
 * The plan is what the template engine has always done before evaluating a
 * plant, moved here unchanged so that the processor and the GPU sweep the
 * same family: on the processor the plants come out as they always have,
 * to the bit. The family's poles are found once per distinct denominator,
 * and a plant whose poles cannot be placed leaves the family's count
 * unknown rather than refusing it.
 */

#include "src/core/templates/sweep_plan.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <map>
#include <stdexcept>
#include <string>

#include "src/core/common/exception.h"
#include "src/core/common/text_tokens.h"
#include "src/core/math/polynomial.h"

using std::complex;

namespace qftbx {

SweepPlan planSweep(LtiSystem & plant, const std::vector<double> & frequencies, const ParameterGrids & sweepGrids,
                    bool borderSweep)
{
    SweepPlan plan;

    const auto gridFor = [&sweepGrids](const Parameter & a) -> const std::vector<double> & {
        const auto found = sweepGrids.find(a.name());
        if (found == sweepGrids.end()){
            throw qftbx::InvalidInput(QFTBX_TR("Core", "Missing sweep grid for the uncertain parameter '%1'.").arg(a.name()));
        }
        return found->second;
    };

    std::vector <std::string> names;

    std::vector <const std::vector<double> *> grids;

    plan.combinationCount = 1;

    auto collect = [&](Parameter & var){
        if (var.isUncertain() &&
                std::find(names.begin(), names.end(), var.name()) == names.end()){
            const std::vector<double> & grid = gridFor(var);
            names.push_back(var.name());
            grids.push_back(&grid);
            plan.combinationCount *= grid.size();
        }
    };

    for (Parameter & var : plant.numerator())
        collect(var);
    for (Parameter & var : plant.denominator())
        collect(var);
    collect(plant.gain());
    collect(plant.delay());

    for (const std::vector<double> * grid : grids){
        plan.grids.push_back(*grid);
    }

    const std::size_t digitCount = names.size();
    const std::size_t frequencyCount = frequencies.size();

    const std::optional<std::vector<std::complex<double>>> nominalPoles = plant.nominalPoles();
    const std::vector<double> nominalResonances = nominalPoles.has_value()
            ? qftbx::math::imaginaryAxisFrequencies(*nominalPoles) : std::vector<double>{};
    for (const double w : frequencies){
        const std::complex<double> nominal = plant.evaluate(w);
        bool onAPole = !std::isfinite(nominal.real()) || !std::isfinite(nominal.imag());
        for (const double r : nominalResonances){
            onAPole = onAPole || std::abs(w - r) <= 1e-9 * r;
        }
        if (onAPole){
            throw qftbx::InvalidInput(QFTBX_TR("Core", "The nominal plant has a pole on the imaginary axis at %1 rad/s, which is a design frequency: the nominal loop is infinite there and no boundary can be drawn for it. Take a frequency a little to either side, such as %2 or %3 rad/s.")
                    .arg(qftbx::text::number(w)).arg(qftbx::text::number(0.99 * w)).arg(qftbx::text::number(1.01 * w)));
        }
    }

    std::vector<std::array<double, 2>> & border = plan.border;
    if (borderSweep && digitCount == 2 && grids[0]->size() > 1 && grids[1]->size() > 1){
        std::vector<std::vector<double>> sorted{*grids[0], *grids[1]};
        std::sort(sorted[0].begin(), sorted[0].end());
        std::sort(sorted[1].begin(), sorted[1].end());
        if (sorted[0].front() < sorted[0].back() && sorted[1].front() < sorted[1].back()){
            const auto along = [](const std::vector<double> & g, double t) {
                const double x = t * static_cast<double>(g.size() - 1);
                const std::size_t k = std::min(static_cast<std::size_t>(x), g.size() - 2);
                return g[k] + (x - static_cast<double>(k)) * (g[k + 1] - g[k]);
            };
            const std::size_t perEdge = std::max<std::size_t>(2, plan.combinationCount / 4);
            border.reserve(4 * perEdge);
            for (std::size_t i = 0; i < perEdge; ++i){
                const double t = static_cast<double>(i) / static_cast<double>(perEdge);
                border.push_back({along(sorted[0], t), sorted[1].front()});
            }
            for (std::size_t i = 0; i < perEdge; ++i){
                const double t = static_cast<double>(i) / static_cast<double>(perEdge);
                border.push_back({sorted[0].back(), along(sorted[1], t)});
            }
            for (std::size_t i = 0; i < perEdge; ++i){
                const double t = static_cast<double>(i) / static_cast<double>(perEdge);
                border.push_back({along(sorted[0], 1.0 - t), sorted[1].back()});
            }
            for (std::size_t i = 0; i < perEdge; ++i){
                const double t = static_cast<double>(i) / static_cast<double>(perEdge);
                border.push_back({sorted[0].front(), along(sorted[1], 1.0 - t)});
            }
            plan.combinationCount = border.size();
        }
    }

    const auto slotOf = [&](Parameter & parameter) -> std::int32_t {
        if (!parameter.isUncertain()) {
            return -1;
        }
        const auto found = std::find(names.begin(), names.end(), parameter.name());
        return found == names.end()
                ? -1
                : static_cast<std::int32_t>(std::distance(names.begin(), found));
    };

    std::vector<std::int32_t> & numeratorSlot = plan.numeratorSlot;
    std::vector<std::int32_t> & denominatorSlot = plan.denominatorSlot;
    std::vector<double> & numeratorNominal = plan.numeratorNominal;
    std::vector<double> & denominatorNominal = plan.denominatorNominal;

    for (Parameter & parameter : plant.numerator()){
        numeratorSlot.push_back(slotOf(parameter));
        numeratorNominal.push_back(parameter.nominal());
    }
    for (Parameter & parameter : plant.denominator()){
        denominatorSlot.push_back(slotOf(parameter));
        denominatorNominal.push_back(parameter.nominal());
    }

    plan.gainSlot = slotOf(plant.gain());
    plan.delaySlot = slotOf(plant.delay());
    plan.gainNominal = plant.gain().nominal();
    plan.delayNominal = plant.delay().nominal();

    std::map<std::vector<double>, std::vector<double>> axisByDenominator;
    {
        std::vector<double> numeratorValues = numeratorNominal;
        std::vector<double> denominatorValues = denominatorNominal;
        std::vector<double> digit(digitCount);
        std::map<std::vector<double>, int> countByDenominator;
        std::optional<int> familyCount;
        std::vector<double> firstDenominator;
        bool placeable = true;

        for (std::size_t i = 0; i < plan.combinationCount && placeable; i++){
            if (plan.borderSweepApplied()){
                digit[0] = border[i][0];
                digit[1] = border[i][1];
            } else {
                std::size_t rest = i;
                for (std::size_t j = 0; j < digitCount; j++){
                    const std::vector<double> & grid = *grids.at(j);
                    digit[j] = grid[rest % grid.size()];
                    rest /= grid.size();
                }
            }
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

            const auto known = countByDenominator.find(denominatorValues);
            int count = 0;
            if (known != countByDenominator.end()){
                count = known->second;
            } else {
                const std::optional<std::vector<std::complex<double>>> poles =
                        plant.polesAt(numeratorValues, denominatorValues);
                if (!poles.has_value()){
                    placeable = false;
                    break;
                }
                count = qftbx::math::rightHalfPlaneCount(*poles);
                countByDenominator[denominatorValues] = count;
                axisByDenominator[denominatorValues] = qftbx::math::imaginaryAxisFrequencies(*poles);
            }

            if (!familyCount.has_value()){
                familyCount = count;
                firstDenominator = denominatorValues;
            } else if (count != *familyCount){
                const auto listed = [](const std::vector<double> & values) {
                    std::string text;
                    for (std::size_t k = 0; k < values.size(); k++){
                        text += (k ? ", " : "") + qftbx::text::number(values[k]);
                    }
                    return text;
                };
                throw qftbx::InvalidInput(QFTBX_TR("Core", "The plant family changes its number of right half-plane poles: %1 with the denominator values (%2) and %3 with (%4). The stability of the nominal loop only carries to a family whose members all have the same number; split the uncertainty at the crossing.")
                                          .arg(*familyCount).arg(listed(firstDenominator)).arg(count).arg(listed(denominatorValues)));
            }
        }

        if (placeable){
            plan.familyRhpPoles = familyCount;
        }
    }

    std::vector<std::array<double, 2>> bands;
    for (const auto & [denominator, frequencies] : axisByDenominator){
        for (std::size_t j = 0; j < frequencies.size(); j++){
            if (bands.size() <= j){
                bands.push_back({frequencies[j], frequencies[j]});
            }
            bands[j][0] = std::min(bands[j][0], frequencies[j]);
            bands[j][1] = std::max(bands[j][1], frequencies[j]);
        }
    }
    plan.resonanceBands.assign(frequencyCount, std::nullopt);
    for (std::size_t u = 0; u < frequencyCount; u++){
        for (const std::array<double, 2> & band : bands){
            if (band[0] < frequencies.at(u) && frequencies.at(u) < band[1]){
                plan.resonanceBands[u] = band;
            }
        }
    }

    return plan;
}

CloudSet sweepOnProcessor(LtiSystem & plant, const std::vector<double> & frequencies, const SweepPlan & plan,
                          const CancellationToken * cancellation)
{
    const std::size_t digitCount = plan.grids.size();
    const std::size_t frequencyCount = frequencies.size();
    const std::vector<std::array<double, 2>> & border = plan.border;
    const std::vector<std::int32_t> & numeratorSlot = plan.numeratorSlot;
    const std::vector<std::int32_t> & denominatorSlot = plan.denominatorSlot;
    const std::vector<double> & numeratorNominal = plan.numeratorNominal;
    const std::vector<double> & denominatorNominal = plan.denominatorNominal;
    const std::int32_t gainSlot = plan.gainSlot;
    const std::int32_t delaySlot = plan.delaySlot;
    const double gainNominal = plan.gainNominal;
    const double delayNominal = plan.delayNominal;

    CloudSet allClouds (frequencyCount);

    std::vector<char> nonFiniteFrequencies (frequencyCount, 0);
    std::vector <std::string> parserErrors (frequencyCount);

#ifdef OpenMP_AVAILABLE
#pragma omp parallel for
#endif
    for (std::size_t u = 0; u < frequencyCount; u++){

        if (cancellationAsked(cancellation)) {
            continue;
        }

        const double w = frequencies.at(u);

        std::vector<double> numeratorValues = numeratorNominal;
        std::vector<double> denominatorValues = denominatorNominal;

        std::vector <double> digit (digitCount);
        for (std::size_t j = 0; j < digitCount; j++){
            digit[j] = plan.grids.at(j)[0];
        }

        ComplexCloud cloud;
        cloud.reserve(plan.combinationCount);

        std::vector <std::size_t> counter (digitCount + 1, 0);

        bool nonFinite = false;

        for (std::size_t i = 0; i < plan.combinationCount; i++){

            complex<double> value;

            if (plan.borderSweepApplied()){
                digit[0] = border[i][0];
                digit[1] = border[i][1];
            }

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

            try {
                value = plant.valueAt(w, numeratorValues, denominatorValues,
                                       gainValue, delayValue);
            } catch (const std::invalid_argument & error) {
                parserErrors[u] = std::string(error.what());
                break;
            } catch (const qftbx::Exception & error) {
                parserErrors[u] = std::string(error.what());
                break;
            }

            nonFinite = nonFinite || !std::isfinite(value.real()) || !std::isfinite(value.imag());
            cloud.push_back(value);

            if (plan.borderSweepApplied()){
                continue;
            }
            counter[0]++;
            for (std::size_t j = 0; j < digitCount; j++){
                if (counter.at(j) >= plan.grids.at(j).size()){
                    counter[j] = 0;
                    counter[j+1]++;
                    digit[j] = plan.grids.at(j)[0];
                }else {
                    digit[j] = plan.grids.at(j)[counter.at(j)];
                    break;
                }
            }
        }

        if (nonFinite){
            nonFiniteFrequencies[u] = true;
        }

        allClouds[u] = std::move(cloud);
    }

    for (std::size_t u = 0; u < frequencyCount; u++){
        if (!parserErrors.at(u).empty()){
            const std::string message = parserErrors.at(u);
            throw qftbx::InvalidInput(QFTBX_TR("Core", "The plant expression could not be evaluated at %1 rad/s: %2").arg(frequencies.at(u)).arg(message));
        }
    }

    refuseNonFinite(frequencies, nonFiniteFrequencies);

    return allClouds;
}

void refuseNonFinite(const std::vector<double> & frequencies, const std::vector<char> & nonFinite)
{
    std::vector<std::string> affected;
    for (std::size_t u = 0; u < frequencies.size() && u < nonFinite.size(); u++){
        if (nonFinite.at(u)){
            affected.push_back(qftbx::text::number(frequencies.at(u)));
        }
    }

    if (!affected.empty()){
        throw qftbx::InvalidInput(QFTBX_TR("Core", "Some plants of the sweep have a pole on the imaginary axis exactly at the design frequencies %1 rad/s: an undamped resonance inside the uncertainty, where the template is unbounded and cannot be contoured. Move those frequencies a little, or sweep the parameters so that no plant falls exactly on them.")
                .arg(qftbx::text::join(affected, ", ")));
    }
}


}
