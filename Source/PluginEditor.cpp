#include "PluginEditor.h"

RumbleRoomAudioProcessorEditor::RumbleRoomAudioProcessorEditor (RumbleRoomAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel (&boutiqueLookAndFeel);
    audioProcessor.apvts.addParameterListener ("sync", this);
    audioProcessor.apvts.addParameterListener ("bpm", this);

    configureKnob (sizeSlider, sizeLabel, "SIZE");
    configureKnob (dampSlider, dampLabel, "DAMP");
    configureKnob (brightSlider, brightLabel, "BRIGHT");
    configureKnob (bounceSlider, bounceLabel, "BOUNCE");
    configureKnob (gritSlider, gritLabel, "GRIT");
    configureKnob (mWidthSlider, mWidthLabel, "WIDTH");
    configureKnob (releaseSlider, releaseLabel, "RELEASE");
    configureKnob (duckSlider, duckLabel, "DUCK");
    configureKnob (mixSlider, mixLabel, "MIX");
    configureKnob (wowSlider, wowLabel, "WOW");
    configureKnob (wowSpeedSlider, wowSpeedLabel, "WOW SPD");
    configureKnob (diffuseSlider, diffuseLabel, "DIFFUSE");
    configureKnob (dampingSlider, dampingLabel, "DAMPING");

    syncLabel.setText ("SYNC", juce::dontSendNotification);
    syncLabel.setJustificationType (juce::Justification::centredLeft);
    syncLabel.setFont (juce::Font ("Times New Roman", 14.0f, juce::Font::bold));
    syncLabel.setColour (juce::Label::textColourId, BoutiqueLookAndFeel::getCreamColour());
    addAndMakeVisible (syncLabel);

    syncToggle.setClickingTogglesState (true);
    addAndMakeVisible (syncToggle);
    syncAttachment = std::make_unique<ButtonAttachment> (audioProcessor.apvts, "sync", syncToggle);
    syncToggle.onClick = [this] { updateSizeControlMode(); };

    bpmLabel.setText ("BPM", juce::dontSendNotification);
    bpmLabel.setJustificationType (juce::Justification::centredLeft);
    bpmLabel.setFont (juce::Font ("Times New Roman", 14.0f, juce::Font::bold));
    bpmLabel.setColour (juce::Label::textColourId, BoutiqueLookAndFeel::getCreamColour());
    addAndMakeVisible (bpmLabel);

    bpmEditor.setInputRestrictions (6, "0123456789.");
    bpmEditor.setJustification (juce::Justification::centred);
    bpmEditor.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0x66241815));
    bpmEditor.setColour (juce::TextEditor::textColourId, BoutiqueLookAndFeel::getCreamColour());
    bpmEditor.setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    bpmEditor.addListener (this);
    addAndMakeVisible (bpmEditor);
    refreshBpmTextFromParameter();

    sizeSlider.setScrollWheelEnabled (true);
    updateSizeControlMode();
    dampAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "cutoff", dampSlider);
    brightAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "hpCutoff", brightSlider);
    bounceAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "feedback", bounceSlider);
    gritAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "grit", gritSlider);
    mWidthAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "width", mWidthSlider);
    releaseAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "release", releaseSlider);
    duckAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "duckDepth", duckSlider);
    mixAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "dryWet", mixSlider);
    wowAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "wowDepth", wowSlider);
    wowSpeedAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "wowSpeed", wowSpeedSlider);
    diffuseAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "diffusion", diffuseSlider);
    dampingAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "damping", dampingSlider);

    setSize (1632, 240);
}

RumbleRoomAudioProcessorEditor::~RumbleRoomAudioProcessorEditor()
{
    audioProcessor.apvts.removeParameterListener ("sync", this);
    audioProcessor.apvts.removeParameterListener ("bpm", this);
    bpmEditor.removeListener (this);
    setLookAndFeel (nullptr);
}

void RumbleRoomAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (BoutiqueLookAndFeel::getBackgroundColour());

    auto bounds = getLocalBounds().toFloat();
    auto titleArea = bounds.removeFromTop (44.0f);

    g.setColour (BoutiqueLookAndFeel::getCreamColour().withAlpha (0.9f));
    g.setFont (juce::Font ("Times New Roman", 24.0f, juce::Font::bold));
    g.drawText ("RumbleRoom Boutique", titleArea.toNearestInt(), juce::Justification::centredLeft, true);

    const auto jewelRadius = 9.0f;
    const auto jewelCentre = juce::Point<float> (getWidth() - 32.0f, 24.0f);
    const auto activeGlow = 0.82f;

    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillEllipse (jewelCentre.x - jewelRadius - 1.5f, jewelCentre.y - jewelRadius + 1.5f, jewelRadius * 2.0f, jewelRadius * 2.0f);

    const auto glowAlpha = juce::jmap (activeGlow, 0.0f, 1.0f, 0.18f, 0.95f);
    g.setColour (BoutiqueLookAndFeel::getAmberColour().withAlpha (glowAlpha));
    g.fillEllipse (jewelCentre.x - jewelRadius, jewelCentre.y - jewelRadius, jewelRadius * 2.0f, jewelRadius * 2.0f);

    g.setColour (juce::Colours::white.withAlpha (0.35f + (0.35f * activeGlow)));
    g.fillEllipse (jewelCentre.x - (jewelRadius * 0.45f), jewelCentre.y - (jewelRadius * 0.65f), jewelRadius * 0.8f, jewelRadius * 0.8f);
}

void RumbleRoomAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (20, 14);
    auto topArea = area.removeFromTop (52);
    auto syncPanel = topArea.removeFromRight (220).reduced (4, 6);
    syncToggle.setBounds (syncPanel.removeFromLeft (24).withTrimmedTop (2));
    syncLabel.setBounds (syncPanel.removeFromLeft (58));
    bpmLabel.setBounds (syncPanel.removeFromLeft (38));
    bpmEditor.setBounds (syncPanel.removeFromLeft (72).reduced (0, 2));

    const auto rowWidth = 1586;
    auto centeredRow = area.withSizeKeepingCentre (rowWidth, area.getHeight());
    const auto knobWidth = centeredRow.getWidth() / 13;

    auto placeKnob = [knobWidth] (juce::Rectangle<int> slot, juce::Slider& slider, juce::Label& label)
    {
        auto column = slot.withWidth (knobWidth).reduced (10, 6);
        auto labelArea = column.removeFromTop (28);
        label.setBounds (labelArea);
        slider.setBounds (column.reduced (2, 0));
    };

    placeKnob (centeredRow.removeFromLeft (knobWidth), sizeSlider, sizeLabel);
    placeKnob (centeredRow.removeFromLeft (knobWidth), dampSlider, dampLabel);
    placeKnob (centeredRow.removeFromLeft (knobWidth), brightSlider, brightLabel);
    placeKnob (centeredRow.removeFromLeft (knobWidth), bounceSlider, bounceLabel);
    placeKnob (centeredRow.removeFromLeft (knobWidth), gritSlider, gritLabel);
    placeKnob (centeredRow.removeFromLeft (knobWidth), mWidthSlider, mWidthLabel);
    placeKnob (centeredRow.removeFromLeft (knobWidth), releaseSlider, releaseLabel);
    placeKnob (centeredRow.removeFromLeft (knobWidth), duckSlider, duckLabel);
    placeKnob (centeredRow.removeFromLeft (knobWidth), mixSlider, mixLabel);
    placeKnob (centeredRow.removeFromLeft (knobWidth), wowSlider, wowLabel);
    placeKnob (centeredRow.removeFromLeft (knobWidth), wowSpeedSlider, wowSpeedLabel);
    placeKnob (centeredRow.removeFromLeft (knobWidth), diffuseSlider, diffuseLabel);
    placeKnob (centeredRow.removeFromLeft (knobWidth), dampingSlider, dampingLabel);
}

