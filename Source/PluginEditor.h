#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UI/BoutiqueKnobHelpers.h"
#include "UI/ChromeTextButton.h"
#include "UI/HydraPalette.h"
#include "UI/ProceduralDarkLookAndFeel.h"

class RumbleRoomAudioProcessorEditor : public juce::AudioProcessorEditor,
                                       private juce::AudioProcessorValueTreeState::Listener
{
public:
    explicit RumbleRoomAudioProcessorEditor (RumbleRoomAudioProcessor&);
    ~RumbleRoomAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void updateDelayTimeControlMode();
    void refreshDelayTimeReadout();
    void refreshPresetUi();
    void layoutPresetHeader (juce::Rectangle<int> headerArea);
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    static void alignKnobCell (juce::Rectangle<int> cell, juce::Slider& slider, juce::Label& label);
    static void alignMixKnobCell (juce::Rectangle<int> cell, juce::Slider& slider, juce::Label& label);
    static void layoutDualKnobModule (juce::Rectangle<int> module,
                                      juce::Slider& topSlider,
                                      juce::Label& topLabel,
                                      juce::Slider& bottomSlider,
                                      juce::Label& bottomLabel);

    RumbleRoomAudioProcessor& audioProcessor;
    ProceduralDarkLookAndFeel customLookAndFeel;
    juce::Image mLogoImage;

    ChromeTextButton mSyncButton;
    ChromeTextButton mPrevButton, mNextButton, mSaveButton, mSetFolderButton;
    juce::Label mPresetLabel;

    juce::Slider mDelayTimeSlider;
    juce::Slider mFeedbackSlider;
    juce::Slider mDuckDepthSlider;
    juce::Slider mDuckReleaseSlider;
    juce::Slider mLowPassSlider;
    juce::Slider mHighPassSlider;
    juce::Slider mDiffusionSlider;
    juce::Slider mFeedbackDampSlider;
    juce::Slider mWowSpeedSlider;
    juce::Slider mWowDepthSlider;
    juce::Slider mGritSlider;
    juce::Slider mWidthSlider;
    juce::Slider mMixSlider;

    juce::Label mDelayTimeLabel;
    juce::Label mFeedbackLabel;
    juce::Label mDuckDepthLabel;
    juce::Label mDuckReleaseLabel;
    juce::Label mLowPassLabel;
    juce::Label mHighPassLabel;
    juce::Label mDiffusionLabel;
    juce::Label mFeedbackDampLabel;
    juce::Label mWowSpeedLabel;
    juce::Label mWowDepthLabel;
    juce::Label mGritLabel;
    juce::Label mWidthLabel;
    juce::Label mMixLabel;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<ButtonAttachment> mSyncAttachment;
    std::unique_ptr<juce::FileChooser> mPresetSaveChooser, mPresetFolderChooser;
    std::unique_ptr<SliderAttachment> mDelayTimeAttachment;
    std::unique_ptr<SliderAttachment> mSubdivisionAttachment;
    std::unique_ptr<SliderAttachment> mFeedbackAttachment;
    std::unique_ptr<SliderAttachment> mDuckDepthAttachment;
    std::unique_ptr<SliderAttachment> mDuckReleaseAttachment;
    std::unique_ptr<SliderAttachment> mLowPassAttachment;
    std::unique_ptr<SliderAttachment> mHighPassAttachment;
    std::unique_ptr<SliderAttachment> mDiffusionAttachment;
    std::unique_ptr<SliderAttachment> mFeedbackDampAttachment;
    std::unique_ptr<SliderAttachment> mWowSpeedAttachment;
    std::unique_ptr<SliderAttachment> mWowDepthAttachment;
    std::unique_ptr<SliderAttachment> mGritAttachment;
    std::unique_ptr<SliderAttachment> mWidthAttachment;
    std::unique_ptr<SliderAttachment> mMixAttachment;

    bool mDelayUsingSync { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RumbleRoomAudioProcessorEditor)
};
