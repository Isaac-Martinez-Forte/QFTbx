/**
 * @file
 * @brief The sheets of the boundaries, computed on the card.
 *
 * sheetsOnCard() gives, for every frequency and every point of the Nichols
 * grid, the closed-loop magnitudes the frequency's specifications bound, in
 * decibels, worst case over the template and with the guard near the
 * singular locus when it is on, as BoundaryEngine computes them: only the
 * sheets the frequency reads, the others left empty. The loop value of a
 * cell is the processor's to the bit, from the row's magnitude and the
 * column's cosine and sine; the rest agrees to rounding.
 *
 * Every extreme over the template is taken in two passes (sheet_bounds.cuh): a
 * pass in single precision with a bound on its error leaves the few plants
 * or segments that can hold the extreme, and the double one decides among
 * them. It uploads the templates, unless they are whole clouds the card
 * already holds (resident), and computes as many frequencies at a time
 * as half the card's free memory holds; a failure on the card throws
 * CudaError, and a cancelled computation stops between and inside kernels
 * and answers cancelled. Plain C++, so the glue includes it without the
 * toolkit. SheetsOnProcessor runs the threads' own code on the processor,
 * one cell at a time after preparing the frequency once, which is how the
 * tests check it without a card; it gives all five values of a cell.
 */

#ifndef QFTBX_GPU_SHEET_SWEEP_H
#define QFTBX_GPU_SHEET_SWEEP_H

#include <array>
#include <complex>
#include <cstddef>
#include <memory>
#include <vector>

#include "src/core/boundaries/boundary_types.h"
#include "src/core/pipeline/cancellation.h"

namespace qftbx::gpu {

class DeviceClouds;

struct SheetProblem
{
    const std::vector<std::vector<std::complex<double>>> * templates = nullptr;
    std::vector<std::complex<double>> nominal;
    std::vector<std::array<bool, 5>> sheets;
    std::vector<double> rowMagnitudes;
    std::vector<double> cosines;
    std::vector<double> sines;
    bool contours = true;
    bool guard = true;
    const DeviceClouds * resident = nullptr;
};

struct SheetsOnCard
{
    std::vector<BoundarySheets> sheets;
    bool cancelled = false;
    double kernelMilliseconds = 0.0;
    double copyMilliseconds = 0.0;
    std::size_t batches = 0;
};

SheetsOnCard sheetsOnCard(const SheetProblem & problem, const CancellationToken * cancellation);

class SheetsOnProcessor
{
public:
    SheetsOnProcessor(const SheetProblem & problem, std::size_t frequency);
    ~SheetsOnProcessor();
    SheetsOnProcessor(const SheetsOnProcessor &) = delete;
    SheetsOnProcessor & operator=(const SheetsOnProcessor &) = delete;

    std::array<double, 5> cell(std::size_t row, std::size_t column) const;
    const std::vector<double> & spacings() const;

private:
    struct State;

    const SheetProblem & m_problem;
    std::size_t m_frequency = 0;
    std::unique_ptr<State> m_state;
};

}

#endif