void RumbleRoomAudioProcessorEditor::configureKnob (juce::Slider& slider, juce::Label& label, const juce::String& text)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 72, 20);
    slider.setColour (juce::Slider::textBoxTextColourId, BoutiqueLookAndFeel::getCreamColour());
    slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0x66241815));
    addAndMakeVisible (slider);

    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::Font ("Times New Roman", 17.0f, juce::Font::plain));
    label.setColour (juce::Label::textColourId, BoutiqueLookAndFeel::getCreamColour());
    addAndMakeVisible (label);
}

void RumbleRoomAudioProcessorEditor::updateSizeControlMode()
{
    const auto syncValue = audioProcessor.apvts.getRawParameterValue ("sync");
    const auto syncEnabled = (syncValue != nullptr && syncValue->load() > 0.5f);

    if (syncEnabled == sizeUsingSync && (sizeDelayAttachment != nullptr || sizeSubdivisionAttachment != nullptr))
        return;

    sizeDelayAttachment.reset();
    sizeSubdivisionAttachment.reset();

    if (syncEnabled)
    {
        sizeSlider.setRange (0.0, 10.0, 1.0);
        sizeSlider.setNumDecimalPlacesToDisplay (0);
        sizeSlider.setScrollWheelEnabled (true);
        sizeSlider.textFromValueFunction = [] (double value)
        {
            static const juce::StringArray labels { "1/1", "1/2", "1/2T", "1/4", "1/4T",
                                                    "1/8", "1/8T", "1/16", "1/16T", "1/32", "1/64" };
            const auto idx = juce::jlimit (0, labels.size() - 1, juce::roundToInt (value));
            return labels[idx];
        };
        sizeSlider.valueFromTextFunction = [] (const juce::String& text)
        {
            static const juce::StringArray labels { "1/1", "1/2", "1/2T", "1/4", "1/4T",
                                                    "1/8", "1/8T", "1/16", "1/16T", "1/32", "1/64" };
            const auto idx = labels.indexOf (text.trim());
            return idx >= 0 ? static_cast<double> (idx) : 3.0;
        };
        sizeSubdivisionAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "subdivision", sizeSlider);
    }
    else
    {
        sizeSlider.setRange (0.1, 1000.0, 0.01);
        sizeSlider.setSkewFactor (0.35);
        sizeSlider.setNumDecimalPlacesToDisplay (1);
        sizeSlider.setScrollWheelEnabled (false);
        sizeSlider.textFromValueFunction = [] (double value) { return juce::String (value, 1) + " ms"; };
        sizeSlider.valueFromTextFunction = nullptr;
        sizeDelayAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "delayTime", sizeSlider);
    }

    sizeUsingSync = syncEnabled;
}

void RumbleRoomAudioProcessorEditor::parameterChanged (const juce::String& parameterID, float)
{
    if (parameterID == "sync" || parameterID == "bpm")
        triggerAsyncUpdate();
}

void RumbleRoomAudioProcessorEditor::handleAsyncUpdate()
{
    updateSizeControlMode();
    refreshBpmTextFromParameter();
}

void RumbleRoomAudioProcessorEditor::textEditorReturnKeyPressed (juce::TextEditor& editor)
{
    if (&editor == &bpmEditor)
        pushBpmTextToParameter();
}

void RumbleRoomAudioProcessorEditor::textEditorFocusLost (juce::TextEditor& editor)
{
    if (&editor == &bpmEditor)
        pushBpmTextToParameter();
}

void RumbleRoomAudioProcessorEditor::pushBpmTextToParameter()
{
    const auto entered = bpmEditor.getText().getFloatValue();
    const auto clamped = juce::jlimit (40.0f, 260.0f, entered);
    if (auto* param = audioProcessor.apvts.getParameter ("bpm"))
    {
        const auto normalised = param->convertTo0to1 (clamped);
        param->beginChangeGesture();
        param->setValueNotifyingHost (normalised);
        param->endChangeGesture();
    }
    refreshBpmTextFromParameter();
}

void RumbleRoomAudioProcessorEditor::refreshBpmTextFromParameter()
{
    const auto bpmValue = audioProcessor.apvts.getRawParameterValue ("bpm");
    if (bpmValue != nullptr)
        bpmEditor.setText (juce::String (bpmValue->load(), 1), juce::dontSendNotification);
}
