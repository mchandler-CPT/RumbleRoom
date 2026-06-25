#include "ProceduralDarkLookAndFeel.h"

ProceduralDarkLookAndFeel::ProceduralDarkLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, HydraPalette::colour (HydraPalette::backgroundBase));
    setColour (juce::Slider::rotarySliderFillColourId, HydraPalette::colour (HydraPalette::dialFill));
    setColour (juce::Slider::rotarySliderOutlineColourId, HydraPalette::colour (HydraPalette::dialTrack));
    setColour (juce::Slider::thumbColourId, HydraPalette::colour (HydraPalette::dialPointer));
    setColour (juce::Slider::textBoxTextColourId, HydraPalette::colour (HydraPalette::accentGoldBright));
    setColour (juce::Slider::textBoxBackgroundColourId, HydraPalette::colour (HydraPalette::panelRaised));
    setColour (juce::Slider::textBoxOutlineColourId, HydraPalette::colour (HydraPalette::borderMuted));
    setColour (juce::Label::textColourId, HydraPalette::colour (HydraPalette::textLabel));
    setColour (juce::ComboBox::backgroundColourId, HydraPalette::colour (HydraPalette::panelRaised));
    setColour (juce::ComboBox::outlineColourId, HydraPalette::colour (HydraPalette::borderMuted));
    setColour (juce::ComboBox::textColourId, HydraPalette::colour (HydraPalette::accentGoldBright));
    setColour (juce::ComboBox::arrowColourId, HydraPalette::colour (HydraPalette::accentGold));
    setColour (juce::PopupMenu::backgroundColourId, HydraPalette::colour (HydraPalette::panelRaised));
    setColour (juce::PopupMenu::highlightedBackgroundColourId, HydraPalette::colour (HydraPalette::backgroundWarmTint));
    setColour (juce::PopupMenu::textColourId, HydraPalette::colour (HydraPalette::textMuted));
    setColour (juce::PopupMenu::highlightedTextColourId, HydraPalette::colour (HydraPalette::accentGoldBright));
    setColour (juce::TextEditor::backgroundColourId, HydraPalette::colour (HydraPalette::panelRaised));
    setColour (juce::TextEditor::textColourId, HydraPalette::colour (HydraPalette::accentGoldBright));
    setColour (juce::TextEditor::outlineColourId, HydraPalette::colour (HydraPalette::borderMuted));
}

