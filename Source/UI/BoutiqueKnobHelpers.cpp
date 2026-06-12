#include "BoutiqueKnobHelpers.h"
#include "BoutiqueLayoutConstants.h"
#include "HydraPalette.h"

juce::String formatTimeReadout (const double seconds)
{
    if (seconds < 1.0)
        return juce::String (juce::roundToInt (seconds * 1000.0)) + " ms";

    return juce::String (seconds, 3) + " s";
}

double parseTimeReadout (const juce::String& text)
{
    auto trimmed = text.trim();

    if (trimmed.endsWithIgnoreCase ("ms"))
        return trimmed.dropLastCharacters (2).trim().getDoubleValue() / 1000.0;

    if (trimmed.endsWithIgnoreCase ("s"))
        return trimmed.dropLastCharacters (1).trim().getDoubleValue();

    return trimmed.getDoubleValue();
}

juce::String formatMillisecondsReadout (const double milliseconds)
{
    if (milliseconds < 1000.0)
        return juce::String (milliseconds, 1) + " ms";

    return juce::String (milliseconds / 1000.0, 3) + " s";
}

double parseMillisecondsReadout (const juce::String& text)
{
    auto trimmed = text.trim();

    if (trimmed.endsWithIgnoreCase ("ms"))
        return trimmed.dropLastCharacters (2).trim().getDoubleValue();

    if (trimmed.endsWithIgnoreCase ("s"))
        return trimmed.dropLastCharacters (1).trim().getDoubleValue() * 1000.0;

    return trimmed.getDoubleValue();
}

void styleKnobLabel (juce::Label& label)
{
    label.setFont (juce::Font (juce::FontOptions { 10.5f, juce::Font::bold }));
    label.setColour (juce::Label::textColourId, HydraPalette::colour (HydraPalette::textMuted));
    label.setMinimumHorizontalScale (1.0f);
}

void styleKnobValueReadout (juce::Slider& slider)
{
    slider.setColour (juce::Slider::textBoxTextColourId, HydraPalette::colour (HydraPalette::textMuted));
    slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
}

void applyKnobReadout (juce::Slider& slider,
                       const KnobReadoutKind kind,
                       const juce::String& valueSuffix)
{
    using namespace BoutiqueLayout;

    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, true, kCutoffTextBoxWidth, kCutoffTextBoxHeight);
    styleKnobValueReadout (slider);
    slider.setTextValueSuffix ({});
    slider.textFromValueFunction = nullptr;
    slider.valueFromTextFunction = nullptr;

    switch (kind)
    {
        case KnobReadoutKind::unitless2dp:
            slider.setNumDecimalPlacesToDisplay (2);
            slider.textFromValueFunction = [] (double value) { return juce::String (value, 2); };
            slider.valueFromTextFunction = [] (const juce::String& text) { return text.getDoubleValue(); };
            break;

        case KnobReadoutKind::unitless3dp:
            slider.setNumDecimalPlacesToDisplay (3);
            slider.textFromValueFunction = [] (double value) { return juce::String (value, 3); };
            slider.valueFromTextFunction = [] (const juce::String& text) { return text.getDoubleValue(); };
            break;

        case KnobReadoutKind::percent:
            slider.setNumDecimalPlacesToDisplay (0);
            slider.textFromValueFunction = [] (double value) { return juce::String (juce::roundToInt (value * 100.0)) + "%"; };
            slider.valueFromTextFunction = [] (const juce::String& text)
            {
                return text.retainCharacters ("0123456789.-").getDoubleValue() / 100.0;
            };
            break;

        case KnobReadoutKind::bipolar2dp:
            slider.setNumDecimalPlacesToDisplay (2);
            slider.textFromValueFunction = [] (double value)
            {
                const auto sign = value > 0.0 ? "+" : "";
                return sign + juce::String (value, 2);
            };
            slider.valueFromTextFunction = [] (const juce::String& text) { return text.getDoubleValue(); };
            break;

        case KnobReadoutKind::bipolar:
            slider.setNumDecimalPlacesToDisplay (3);
            slider.textFromValueFunction = [] (double value)
            {
                const auto sign = value > 0.0 ? "+" : "";
                return sign + juce::String (value, 3);
            };
            slider.valueFromTextFunction = [] (const juce::String& text) { return text.getDoubleValue(); };
            break;

        case KnobReadoutKind::hertz:
            slider.setNumDecimalPlacesToDisplay (6);
            slider.textFromValueFunction = [] (double value) { return juce::String (value, 6); };
            slider.valueFromTextFunction = [] (const juce::String& text) { return text.getDoubleValue(); };
            break;

        case KnobReadoutKind::timeSeconds:
            slider.textFromValueFunction = [] (double value) { return formatTimeReadout (value); };
            slider.valueFromTextFunction = [] (const juce::String& text) { return parseTimeReadout (text); };
            break;

        case KnobReadoutKind::timeMilliseconds:
            slider.textFromValueFunction = [] (double value) { return formatMillisecondsReadout (value); };
            slider.valueFromTextFunction = [] (const juce::String& text) { return parseMillisecondsReadout (text); };
            break;
    }

    if (valueSuffix.isNotEmpty())
        slider.setTextValueSuffix (valueSuffix);

    slider.updateText();
}

void configureRotaryKnob (juce::Slider& slider,
                          juce::Label& label,
                          const juce::String& labelText,
                          const KnobReadoutKind readoutKind,
                          ProceduralDarkLookAndFeel& lookAndFeel,
                          juce::Component& parent,
                          const juce::String& valueSuffix)
{
    slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    slider.setLookAndFeel (&lookAndFeel);
    applyKnobReadout (slider, readoutKind, valueSuffix);

    parent.addAndMakeVisible (slider);

    label.setText (labelText, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setInterceptsMouseClicks (false, false);
    styleKnobLabel (label);
    parent.addAndMakeVisible (label);
}

void configureAdsrKnob (juce::Slider& slider,
                        juce::Label& label,
                        const juce::String& labelText,
                        const KnobReadoutKind readoutKind,
                        ProceduralDarkLookAndFeel& lookAndFeel,
                        juce::Component& parent)
{
    slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    slider.setLookAndFeel (&lookAndFeel);
    applyKnobReadout (slider, readoutKind);

    parent.addAndMakeVisible (slider);

    label.setText (labelText, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setInterceptsMouseClicks (false, false);
    styleKnobLabel (label);
    parent.addAndMakeVisible (label);
}
