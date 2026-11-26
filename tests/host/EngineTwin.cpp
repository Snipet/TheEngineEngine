// tests/host/EngineTwin.cpp: EngineTwin.h.
#include "EngineTwin.h"

#include "tee/EnginePlayer.h"
#include "tee/test/Scenario.h"

namespace tee::test
{
    bool enginePlayerRender(const std::string& buildFile, const std::string& scenarioFile, double seconds, int block,
                            std::vector<float>& left, std::vector<float>& right, std::string& error)
    {
        // on the run's one compile (tee::test::compiled): the same program as a compile of its own, not compiled again
        auto player = EnginePlayer::create(compiled(std::filesystem::path(TEE_SOURCE_DIR) / "sim", {}, error), 48000.0,
                                           error);
        if (!player)
            return false;
        tee::EngineBuild build;
        std::string warnings;
        if (!loadBuild(*player, buildFile, build, error, warnings))
            return false;
        EngineScenario scenario;
        if (!loadEngineScenario(scenarioFile, scenario, error))
            return false;
        RenderOptions options;
        options.blockSize = block;
        options.seconds = seconds;
        RenderResult result;
        if (!renderEngine(*player, build, scenario, options, result, error))
            return false;
        left = std::move(result.audio.left);
        right = std::move(result.audio.right);
        return true;
    }
} // namespace tee::test
