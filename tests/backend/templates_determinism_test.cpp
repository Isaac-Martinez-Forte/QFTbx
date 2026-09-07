// The template sweep runs one OpenMP iteration per design frequency, and the
// epsilon hull another one per frequency. A data race in either would not
// necessarily crash: it would quietly change the numbers, which for a toolbox
// whose whole claim is a rigorous enclosure is the worst kind of failure -
// and it is the shape of the intermittent golden failure this suite saw once
// and could not reproduce.
//
// What is pinned here is the property that matters: the sweep is BIT-EXACT
// however many threads run it, and bit-exact when repeated. That covers both
// a race that reorders writes and one that loses them, without needing a
// thread sanitiser (there is no gcc libtsan on this machine).

#include <gtest/gtest.h>

#include <string>

#include <complex>
#include <vector>


#ifdef OpenMP_AVAILABLE
#include <omp.h>
#endif

#include "src/core/frequencies/omega.h"
#include "src/core/math/sequence_vectors.h"
#include "src/core/math/sequences.h"
#include "src/core/system/lti_system.h"
#include "src/core/templates/template_engine.h"
#include "src/persistence/project_reader.h"

using namespace qftbx;

namespace {

//Every computed number of one run, in order: the clouds first, then the
//contours. Compared with ==, so any changed bit shows up.
using Numbers = std::vector<double>;

void appendAll(const qftbx::CloudSet & sets, Numbers & out)
{
    for (const qftbx::ComplexCloud & set : sets) {
        out.push_back(static_cast<double>(set.size()));
        for (const std::complex<double> & value : set) {
            out.push_back(value.real());
            out.push_back(value.imag());
        }
    }
}

//One full sweep of planta2.qft over the 10x10 grid of the golden fixture,
//with the requested thread count.
Numbers sweep(int threads)
{
#ifdef OpenMP_AVAILABLE
    //Process-wide: restored by the caller so the rest of the binary keeps
    //the thread count it was launched with.
    omp_set_num_threads(threads);
#else
    (void) threads;
#endif

    qftbx::ProjectReader parser;
    parser.load(std::string(QFTBX_TEST_DATA_DIR "/planta2.qft"));

    LtiSystem * plant = parser.plant();
    if (plant == nullptr) {
        return {};
    }

    qftbx::ParameterGrids grids;
    grids[plant->numerator()[0].name()] = qftbx::math::linspace(1.0, 10.0, 10);
    grids[plant->gain().name()] = qftbx::math::linspace(1.0, 10.0, 10);

    std::vector<double> frequencies(*parser.omega()->values());
    const std::vector<double> epsilon(frequencies.size(), 10.0);

    TemplateEngine engine;
    engine.setEpsilon(epsilon);
    engine.setGrids(grids);
    engine.compute(plant, &frequencies, false);

    Numbers numbers;
    appendAll(engine.clouds(), numbers);
    appendAll(engine.contours(), numbers);

    return numbers;
}

//RAII for the process-wide thread count: leaving it at 1 would quietly
//serialise every test that runs after these.
class ThreadCount
{
public:
#ifdef OpenMP_AVAILABLE
    ThreadCount() : m_entry(omp_get_max_threads()) {}
    ~ThreadCount() { omp_set_num_threads(m_entry); }
private:
    int m_entry;
#endif
};

TEST(TemplatesDeterminism, TheSweepIsBitExactWhateverTheThreadCount)
{
    ThreadCount restore;

    const Numbers single = sweep(1);
    ASSERT_FALSE(single.empty()) << "the sweep produced nothing";

    //Six is the ceiling this project builds and runs with.
    const Numbers many = sweep(6);

    ASSERT_EQ(single.size(), many.size())
        << "the thread count changed how many points came out";
    EXPECT_EQ(single, many)
        << "the sweep is not bit-exact across thread counts: a write in one "
           "of the two parallel regions is not confined to its own frequency";
}

TEST(TemplatesDeterminism, TheSweepRepeatsItselfBitExactly)
{
    ThreadCount restore;

    //Same thread count twice: catches a race whose outcome depends on
    //scheduling rather than on the number of workers.
    const Numbers first = sweep(6);
    ASSERT_FALSE(first.empty()) << "the sweep produced nothing";

    for (int repetition = 0; repetition < 3; repetition++) {
        const Numbers again = sweep(6);
        ASSERT_EQ(first.size(), again.size()) << "repetition " << repetition;
        EXPECT_EQ(first, again) << "the sweep is not reproducible, repetition "
                               << repetition;
    }
}

} // namespace
