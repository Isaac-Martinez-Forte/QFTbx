// The battery of published problems (examples/) is only worth having if every
// file in it still opens with all seven phases in it and a controller that
// meets every specification. This walks them all: whatever the format or the
// algorithms do from now on, a problem that stops loading or stops meeting its
// specifications says so here, and not to whoever downloads the repository.

#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

#include "src/app/project_controller.h"
#include "src/core/loopshaping/common/specification_checker.h"
#include "src/core/pipeline/pipeline_step.h"
#include "src/core/specifications/specification_record.h"

namespace {

std::vector<std::string> publishedProblems()
{
    std::vector<std::string> files;
    const std::filesystem::path directory(QFTBX_EXAMPLES_DIR);
    if (!std::filesystem::is_directory(directory)) {
        return files;
    }
    for (const auto & entry : std::filesystem::directory_iterator(directory)) {
        if (entry.path().extension() == ".qft") {
            files.push_back(entry.path().string());
        }
    }
    std::sort(files.begin(), files.end());
    return files;
}

} // namespace

TEST(PublishedProblems, EveryOneOpensWholeAndMeetsItsSpecifications)
{
    const std::vector<std::string> files = publishedProblems();
    if (files.empty()) {
        GTEST_SKIP() << "no published problems under " << QFTBX_EXAMPLES_DIR;
    }

    for (const std::string & file : files) {
        const std::string name = std::filesystem::path(file).stem().string();

        qftbx::ProjectController project;
        const qftbx::StepSet loaded = project.load(file);

        for (const qftbx::Step step : {qftbx::Step::Plant, qftbx::Step::Frequencies,
                                        qftbx::Step::Specifications, qftbx::Step::Templates,
                                        qftbx::Step::Boundaries, qftbx::Step::Controller,
                                        qftbx::Step::LoopShaping}) {
            EXPECT_TRUE(loaded.has(step)) << name << " lacks phase " << int(step);
        }

        ASSERT_NE(project.loopShapingResult(), nullptr) << name;
        qftbx::LtiSystem * controller = project.loopShapingResult()->controller();
        ASSERT_NE(controller, nullptr) << name << " carries no controller";

        EXPECT_TRUE(project.loopShapingResult()->check().has_value())
            << name << " was written without the verifier's verdict";

        const qftbx::SpecificationCheck check = qftbx::checkAgainstSpecifications(
            *controller, *project.plant(), *project.omega()->values(),
            project.templates(), qftbx::toSpecificationSet(*project.specifications()));

        EXPECT_TRUE(check.satisfied())
            << name << " exceeds a specification by " << check.worstExcessDb << " dB";
        if (project.loopShapingResult()->check().has_value()) {
            EXPECT_NEAR(check.worstExcessDb, project.loopShapingResult()->check()->worstExcessDb, 1e-6)
                << name << ": the verdict in the file is not the verdict of the file";
        }
    }
}
