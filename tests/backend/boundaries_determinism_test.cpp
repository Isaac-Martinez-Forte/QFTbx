/**
 * @file
 * @brief The boundaries and their union are bit-exact across thread counts.
 *
 * The sheets, the traces and the union run one OpenMP iteration per design
 * frequency, and a write that strays out of its frequency would change the
 * numbers rather than crash. The boundaries of `fopdt.qft`, nine
 * frequencies open and closed, on the grid the project stores, must be
 * identical with one thread and with six: the traces, the union, its phase
 * buckets and the open and upper flags. The process-wide thread count is
 * restored afterwards.
 */

#include <gtest/gtest.h>

#include <optional>
#include <string>

#ifdef OpenMP_AVAILABLE
#include <omp.h>
#endif

#include "src/core/boundaries/boundary_data.h"
#include "src/core/boundaries/boundary_engine.h"
#include "src/core/frequencies/omega.h"
#include "src/persistence/project_reader.h"

using namespace qftbx;

namespace {

std::optional<BoundaryData> boundaries(int threads)
{
#ifdef OpenMP_AVAILABLE
    omp_set_num_threads(threads);
#else
    (void) threads;
#endif

    ProjectReader parser;
    parser.load(std::string(QFTBX_EXAMPLES_DIR "/fopdt.qft"));
    const BoundaryData * stored = parser.boundaries();
    if (stored == nullptr || parser.plant() == nullptr) {
        return std::nullopt;
    }

    BoundaryEngine engine;
    engine.compute(parser.omega()->values(), parser.plant(), parser.contour(), true, parser.specifications(),
                   stored->phaseRange(), stored->phaseCount(), stored->magnitudeRange(), stored->magnitudeCount(),
                   -1.0);
    return engine.boundaryData();
}

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

TEST(BoundariesDeterminism, TheBoundariesAndTheirUnionAreBitExactWhateverTheThreadCount)
{
    ThreadCount restore;

    const std::optional<BoundaryData> single = boundaries(1);
    ASSERT_TRUE(single.has_value()) << "the example has no plant or no boundaries";
    ASSERT_FALSE(single->unionBoundaries().empty());

    const std::optional<BoundaryData> many = boundaries(6);
    ASSERT_TRUE(many.has_value());

    std::size_t open = 0;
    for (const bool flag : single->openFlags()) {
        open += flag ? 1 : 0;
    }
    EXPECT_GT(open, 0u) << "the example should have open frequencies";
    EXPECT_LT(open, single->openFlags().size()) << "the example should have closed frequencies";

    EXPECT_EQ(single->boundaries(), many->boundaries()) << "the traces";
    EXPECT_EQ(single->unionBoundaries(), many->unionBoundaries()) << "the union";
    EXPECT_EQ(single->unionBuckets(), many->unionBuckets()) << "the phase buckets of the union";
    EXPECT_EQ(single->openFlags(), many->openFlags());
    EXPECT_EQ(single->upperFlags(), many->upperFlags());
}

}
