// tests/ProgramTest.cpp: the reference compiles, and the host reads it as its contract says.
#include "tee/RefPlayer.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("the reference compiles with no error", "[ref][compile]")
{
    tee::RefPlayer::Options options;
    options.directory = TEE_SOURCE_SIM_DIR;
    std::string errors;
    const auto player = tee::RefPlayer::create(options, errors);
    INFO(errors);
    REQUIRE(player != nullptr);
    CHECK(errors.empty());
    CHECK(player->dials().size() == 32);
    CHECK(player->showIndex("showRpm") >= 0);
}
