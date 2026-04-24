#include "PluginEditor.h"

namespace
{
const auto boutiqueBackground = juce::Colour (0xFF1A1512);
const auto boutiqueAmber = juce::Colour (0xFFFFA500);
const auto boutiqueCopper = juce::Colour (0xFFB87333);
const auto boutiqueCream = juce::Colour (0xFFF3E6D0);
} // namespace

RumbleRoomAudioProcessorEditor::BoutiqueLookAndFeel::BoutiqueLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, juce::Colour (0xFF161412));
    setColour (juce::Slider::rotarySliderFillColourId, boutiqueAmber);
    setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xFF2B2A28));
    setColour (juce::Slider::thumbColourId, boutiqueCopper);
    setColour (juce::Label::textColourId, boutiqueCream);
}

RumbleRoomAudioProcessorEditor::RumbleRoomAudioProcessorEditor (RumbleRoomAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel (&boutiqueLookAndFeel);

    configureKnob (sizeSlider, sizeLabel, "SIZE");
    configureKnob (dampSlider, dampLabel, "DAMP");
    configureKnob (bounceSlider, bounceLabel, "BOUNCE");
    configureKnob (gritSlider, gritLabel, "GRIT");
    configureKnob (mixSlider, mixLabel, "MIX");

    sizeAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "delayTime", sizeSlider);
    dampAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "cutoff", dampSlider);
    bounceAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "feedback", bounceSlider);
    gritAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "grit", gritSlider);
    mixAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "dryWet", mixSlider);

    setSize (780, 240);
}

RumbleRoomAudioProcessorEditor::~RumbleRoomAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void RumbleRoomAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (boutiqueBackground);

    auto bounds = getLocalBounds().toFloat();
    auto titleArea = bounds.removeFromTop (44.0f);

    g.setColour (boutiqueCream.withAlpha (0.9f));
    g.setFont (juce::Font ("Times New Roman", 24.0f, juce::Font::bold));
    g.drawText ("RumbleRoom Boutique", titleArea.toNearestInt(), juce::Justification::centredLeft, true);

    const auto jewelRadius = 9.0f;
    const auto jewelCentre = juce::Point<float> (getWidth() - 32.0f, 24.0f);
    const auto activeGlow = 0.82f;

    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillEllipse (jewelCentre.x - jewelRadius - 1.5f, jewelCentre.y - jewelRadius + 1.5f, jewelRadius * 2.0f, jewelRadius * 2.0f);

    const auto glowAlpha = juce::jmap (activeGlow, 0.0f, 1.0f, 0.18f, 0.95f);
    g.setColour (boutiqueAmber.withAlpha (glowAlpha));
    g.fillEllipse (jewelCentre.x - jewelRadius, jewelCentre.y - jewelRadius, jewelRadius * 2.0f, jewelRadius * 2.0f);

    g.setColour (juce::Colours::white.withAlpha (0.35f + (0.35f * activeGlow)));
    g.fillEllipse (jewelCentre.x - (jewelRadius * 0.45f), jewelCentre.y - (jewelRadius * 0.65f), jewelRadius * 0.8f, jewelRadius * 0.8f);
}

void RumbleRoomAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (20, 14);
    area.removeFromTop (52);
    const auto rowWidth = 700;
    auto centeredRow = area.withSizeKeepingCentre (rowWidth, area.getHeight());
    const auto knobWidth = centeredRow.getWidth() / 5;

    auto placeKnob = [knobWidth] (juce::Rectangle<int> slot, juce::Slider& slider, juce::Label& label)
    {
        auto column = slot.withWidth (knobWidth).reduced (10, 6);
        auto labelArea = column.removeFromTop (28);
        label.setBounds (labelArea);
        slider.setBounds (column.reduced (2, 0));
    };

    placeKnob (centeredRow.removeFromLeft (knobWidth), sizeSlider, sizeLabel);
    placeKnob (centeredRow.removeFromLeft (knobWidth), dampSlider, dampLabel);
    placeKnob (centeredRow.removeFromLeft (knobWidth), bounceSlider, bounceLabel);
    placeKnob (centeredRow.removeFromLeft (knobWidth), gritSlider, gritLabel);
    placeKnob (centeredRow.removeFromLeft (knobWidth), mixSlider, mixLabel);
}

void RumbleRoomAudioProcessorEditor::configureKnob (juce::Slider& slider, juce::Label& label, const juce::String& text)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 72, 20);
    slider.setColour (juce::Slider::textBoxTextColourId, boutiqueCream);
    slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0x66241815));
    addAndMakeVisible (slider);

    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::Font ("Times New Roman", 17.0f, juce::Font::plain));
    label.setColour (juce::Label::textColourId, boutiqueCream);
    addAndMakeVisible (label);
}

void RumbleRoomAudioProcessorEditor::BoutiqueLookAndFeel::drawRotarySlider (juce::Graphics& g,
                                                                             int x,
                                                                             int y,
                                                                             int width,
                                                                             int height,
                                                                             float sliderPosProportional,
                                                                             float rotaryStartAngle,
                                                                             float rotaryEndAngle,
                                                                             juce::Slider&)
{
    const auto bounds = juce::Rectangle<float> (static_cast<float> (x), static_cast<float> (y),
                                                static_cast<float> (width), static_cast<float> (height)).reduced (6.0f);
    const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const auto angle = juce::jmap (sliderPosProportional, 0.0f, 1.0f, rotaryStartAngle, rotaryEndAngle);

    juce::ColourGradient bodyGradient (boutiqueBackground.brighter (0.18f), centre.x, bounds.getY(),
                                       boutiqueBackground.darker (0.18f), centre.x, bounds.getBottom(), false);
    g.setGradientFill (bodyGradient);
    g.fillEllipse (bounds);

    juce::Path track;
    track.addCentredArc (centre.x, centre.y, radius - 5.0f, radius - 5.0f, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (findColour (juce::Slider::rotarySliderOutlineColourId).brighter (0.2f));
    g.strokePath (track, juce::PathStrokeType (2.0f));

    juce::Path valueArc;
    valueArc.addCentredArc (centre.x, centre.y, radius - 5.0f, radius - 5.0f, 0.0f, rotaryStartAngle, angle, true);
    juce::ColourGradient amberArc (boutiqueAmber.brighter (0.2f), centre.x, bounds.getY(),
                                   boutiqueAmber.darker (0.30f), centre.x, bounds.getBottom(), false);
    g.setGradientFill (amberArc);
    g.strokePath (valueArc, juce::PathStrokeType (3.5f));

    juce::Path pointer;
    pointer.addRectangle (-1.2f, -radius + 9.0f, 2.4f, radius * 0.48f);
    pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));
    g.setColour (boutiqueCopper);
    g.fillPath (pointer);

    // Synth-style glass sheen.
    juce::Path shine;
    shine.addPieSegment (bounds.reduced (radius * 0.22f), juce::MathConstants<float>::pi * 1.18f,
                         juce::MathConstants<float>::pi * 1.78f, 0.45f);
    g.setColour (juce::Colours::white.withAlpha (0.17f));
    g.fillPath (shine);

    g.setColour (findColour (juce::Slider::rotarySliderOutlineColourId).brighter (0.4f));
    g.drawEllipse (bounds, 1.0f);
}
