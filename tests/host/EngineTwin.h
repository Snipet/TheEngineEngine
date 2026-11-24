// tests/host/EngineTwin.h: K3a's EnginePlayer rendering a build and a scenario, behind a function, so that the test
// that compares SimHost with it need not include EnginePlayer.h (whose tee::Build is another class than the document).
#pragma once

#include <string>
#include <vector>

namespace tee::test
{
    bool enginePlayerRender(const std::string& buildFile, const std::string& scenarioFile, double seconds, int block,
                            std::vector<float>& left, std::vector<float>& right, std::string& error);
}
