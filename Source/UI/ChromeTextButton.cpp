#include "ChromeTextButton.h"
#include "HydraPalette.h"

ChromeTextButton::ChromeTextButton()
{
    setClickingTogglesState (true);
}

void ChromeTextButton::paintButton (juce::Graphics& g,
                                    const bool shouldDrawButtonAsHighlighted,
                                    const bool shouldDrawButtonAsDown)
{
    auto bounds = getLocalBounds().toFloat().reduced (0.5f);
    const auto isOn = getToggleState();

    if (shouldDrawButtonAsDown)
        bounds = bounds.translated (0.0f, 0.5f);

    g.setColour (HydraPalette::colour (isOn ? HydraPalette::panelRaisedEdge : HydraPalette::panelRaised));
    g.fillRoundedRectangle (bounds, 3.0f);

    if (isOn)
    {
        g.setColour (HydraPalette::colour (HydraPalette::accentGold).withAlpha (0.18f));
        g.fillRoundedRectangle (bounds, 3.0f);
    }

    g.setColour (HydraPalette::colour (shouldDrawButtonAsHighlighted ? HydraPalette::accentGoldDim
                                                                     : HydraPalette::borderMuted));
    g.drawRoundedRectangle (bounds, 3.0f, 1.0f);

    g.setColour (HydraPalette::colour (isOn ? HydraPalette::accentGoldBright : HydraPalette::textMuted));
    g.setFont (juce::Font (juce::FontOptions { 10.0f, juce::Font::bold }));
    g.drawText (getButtonText(), bounds.toNearestInt(), juce::Justification::centred, false);
}
