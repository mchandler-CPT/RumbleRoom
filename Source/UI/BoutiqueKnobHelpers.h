#pragma once

#include <JuceHeader.h>
#include "ProceduralDarkLookAndFeel.h"

enum class KnobReadoutKind
{
    unitless2dp,
    unitless3dp,
    percent,
    bipolar2dp,
    bipolar,
    hertz,
    timeSeconds,
    timeMilliseconds
};

juce::String formatTimeReadout (double seconds);
juce::String formatMillisecondsReadout (double milliseconds);

void styleKnobLabel (juce::Label& label);
void styleKnobValueReadout (juce::Slider& slider);
void applyKnobReadout (juce::Slider& slider,
                       KnobReadoutKind kind,
                       const juce::String& valueSuffix = {});

void configureRotaryKnob (juce::Slider& slider,
                          juce::Label& label,
                          const juce::String& labelText,
                          KnobReadoutKind readoutKind,
                          ProceduralDarkLookAndFeel& lookAndFeel,
                          juce::Component& parent,
                          const juce::String& valueSuffix = {});

void configureAdsrKnob (juce::Slider& slider,
                        juce::Label& label,
                        const juce::String& labelText,
                        KnobReadoutKind readoutKind,
                        ProceduralDarkLookAndFeel& lookAndFeel,
                        juce::Component& parent);
