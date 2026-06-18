#pragma once

#include <JuceHeader.h>

class ChromeTextButton : public juce::ToggleButton
{
public:
    ChromeTextButton();

private:
    void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
};

class ChromeActionButton : public juce::TextButton
{
public:
    ChromeActionButton();

private:
    void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
};
