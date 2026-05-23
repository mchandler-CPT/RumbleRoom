#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class RumbleRoomAudioProcessorEditor : public juce::AudioProcessorEditor,
                                       private juce::AudioProcessorValueTreeState::Listener,
                                       private juce::AsyncUpdater,
                                       private juce::TextEditor::Listener
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
    void updateSizeControlMode();
    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void handleAsyncUpdate() override;
    void textEditorReturnKeyPressed (juce::TextEditor& editor) override;
    void textEditorFocusLost (juce::TextEditor& editor) override;
    void pushBpmTextToParameter();
    void refreshBpmTextFromParameter();

    RumbleRoomAudioProcessor& audioProcessor;
    BoutiqueLookAndFeel boutiqueLookAndFeel;

    juce::Slider sizeSlider;
    juce::Slider dampSlider;
    juce::Slider bounceSlider;
    juce::Slider gritSlider;
    juce::Slider releaseSlider;
    juce::Slider mixSlider;

    juce::Label sizeLabel;
    juce::Label dampLabel;
    juce::Label bounceLabel;
    juce::Label gritLabel;
    juce::Label releaseLabel;
    juce::Label mixLabel;
    juce::ToggleButton syncToggle;
    juce::Label syncLabel;
    juce::Label bpmLabel;
    juce::TextEditor bpmEditor;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<SliderAttachment> sizeDelayAttachment;
    std::unique_ptr<SliderAttachment> sizeSubdivisionAttachment;
    std::unique_ptr<SliderAttachment> dampAttachment;
    std::unique_ptr<SliderAttachment> bounceAttachment;
    std::unique_ptr<SliderAttachment> gritAttachment;
    std::unique_ptr<SliderAttachment> releaseAttachment;
    std::unique_ptr<SliderAttachment> mixAttachment;
    std::unique_ptr<ButtonAttachment> syncAttachment;

    bool sizeUsingSync { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RumbleRoomAudioProcessorEditor)
};
