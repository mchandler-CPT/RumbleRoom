#pragma once

#include <JuceHeader.h>

class BoutiqueLookAndFeel : public juce::LookAndFeel_V4
{
public:
    BoutiqueLookAndFeel();

    void drawRotarySlider (juce::Graphics& g,
                           int x,
                           int y,
                           int width,
                           int height,
                           float sliderPosProportional,
                           float rotaryStartAngle,
                           float rotaryEndAngle,
                           juce::Slider& slider) override;

    static juce::Colour getBackgroundColour() noexcept;
    static juce::Colour getAmberColour() noexcept;
    static juce::Colour getCopperColour() noexcept;
    static juce::Colour getCreamColour() noexcept;
};
