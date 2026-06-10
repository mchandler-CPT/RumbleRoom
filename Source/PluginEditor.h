#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UI/BoutiqueLookAndFeel.h"

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
    juce::Slider brightSlider;
    juce::Slider bounceSlider;
    juce::Slider gritSlider;
    juce::Slider mWidthSlider;
    juce::Slider releaseSlider;
    juce::Slider mixSlider;
    juce::Slider wowSlider;
    juce::Slider wowSpeedSlider;
    juce::Slider diffuseSlider;
    juce::Slider dampingSlider;

    juce::Label sizeLabel;
    juce::Label dampLabel;
    juce::Label brightLabel;
    juce::Label bounceLabel;
    juce::Label gritLabel;
    juce::Label mWidthLabel;
    juce::Label releaseLabel;
    juce::Label mixLabel;
    juce::Label wowLabel;
    juce::Label wowSpeedLabel;
    juce::Label diffuseLabel;
    juce::Label dampingLabel;
    juce::ToggleButton syncToggle;
    juce::Label syncLabel;
    juce::Label bpmLabel;
    juce::TextEditor bpmEditor;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<SliderAttachment> sizeDelayAttachment;
    std::unique_ptr<SliderAttachment> sizeSubdivisionAttachment;
    std::unique_ptr<SliderAttachment> dampAttachment;
    std::unique_ptr<SliderAttachment> brightAttachment;
    std::unique_ptr<SliderAttachment> bounceAttachment;
    std::unique_ptr<SliderAttachment> gritAttachment;
    std::unique_ptr<SliderAttachment> mWidthAttachment;
    std::unique_ptr<SliderAttachment> releaseAttachment;
    std::unique_ptr<SliderAttachment> mixAttachment;
    std::unique_ptr<SliderAttachment> wowAttachment;
    std::unique_ptr<SliderAttachment> wowSpeedAttachment;
    std::unique_ptr<SliderAttachment> diffuseAttachment;
    std::unique_ptr<SliderAttachment> dampingAttachment;
    std::unique_ptr<ButtonAttachment> syncAttachment;

    bool sizeUsingSync { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RumbleRoomAudioProcessorEditor)
};
