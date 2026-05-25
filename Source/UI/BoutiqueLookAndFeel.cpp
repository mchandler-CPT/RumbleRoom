#include "BoutiqueLookAndFeel.h"

namespace
{
const auto boutiqueBackground = juce::Colour (0xFF1A1512);
const auto boutiqueAmber = juce::Colour (0xFFFFA500);
const auto boutiqueCopper = juce::Colour (0xFFB87333);
const auto boutiqueCream = juce::Colour (0xFFF3E6D0);
} // namespace

BoutiqueLookAndFeel::BoutiqueLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, juce::Colour (0xFF161412));
    setColour (juce::Slider::rotarySliderFillColourId, boutiqueAmber);
    setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xFF2B2A28));
    setColour (juce::Slider::thumbColourId, boutiqueCopper);
    setColour (juce::Label::textColourId, boutiqueCream);
}

juce::Colour BoutiqueLookAndFeel::getBackgroundColour() noexcept { return boutiqueBackground; }
juce::Colour BoutiqueLookAndFeel::getAmberColour() noexcept { return boutiqueAmber; }
juce::Colour BoutiqueLookAndFeel::getCopperColour() noexcept { return boutiqueCopper; }
juce::Colour BoutiqueLookAndFeel::getCreamColour() noexcept { return boutiqueCream; }

void BoutiqueLookAndFeel::drawRotarySlider (juce::Graphics& g,
                                            const int x,
                                            const int y,
                                            const int width,
                                            const int height,
                                            const float sliderPosProportional,
                                            const float rotaryStartAngle,
                                            const float rotaryEndAngle,
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

    juce::Path shine;
    shine.addPieSegment (bounds.reduced (radius * 0.22f), juce::MathConstants<float>::pi * 1.18f,
                         juce::MathConstants<float>::pi * 1.78f, 0.45f);
    g.setColour (juce::Colours::white.withAlpha (0.17f));
    g.fillPath (shine);

    g.setColour (findColour (juce::Slider::rotarySliderOutlineColourId).brighter (0.4f));
    g.drawEllipse (bounds, 1.0f);
}
