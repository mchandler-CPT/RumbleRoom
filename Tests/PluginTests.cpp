#include <catch2/catch_test_macros.hpp>

#include "PluginProcessor.h"

TEST_CASE ("RumbleRoom processor constructs", "[processor]")
{
    RumbleRoomAudioProcessor processor;
    REQUIRE (processor.getName().isNotEmpty());
}

TEST_CASE ("RumbleRoom exposes expected parameters", "[processor]")
{
    RumbleRoomAudioProcessor processor;
    REQUIRE (processor.apvts.getParameter ("delayTime") != nullptr);
    REQUIRE (processor.apvts.getParameter ("hpCutoff") != nullptr);
    REQUIRE (processor.apvts.getParameter ("cutoff") != nullptr);
} 
