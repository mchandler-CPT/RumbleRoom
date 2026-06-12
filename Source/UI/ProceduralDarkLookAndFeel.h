#pragma once

#include <JuceHeader.h>
#include "HydraPalette.h"

class ProceduralDarkLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ProceduralDarkLookAndFeel();

    void drawRotarySlider (juce::Graphics& g,
                           int x,
                           int y,
                           int width,
                           int height,
                           float sliderPosProportional,
                           float rotaryStartAngle,
                           float rotaryEndAngle,
                           juce::Slider& slider) override;

    void drawToggleButton (juce::Graphics& g,
                           juce::ToggleButton& button,
                           bool shouldDrawButtonAsHighlighted,
                           bool shouldDrawButtonAsDown) override;

    void drawComboBox (juce::Graphics& g,
                       int width,
                       int height,
                       bool isButtonDown,
                       int buttonX,
                       int buttonY,
                       int buttonW,
                       int buttonH,
                       juce::ComboBox& box) override;

    void drawLabel (juce::Graphics& g, juce::Label& label) override;

    juce::Font getLabelFont (juce::Label& label) override;
};