void ProceduralDarkLookAndFeel::drawRotarySlider (juce::Graphics& g,
                                                   const int x,
                                                   const int y,
                                                   const int width,
                                                   const int height,
                                                   const float sliderPosProportional,
                                                   const float rotaryStartAngle,
                                                   const float rotaryEndAngle,
                                                   juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<float> (static_cast<float> (x), static_cast<float> (y),
                                                static_cast<float> (width), static_cast<float> (height)).reduced (6.0f);
    const auto dialSize = juce::jmin (bounds.getWidth(), bounds.getHeight());
    const auto dialBounds = bounds.withSizeKeepingCentre (dialSize, dialSize);
    const auto centre = dialBounds.getCentre();
    const auto radius = dialSize * 0.5f;
    const auto angle = juce::jmap (sliderPosProportional, 0.0f, 1.0f, rotaryStartAngle, rotaryEndAngle);

    const auto gutterRadius = radius - 1.5f;
    const auto faceRadius = radius - 7.5f;
    const auto dangerRadius = gutterRadius - 4.2f;
    const auto faceBounds = dialBounds.withSizeKeepingCentre (faceRadius * 2.0f, faceRadius * 2.0f);

    // 0. Cast shadow beneath the dial body for floating depth.
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillEllipse (dialBounds.translated (0.0f, 2.5f).reduced (1.0f));

    // 1. Dark recessed gutter ring
    juce::Path gutterRing;
    gutterRing.addCentredArc (centre.x, centre.y, gutterRadius, gutterRadius, 0.0f,
                              0.0f, juce::MathConstants<float>::twoPi, true);
    g.setColour (HydraPalette::colour (HydraPalette::backgroundBase).darker (0.55f));
    g.strokePath (gutterRing, juce::PathStrokeType (5.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.setColour (HydraPalette::colour (HydraPalette::dialTrack).darker (0.25f));
    g.strokePath (gutterRing, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const bool isDangerZoneKnob = static_cast<bool> (slider.getProperties()["dangerZoneKnob"]);
    if (isDangerZoneKnob)
    {
        const auto dangerStartAngle = juce::jmap (0.5f, 0.0f, 1.0f, rotaryStartAngle, rotaryEndAngle);
        juce::Path dangerArc;
        dangerArc.addCentredArc (centre.x, centre.y, dangerRadius, dangerRadius, 0.0f,
                                 dangerStartAngle, rotaryEndAngle, true);

        const auto redDanger = juce::Colour::fromRGB (0xCC, 0x2A, 0x2A);
        g.setColour (redDanger.withAlpha (0.42f));
        g.strokePath (dangerArc, juce::PathStrokeType (2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        if (sliderPosProportional > 0.5f)
        {
            const auto activeDanger = juce::jmap (sliderPosProportional, 0.5f, 1.0f, dangerStartAngle, rotaryEndAngle);
            juce::Path activeDangerArc;
            activeDangerArc.addCentredArc (centre.x, centre.y, dangerRadius, dangerRadius, 0.0f,
                                           dangerStartAngle, activeDanger, true);
            g.setColour (redDanger.withAlpha (0.88f));
            g.strokePath (activeDangerArc, juce::PathStrokeType (2.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
    }

    // 2. Elevated inner knob face — metallic vertical gradient
    juce::ColourGradient faceGradient (HydraPalette::colour (HydraPalette::dialBodyTop), centre.x, faceBounds.getY(),
                                       HydraPalette::colour (HydraPalette::dialBodyBottom), centre.x, faceBounds.getBottom(), false);
    g.setGradientFill (faceGradient);
    g.fillEllipse (faceBounds);

    // 2b. Inner top shadow + lower catch-light for a domed feel.
    juce::ColourGradient innerShade (juce::Colours::black.withAlpha (0.30f), centre.x, faceBounds.getY(),
                                     juce::Colours::transparentBlack, centre.x, faceBounds.getCentreY(), false);
    g.setGradientFill (innerShade);
    g.fillEllipse (faceBounds);

    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.drawEllipse (faceBounds.reduced (0.5f).translated (0.0f, 0.8f), 1.0f);

    // 3. Outer highlight lip
    g.setColour (HydraPalette::colour (HydraPalette::accentGoldBright).withAlpha (0.22f));
    g.drawEllipse (faceBounds.expanded (0.75f), 1.25f);
    g.setColour (HydraPalette::colour (HydraPalette::borderMuted).withAlpha (0.55f));
    g.drawEllipse (faceBounds, 0.85f);

    // 4. Active outer illumination arc (rounded caps)
    const auto arcRadius = gutterRadius - 1.0f;
    juce::Path valueArc;
    valueArc.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, rotaryStartAngle, angle, true);

    juce::ColourGradient fillArc (HydraPalette::colour (HydraPalette::accentGoldBright), centre.x, dialBounds.getY(),
                                  HydraPalette::colour (HydraPalette::accentGoldDim), centre.x, dialBounds.getBottom(), false);
    g.setGradientFill (fillArc);
    g.strokePath (valueArc, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // 5. High-contrast indicator pointer
    juce::Path pointer;
    const auto pointerLength = faceRadius * 0.72f;
    const auto pointerWidth = juce::jmax (2.4f, faceRadius * 0.09f);
    pointer.addRoundedRectangle (-pointerWidth * 0.5f,
                                 -faceRadius + 4.0f,
                                 pointerWidth,
                                 pointerLength,
                                 1.2f);
    pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));
    g.setColour (HydraPalette::colour (HydraPalette::dialPointer));
    g.fillPath (pointer);

    g.setColour (juce::Colours::white.withAlpha (0.18f));
    g.strokePath (pointer, juce::PathStrokeType (0.6f));
}

void ProceduralDarkLookAndFeel::drawToggleButton (juce::Graphics& g,
                                                   juce::ToggleButton& button,
                                                   bool,
                                                   bool)
{
    const auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
    const auto isOn = button.getToggleState();

    g.setColour (HydraPalette::colour (isOn ? HydraPalette::snapOnFill : HydraPalette::snapOffFill));
    g.fillRoundedRectangle (bounds, 4.0f);

    g.setColour (HydraPalette::colour (isOn ? HydraPalette::accentGoldDim : HydraPalette::snapOffBorder));
    g.drawRoundedRectangle (bounds, 4.0f, 1.0f);

    g.setColour (HydraPalette::colour (isOn ? HydraPalette::snapOnText : HydraPalette::snapOffText));
    g.setFont (juce::Font (12.0f, juce::Font::bold));
    g.drawText (button.getButtonText(), bounds.toNearestInt(), juce::Justification::centred, false);
}

void ProceduralDarkLookAndFeel::drawComboBox (juce::Graphics& g,
                                              const int width,
                                              const int height,
                                              const bool,
                                              const int,
                                              const int,
                                              const int,
                                              const int,
                                              juce::ComboBox& box)
{
    const auto bounds = juce::Rectangle<float> (0.0f, 0.0f, static_cast<float> (width), static_cast<float> (height)).reduced (0.5f);

    g.setColour (HydraPalette::colour (HydraPalette::panelRaised));
    g.fillRoundedRectangle (bounds, 4.0f);

    g.setColour (HydraPalette::colour (HydraPalette::borderMuted));
    g.drawRoundedRectangle (bounds, 4.0f, 1.0f);

    juce::Path arrow;
    auto arrowArea = bounds;
    const auto arrowBounds = arrowArea.removeFromRight (18.0f).reduced (5.0f, height * 0.35f);
    arrow.addTriangle (arrowBounds.getX(), arrowBounds.getY(),
                       arrowBounds.getRight(), arrowBounds.getY(),
                       arrowBounds.getCentreX(), arrowBounds.getBottom());
    g.setColour (HydraPalette::colour (HydraPalette::accentGold));
    g.fillPath (arrow);

    g.setColour (box.findColour (juce::ComboBox::textColourId));
    g.setFont (juce::Font (12.0f));
    g.drawFittedText (box.getText(), arrowArea.reduced (8.0f, 0.0f).toNearestInt(),
                      juce::Justification::centredLeft, 1);
}

juce::Font ProceduralDarkLookAndFeel::getLabelFont (juce::Label& label)
{
    if (label.getHeight() > 0 && label.getHeight() <= 16)
        return juce::Font (juce::FontOptions { 9.0f });

    return LookAndFeel_V4::getLabelFont (label);
}

void ProceduralDarkLookAndFeel::drawLabel (juce::Graphics& g, juce::Label& label)
{
    g.setColour (label.findColour (juce::Label::textColourId));
    g.setFont (getLabelFont (label));
    g.drawText (label.getText(), label.getLocalBounds(), label.getJustificationType(), true);
}
