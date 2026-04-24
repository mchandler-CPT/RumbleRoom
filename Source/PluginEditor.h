#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class RumbleRoomAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit RumbleRoomAudioProcessorEditor (RumbleRoomAudioProcessor&);
    ~RumbleRoomAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
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
    };

    void configureKnob (juce::Slider& slider, juce::Label& label, const juce::String& text);

    RumbleRoomAudioProcessor& audioProcessor;
    BoutiqueLookAndFeel boutiqueLookAndFeel;

    juce::Slider sizeSlider;
    juce::Slider dampSlider;
    juce::Slider bounceSlider;
    juce::Slider gritSlider;
    juce::Slider mixSlider;

    juce::Label sizeLabel;
    juce::Label dampLabel;
    juce::Label bounceLabel;
    juce::Label gritLabel;
    juce::Label mixLabel;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAttachment> sizeAttachment;
    std::unique_ptr<SliderAttachment> dampAttachment;
    std::unique_ptr<SliderAttachment> bounceAttachment;
    std::unique_ptr<SliderAttachment> gritAttachment;
    std::unique_ptr<SliderAttachment> mixAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RumbleRoomAudioProcessorEditor)
};
