#include "HydraPanelBackgroundLayout.h"

void HydraPanelBackgroundLayout::paintBackground (juce::Graphics& g, const juce::Rectangle<int> bounds)
{
    juce::ColourGradient gradient (HydraPalette::colour (HydraPalette::backgroundTop), 0.0f, static_cast<float> (bounds.getY()),
                                   HydraPalette::colour (HydraPalette::backgroundBottom), 0.0f, static_cast<float> (bounds.getBottom()), false);
    g.setGradientFill (gradient);
    g.fillRect (bounds);
}

void HydraPanelBackgroundLayout::paintHeader (juce::Graphics& g, const juce::Rectangle<int> headerBounds, const juce::String& title)
{
    g.setColour (HydraPalette::colour (HydraPalette::accentGoldBright).withAlpha (0.92f));
    g.setFont (juce::Font (22.0f, juce::Font::bold));
    g.drawText (title, headerBounds.reduced (16, 0), juce::Justification::centredLeft, true);

    const auto jewelRadius = 7.0f;
    const auto jewelCentre = juce::Point<float> (static_cast<float> (headerBounds.getRight()) - 28.0f,
                                                 static_cast<float> (headerBounds.getCentreY()));

    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillEllipse (jewelCentre.x - jewelRadius - 1.0f, jewelCentre.y - jewelRadius + 1.0f, jewelRadius * 2.0f, jewelRadius * 2.0f);

    g.setColour (HydraPalette::colour (HydraPalette::accentGold).withAlpha (0.75f));
    g.fillEllipse (jewelCentre.x - jewelRadius, jewelCentre.y - jewelRadius, jewelRadius * 2.0f, jewelRadius * 2.0f);

    g.setColour (juce::Colours::white.withAlpha (0.28f));
    g.fillEllipse (jewelCentre.x - (jewelRadius * 0.4f), jewelCentre.y - (jewelRadius * 0.6f),
                   jewelRadius * 0.75f, jewelRadius * 0.75f);
}

void HydraPanelBackgroundLayout::paintZoneCard (juce::Graphics& g, const juce::Rectangle<float> bounds, const juce::String& zoneTitle)
{
    auto card = bounds.reduced (0.5f);

    g.setColour (HydraPalette::colour (HydraPalette::panelRaised));
    g.fillRoundedRectangle (card, 6.0f);

    g.setColour (HydraPalette::colour (HydraPalette::panelRaisedEdge));
    g.drawRoundedRectangle (card.reduced (0.5f), 6.0f, 1.0f);

    g.setColour (HydraPalette::colour (HydraPalette::borderMuted).withAlpha (0.55f));
    g.drawRoundedRectangle (card, 6.0f, 0.5f);

    const auto titleArea = card.removeFromTop (22.0f).reduced (10.0f, 2.0f);
    g.setColour (HydraPalette::colour (HydraPalette::accentGold).withAlpha (0.88f));
    g.setFont (juce::Font (11.0f, juce::Font::bold));
    g.drawText (zoneTitle, titleArea.toNearestInt(), juce::Justification::centredLeft, true);

    g.setColour (HydraPalette::colour (HydraPalette::borderMuted).withAlpha (0.35f));
    g.drawHorizontalLine (juce::roundToInt (card.getY()) + 1, card.getX() + 8.0f, card.getRight() - 8.0f);
}

std::array<juce::Rectangle<int>, HydraPanelBackgroundLayout::zoneCount>
HydraPanelBackgroundLayout::layoutZoneColumns (const juce::Rectangle<int> contentArea, const int columnGap)
{
    std::array<juce::Rectangle<int>, zoneCount> zones {};
    const auto totalGap = columnGap * (zoneCount - 1);
    const auto columnWidth = (contentArea.getWidth() - totalGap) / zoneCount;

    auto x = contentArea.getX();
    for (auto& zone : zones)
    {
        zone = { x, contentArea.getY(), columnWidth, contentArea.getHeight() };
        x += columnWidth + columnGap;
    }

    return zones;
}
