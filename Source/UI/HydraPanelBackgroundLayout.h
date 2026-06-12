#pragma once

#include <JuceHeader.h>
#include "HydraPalette.h"

class HydraPanelBackgroundLayout
{
public:
    static constexpr int zoneCount = 4;

    static void paintBackground (juce::Graphics& g, juce::Rectangle<int> bounds);
    static void paintHeader (juce::Graphics& g, juce::Rectangle<int> headerBounds, const juce::String& title);
    static void paintZoneCard (juce::Graphics& g, juce::Rectangle<float> bounds, const juce::String& zoneTitle);
    static std::array<juce::Rectangle<int>, zoneCount> layoutZoneColumns (juce::Rectangle<int> contentArea,
                                                                          int columnGap = 8);
};
