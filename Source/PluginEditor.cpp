#include "PluginEditor.h"
#include "UI/BoutiqueLayoutConstants.h"
#include "UI/HydraPalette.h"
#include <RumbleRoomAssets.h>

namespace
{
constexpr int kEditorWidth = 1080;
constexpr int kModuleCount = 6;
constexpr int kModuleTitleHeight = 22;
constexpr int kModuleGap = 6;
constexpr int kMasterStripWidth = 124;
constexpr int kKnobCellWidth = 72;
constexpr int kKnobCellHeight = 90;
constexpr int kMixKnobCellWidth = 92;
constexpr int kMixKnobCellHeight = 115;
constexpr int kKnobLabelHeight = 14;
constexpr int kModuleBottomInset = 6;
constexpr int kFooterHeight = 30;
constexpr int kControlPanelHeight = kModuleTitleHeight + (2 * kKnobCellHeight) + kModuleBottomInset;
constexpr int kHeaderChromeHeight = BoutiqueLayout::kZoneGap + BoutiqueLayout::kHeaderHeight + BoutiqueLayout::kZoneGap;
constexpr int kEditorHeight = kHeaderChromeHeight + kControlPanelHeight + (2 * BoutiqueLayout::kPanelHorizontalMargin) + kFooterHeight;

const juce::StringArray& subdivisionLabels()
{
    static const juce::StringArray labels { "8/1", "4/1", "2/1", "1/1", "1/2", "1/2T", "1/4", "1/4T",
                                            "1/8", "1/8T", "1/16", "1/16T", "1/32", "1/64" };
    return labels;
}

void paintBackplate (juce::Graphics& g, juce::Rectangle<int> area)
{
    const auto areaF = area.toFloat();

    juce::ColourGradient base (HydraPalette::colour (HydraPalette::backgroundTop), areaF.getX(), areaF.getY(),
                               HydraPalette::colour (HydraPalette::backgroundBottom), areaF.getX(), areaF.getBottom(), false);
    g.setGradientFill (base);
    g.fillRect (area);

    // Warm glow rising from the upper-centre, like panel lighting.
    juce::ColourGradient glow (HydraPalette::colour (HydraPalette::backgroundWarmTint).withAlpha (0.40f),
                               areaF.getCentreX(), areaF.getHeight() * 0.32f,
                               juce::Colours::transparentBlack,
                               areaF.getCentreX(), areaF.getBottom(),
                               true);
    g.setGradientFill (glow);
    g.fillRect (area);

    // Faint horizontal brushed-metal striations.
    g.setColour (juce::Colours::white.withAlpha (0.012f));
    for (int y = area.getY(); y < area.getBottom(); y += 3)
        g.drawHorizontalLine (y, areaF.getX(), areaF.getRight());

    // Edge vignette to lift the panel off the chrome.
    juce::ColourGradient vignette (juce::Colours::transparentBlack,
                                   areaF.getCentreX(), areaF.getCentreY(),
                                   juce::Colours::black.withAlpha (0.55f),
                                   areaF.getX(), areaF.getBottom(),
                                   true);
    g.setGradientFill (vignette);
    g.fillRect (area);
}

void paintModuleCard (juce::Graphics& g, juce::Rectangle<int> moduleBounds, const juce::String& title)
{
    const auto cardF = moduleBounds.toFloat();

    juce::Path cardPath;
    cardPath.addRoundedRectangle (cardF, 6.0f);
    juce::DropShadow shadow (juce::Colours::black.withAlpha (0.50f), 9, { 0, 3 });
    shadow.drawForPath (g, cardPath);

    juce::ColourGradient body (HydraPalette::colour (HydraPalette::panelRaisedEdge), cardF.getX(), cardF.getY(),
                               HydraPalette::colour (HydraPalette::panelRaised), cardF.getX(), cardF.getBottom(), false);
    g.setGradientFill (body);
    g.fillRoundedRectangle (cardF, 6.0f);

    // Top sheen.
    g.setColour (juce::Colours::white.withAlpha (0.045f));
    g.drawLine (cardF.getX() + 7.0f, cardF.getY() + 1.2f, cardF.getRight() - 7.0f, cardF.getY() + 1.2f, 1.0f);

    g.setColour (HydraPalette::colour (HydraPalette::borderHighlight));
    g.drawRoundedRectangle (cardF.reduced (0.5f), 6.0f, 1.0f);

    auto titleArea = moduleBounds.removeFromTop (kModuleTitleHeight).reduced (BoutiqueLayout::kZoneCardInset + 2, 0);
    g.setColour (HydraPalette::colour (HydraPalette::textTitle));
    g.setFont (juce::Font (juce::FontOptions { 9.5f, juce::Font::bold }));
    g.drawText (title, titleArea, juce::Justification::centred);

    // Plain divider beneath the title.
    const auto dividerY = static_cast<float> (moduleBounds.getY());
    g.setColour (HydraPalette::colour (HydraPalette::borderMuted).withAlpha (0.85f));
    g.fillRect (cardF.getX() + 8.0f, dividerY - 1.0f, cardF.getWidth() - 16.0f, 1.0f);
}
} // namespace

RumbleRoomAudioProcessorEditor::RumbleRoomAudioProcessorEditor (RumbleRoomAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (kEditorWidth, kEditorHeight);

    if (auto svgXml = juce::parseXML (juce::String::createStringFromData (RumbleRoomAssets::logo_svg,
                                                                           RumbleRoomAssets::logo_svgSize)))
        mLogoDrawable = juce::Drawable::createFromSVG (*svgXml);
    if (auto bdSvgXml = juce::parseXML (juce::String::createStringFromData (RumbleRoomAssets::bdEnergyLogo_svg,
                                                                             RumbleRoomAssets::bdEnergyLogo_svgSize)))
        mBdEnergyLogoDrawable = juce::Drawable::createFromSVG (*bdSvgXml);

    audioProcessor.apvts.addParameterListener ("sync", this);

    mSyncButton.setButtonText ("s");
    mSyncButton.setTooltip ("Tempo Sync");
    addAndMakeVisible (mSyncButton);
    mSyncAttachment = std::make_unique<ButtonAttachment> (audioProcessor.apvts, "sync", mSyncButton);
    mSyncButton.onClick = [this]
    {
        updateDelayTimeControlMode();
        resized();
    };

    mPrevButton.setButtonText ("<");
    mNextButton.setButtonText (">");
    mSaveButton.setButtonText ("SAVE");
    mSetFolderButton.setButtonText ("...");
    mPresetLabel.setJustificationType (juce::Justification::centred);
    mPresetLabel.setColour (juce::Label::textColourId, HydraPalette::colour (HydraPalette::accentGoldBright));

    addAndMakeVisible (mPrevButton);
    addAndMakeVisible (mNextButton);
    addAndMakeVisible (mSaveButton);
    addAndMakeVisible (mSetFolderButton);
    addAndMakeVisible (mPresetLabel);

    mPrevButton.onClick = [this]
    {
        audioProcessor.getPresetManager().loadPreviousPreset();
        refreshPresetUi();
    };

    mNextButton.onClick = [this]
    {
        audioProcessor.getPresetManager().loadNextPreset();
        refreshPresetUi();
    };

    mSaveButton.onClick = [this]
    {
        auto& pm = audioProcessor.getPresetManager();
        const auto currentName = pm.getCurrentPresetName().isEmpty() ? "New Preset" : pm.getCurrentPresetName();

        mPresetSaveChooser = std::make_unique<juce::FileChooser> (
            "Save RumbleRoom Preset",
            pm.getCurrentPresetDirectory().getChildFile (currentName).withFileExtension (".rr"),
            "*.rr");

        const auto flags = juce::FileBrowserComponent::saveMode
                         | juce::FileBrowserComponent::canSelectFiles
                         | juce::FileBrowserComponent::warnAboutOverwriting;

        mPresetSaveChooser->launchAsync (flags, [this] (const juce::FileChooser& chooser)
        {
            const juce::File target = chooser.getResult();
            if (target != juce::File{})
            {
                audioProcessor.getPresetManager().savePreset (target.getFileNameWithoutExtension());
                refreshPresetUi();
            }

            mPresetSaveChooser.reset();
        });
    };

    mSetFolderButton.onClick = [this]
    {
        auto& pm = audioProcessor.getPresetManager();

        mPresetFolderChooser = std::make_unique<juce::FileChooser> (
            "Select Preset Folder",
            pm.getCurrentPresetDirectory());

        const auto flags = juce::FileBrowserComponent::openMode
                         | juce::FileBrowserComponent::canSelectDirectories;

        mPresetFolderChooser->launchAsync (flags, [this] (const juce::FileChooser& chooser)
        {
            const juce::File target = chooser.getResult();
            if (target != juce::File{})
            {
                audioProcessor.getPresetManager().setCustomPresetDirectory (target);
                refreshPresetUi();
            }

            mPresetFolderChooser.reset();
        });
    };

    configureRotaryKnob (mDelayTimeSlider, mDelayTimeLabel, "Delay Time", KnobReadoutKind::timeMilliseconds, customLookAndFeel, *this);

    configureRotaryKnob (mFeedbackSlider, mFeedbackLabel, "Feedback", KnobReadoutKind::percent, customLookAndFeel, *this);
    mFeedbackAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "feedback", mFeedbackSlider);

    configureRotaryKnob (mDuckDepthSlider, mDuckDepthLabel, "Suppress Depth", KnobReadoutKind::percent, customLookAndFeel, *this);
    mDuckDepthAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "duckDepth", mDuckDepthSlider);

    configureRotaryKnob (mDuckReleaseSlider, mDuckReleaseLabel, "Suppress Release", KnobReadoutKind::timeSeconds, customLookAndFeel, *this);
    mDuckReleaseAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "release", mDuckReleaseSlider);

    configureRotaryKnob (mLowPassSlider, mLowPassLabel, "Low Pass", KnobReadoutKind::hertz, customLookAndFeel, *this, " Hz");
    mLowPassAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "cutoff", mLowPassSlider);

    configureRotaryKnob (mHighPassSlider, mHighPassLabel, "High Pass", KnobReadoutKind::hertz, customLookAndFeel, *this, " Hz");
    mHighPassAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "hpCutoff", mHighPassSlider);

    configureRotaryKnob (mDiffusionSlider, mDiffusionLabel, "Diffusion", KnobReadoutKind::percent, customLookAndFeel, *this);
    mDiffusionAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "diffusion", mDiffusionSlider);

    configureRotaryKnob (mFeedbackDampSlider, mFeedbackDampLabel, "Feedback Damp", KnobReadoutKind::hertz, customLookAndFeel, *this, " Hz");
    mFeedbackDampAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "damping", mFeedbackDampSlider);

    configureRotaryKnob (mWowSpeedSlider, mWowSpeedLabel, "Drift Rate", KnobReadoutKind::hertz, customLookAndFeel, *this, " Hz");
    mWowSpeedAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "wowSpeed", mWowSpeedSlider);

    configureRotaryKnob (mWowDepthSlider, mWowDepthLabel, "Drift Depth", KnobReadoutKind::percent, customLookAndFeel, *this);
    mWowDepthAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "wowDepth", mWowDepthSlider);

    configureRotaryKnob (mGritSlider, mGritLabel, "Drive", KnobReadoutKind::percent, customLookAndFeel, *this);
    mGritAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "grit", mGritSlider);

    configureRotaryKnob (mWidthSlider, mWidthLabel, "Stereo Width", KnobReadoutKind::percent, customLookAndFeel, *this);
    mWidthAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "width", mWidthSlider);

    configureRotaryKnob (mMixSlider, mMixLabel, "Mix", KnobReadoutKind::percent, customLookAndFeel, *this);
    mMixAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "dryWet", mMixSlider);

    updateDelayTimeControlMode();
    refreshPresetUi();
}

RumbleRoomAudioProcessorEditor::~RumbleRoomAudioProcessorEditor()
{
    audioProcessor.apvts.removeParameterListener ("sync", this);

    mDelayTimeSlider.setLookAndFeel (nullptr);
    mFeedbackSlider.setLookAndFeel (nullptr);
    mDuckDepthSlider.setLookAndFeel (nullptr);
    mDuckReleaseSlider.setLookAndFeel (nullptr);
    mLowPassSlider.setLookAndFeel (nullptr);
    mHighPassSlider.setLookAndFeel (nullptr);
    mDiffusionSlider.setLookAndFeel (nullptr);
    mFeedbackDampSlider.setLookAndFeel (nullptr);
    mWowSpeedSlider.setLookAndFeel (nullptr);
    mWowDepthSlider.setLookAndFeel (nullptr);
    mGritSlider.setLookAndFeel (nullptr);
    mWidthSlider.setLookAndFeel (nullptr);
    mMixSlider.setLookAndFeel (nullptr);
}

void RumbleRoomAudioProcessorEditor::refreshDelayTimeReadout()
{
    mDelayTimeSlider.textFromValueFunction = [this] (double value)
    {
        const auto syncValue = audioProcessor.apvts.getRawParameterValue ("sync");
        const auto syncEnabled = (syncValue != nullptr && syncValue->load() > 0.5f);

        if (syncEnabled)
        {
            const auto& labels = subdivisionLabels();
            return labels[juce::jlimit (0, labels.size() - 1, juce::roundToInt (value))];
        }

        return formatMillisecondsReadout (value);
    };

    mDelayTimeSlider.valueFromTextFunction = [this] (const juce::String& text)
    {
        const auto syncValue = audioProcessor.apvts.getRawParameterValue ("sync");
        const auto syncEnabled = (syncValue != nullptr && syncValue->load() > 0.5f);

        if (syncEnabled)
        {
            const auto idx = subdivisionLabels().indexOf (text.trim());
            return idx >= 0 ? static_cast<double> (idx) : 6.0;
        }

        auto trimmed = text.trim();
        if (trimmed.endsWithIgnoreCase ("ms"))
            return trimmed.dropLastCharacters (2).trim().getDoubleValue();

        if (trimmed.endsWithIgnoreCase ("s"))
            return trimmed.dropLastCharacters (1).trim().getDoubleValue() * 1000.0;

        return trimmed.getDoubleValue();
    };

    mDelayTimeSlider.updateText();
}

void RumbleRoomAudioProcessorEditor::updateDelayTimeControlMode()
{
    const auto syncValue = audioProcessor.apvts.getRawParameterValue ("sync");
    const auto syncEnabled = (syncValue != nullptr && syncValue->load() > 0.5f);

    if (syncEnabled == mDelayUsingSync && (mDelayTimeAttachment != nullptr || mSubdivisionAttachment != nullptr))
    {
        refreshDelayTimeReadout();
        return;
    }

    mDelayTimeAttachment.reset();
    mSubdivisionAttachment.reset();

    if (syncEnabled)
    {
        mDelayTimeSlider.setRange (0.0, 13.0, 1.0);
        mDelayTimeSlider.setSkewFactor (1.0);
        mDelayTimeSlider.setNumDecimalPlacesToDisplay (0);
        mSubdivisionAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "subdivision", mDelayTimeSlider);
    }
    else
    {
        mDelayTimeSlider.setRange (0.1, 8000.0, 0.01);
        mDelayTimeSlider.setSkewFactor (0.28);
        applyKnobReadout (mDelayTimeSlider, KnobReadoutKind::timeMilliseconds);
        mDelayTimeAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "delayTime", mDelayTimeSlider);
    }

    mDelayUsingSync = syncEnabled;
    refreshDelayTimeReadout();
}

void RumbleRoomAudioProcessorEditor::refreshPresetUi()
{
    auto& pm = audioProcessor.getPresetManager();
    pm.updatePresetList();

    const bool hasPresets = ! pm.getPresetNames().isEmpty();
    mPrevButton.setEnabled (hasPresets);
    mNextButton.setEnabled (hasPresets);

    mPresetLabel.setFont (juce::Font (juce::FontOptions { 14.0f, juce::Font::bold }));
    mPresetLabel.setText (pm.getCurrentPresetName(), juce::dontSendNotification);
}

void RumbleRoomAudioProcessorEditor::layoutPresetHeader (juce::Rectangle<int> headerArea)
{
    // Width and layout definitions matching the hardware spec
    constexpr int pNavWidth = 26;
    constexpr int pActionWidth = 52;
    constexpr int pFolderWidth = 30;
    constexpr int pGap = 8;
    constexpr int pInnerGap = 4;

    auto row = headerArea.reduced (8, 4);
    const auto actionY = row.getY() + ((row.getHeight() - 22) / 2);

    auto actionCluster = row.removeFromRight (pActionWidth + pGap + pFolderWidth);
    mSetFolderButton.setBounds (actionCluster.removeFromRight (pFolderWidth).withY (actionY).withHeight (22));
    actionCluster.removeFromRight (pGap);
    mSaveButton.setBounds (actionCluster.removeFromRight (pActionWidth).withY (actionY).withHeight (22));

    // Center layout split parameters
    const auto centerAreaX = (getWidth() - 180) / 2;
    mPrevButton.setBounds (centerAreaX, actionY, pNavWidth, 22);
    mNextButton.setBounds (centerAreaX + 180 - pNavWidth, actionY, pNavWidth, 22);

    const auto labelLeft = mPrevButton.getRight() + pInnerGap;
    const auto labelRight = mNextButton.getX() - pInnerGap;
    mPresetLabel.setBounds (labelLeft, row.getY(), juce::jmax (0, labelRight - labelLeft), row.getHeight());
}

void RumbleRoomAudioProcessorEditor::parameterChanged (const juce::String& parameterID, float)
{
    if (parameterID == "sync")
    {
        updateDelayTimeControlMode();
        resized();
    }
}

void RumbleRoomAudioProcessorEditor::alignKnobCell (juce::Rectangle<int> cell, juce::Slider& slider, juce::Label& label)
{
    const auto stackY = cell.getY() + ((cell.getHeight() - kKnobCellHeight) / 2);
    label.setBounds (cell.getX(), stackY, cell.getWidth(), kKnobLabelHeight);

    slider.setBounds (cell.getCentreX() - (kKnobCellWidth / 2),
                      stackY + kKnobLabelHeight,
                      kKnobCellWidth,
                      kKnobCellHeight - kKnobLabelHeight);
}

void RumbleRoomAudioProcessorEditor::alignMixKnobCell (juce::Rectangle<int> cell, juce::Slider& slider, juce::Label& label)
{
    const auto stackY = cell.getY() + ((cell.getHeight() - kMixKnobCellHeight) / 2);
    label.setBounds (cell.getX(), stackY, cell.getWidth(), kKnobLabelHeight);

    slider.setBounds (cell.getCentreX() - (kMixKnobCellWidth / 2),
                      stackY + kKnobLabelHeight,
                      kMixKnobCellWidth,
                      kMixKnobCellHeight - kKnobLabelHeight);
}

void RumbleRoomAudioProcessorEditor::layoutDualKnobModule (juce::Rectangle<int> module,
                                                               juce::Slider& topSlider,
                                                               juce::Label& topLabel,
                                                               juce::Slider& bottomSlider,
                                                               juce::Label& bottomLabel)
{
    module.removeFromTop (kModuleTitleHeight);
    const auto halfH = module.getHeight() / 2;

    alignKnobCell ({ module.getX(), module.getY(), module.getWidth(), halfH }, topSlider, topLabel);
    alignKnobCell ({ module.getX(), module.getY() + halfH, module.getWidth(), halfH }, bottomSlider, bottomLabel);
}

void RumbleRoomAudioProcessorEditor::paint (juce::Graphics& g)
{
    paintBackplate (g, getLocalBounds());

    auto bounds = getLocalBounds();

    bounds.removeFromTop (BoutiqueLayout::kZoneGap);
    auto headerArea = bounds.removeFromTop (BoutiqueLayout::kHeaderHeight);
    bounds.removeFromTop (BoutiqueLayout::kZoneGap);

    auto footerArea = bounds.removeFromBottom (kFooterHeight);

    juce::ColourGradient headerFill (HydraPalette::colour (HydraPalette::backgroundTop), 0.0f, static_cast<float> (headerArea.getY()),
                                     HydraPalette::colour (HydraPalette::backgroundBase), 0.0f, static_cast<float> (headerArea.getBottom()), false);
    g.setGradientFill (headerFill);
    g.fillRect (headerArea);

    g.setColour (HydraPalette::colour (HydraPalette::accentGold).withAlpha (0.30f));
    g.drawHorizontalLine (headerArea.getBottom() - 1, 0.0f, static_cast<float> (getWidth()));

    constexpr int logoVerticalPad = 6;
    constexpr int mainLogoRightShift = 5;
    const auto maxLogoHeight = headerArea.getHeight() - (2 * logoVerticalPad);
    const auto logoTop = static_cast<float> (headerArea.getCentreY()) - static_cast<float> (maxLogoHeight) / 2.0f;
    const float drawX = static_cast<float> (BoutiqueLayout::kPanelHorizontalMargin + mainLogoRightShift);

    if (mLogoDrawable != nullptr)
    {
        const auto drawableBounds = mLogoDrawable->getDrawableBounds();
        const auto aspect = drawableBounds.getWidth() / drawableBounds.getHeight();
        const auto logoWidth = juce::roundToInt (static_cast<float> (maxLogoHeight) * aspect);
        const auto logoBounds = juce::Rectangle<float> (drawX, logoTop, static_cast<float> (logoWidth), static_cast<float> (maxLogoHeight));
        mLogoDrawable->drawWithin (g, logoBounds, juce::RectanglePlacement::centred, 1.0f);
    }

    bounds.reduce (BoutiqueLayout::kPanelHorizontalMargin, 0);

    auto masterBounds = bounds.removeFromRight (kMasterStripWidth);
    bounds.removeFromRight (kModuleGap);

    const auto moduleWidth = (bounds.getWidth() - (kModuleGap * (kModuleCount - 1))) / kModuleCount;

    static const juce::StringArray moduleTitles { "LOOP GENERATION", "FREQUENCY", "CHARACTER",
                                                  "REFLECTION", "DRIFT", "SUPPRESSION" };

    for (int i = 0; i < kModuleCount; ++i)
    {
        auto moduleBounds = bounds.removeFromLeft (moduleWidth);
        if (i < kModuleCount - 1)
            bounds.removeFromLeft (kModuleGap);

        paintModuleCard (g, moduleBounds, moduleTitles[i]);
    }

    paintModuleCard (g, masterBounds, "MIX");

    // Footer: product identity on the left, bdEnergy mark on the right.
    g.setColour (HydraPalette::colour (HydraPalette::borderMuted).withAlpha (0.6f));
    g.drawHorizontalLine (footerArea.getY(), static_cast<float> (BoutiqueLayout::kPanelHorizontalMargin),
                          static_cast<float> (getWidth() - BoutiqueLayout::kPanelHorizontalMargin));

    auto footerRow = footerArea.reduced (BoutiqueLayout::kPanelHorizontalMargin, 0);

    g.setFont (juce::Font (juce::FontOptions { 11.0f, juce::Font::plain }));
    g.setColour (HydraPalette::colour (HydraPalette::textFooter));
    g.drawText ("RUMBLE ROOM  //  BOUTIQUE DELAY", footerRow.getX(), footerRow.getY(), 320, footerRow.getHeight(),
                juce::Justification::centredLeft, false);

    if (mBdEnergyLogoDrawable != nullptr)
    {
        constexpr int kFooterLogoHeight = 18;
        const auto drawableBounds = mBdEnergyLogoDrawable->getDrawableBounds();
        const auto aspect = drawableBounds.getWidth() / drawableBounds.getHeight();
        const auto logoWidth = juce::roundToInt (static_cast<float> (kFooterLogoHeight) * aspect);
        const auto logoBounds = juce::Rectangle<float> (static_cast<float> (footerRow.getRight() - logoWidth),
                                                        static_cast<float> (footerRow.getCentreY() - kFooterLogoHeight / 2),
                                                        static_cast<float> (logoWidth),
                                                        static_cast<float> (kFooterLogoHeight));
        mBdEnergyLogoDrawable->drawWithin (g, logoBounds, juce::RectanglePlacement::centred, 1.0f);
    }
}

void RumbleRoomAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop (BoutiqueLayout::kZoneGap);
    auto headerArea = bounds.removeFromTop (BoutiqueLayout::kHeaderHeight);
    bounds.removeFromTop (BoutiqueLayout::kZoneGap);

    layoutPresetHeader (headerArea);

    bounds.removeFromBottom (kFooterHeight);
    bounds.reduce (BoutiqueLayout::kPanelHorizontalMargin, 0);

    auto masterArea = bounds.removeFromRight (kMasterStripWidth);
    bounds.removeFromRight (kModuleGap);

    const auto moduleWidth = (bounds.getWidth() - (kModuleGap * (kModuleCount - 1))) / kModuleCount;

    auto takeModule = [&bounds, moduleWidth]() -> juce::Rectangle<int>
    {
        auto module = bounds.removeFromLeft (moduleWidth);
        bounds.removeFromLeft (kModuleGap);
        return module;
    };

    auto loopGenModule = takeModule();
    layoutDualKnobModule (loopGenModule, mDelayTimeSlider, mDelayTimeLabel, mFeedbackSlider, mFeedbackLabel);
    {
        constexpr int kSyncButtonSize = 18;
        constexpr int kSyncButtonLeftNudge = 12;
        const auto delayKnobBounds = mDelayTimeSlider.getBounds();
        const int gapStart = delayKnobBounds.getRight();
        const int gapEnd = loopGenModule.getRight();
        const int syncCentreX = gapStart + ((gapEnd - gapStart) / 2) - kSyncButtonLeftNudge;
        const int syncCentreY = delayKnobBounds.getCentreY() - (BoutiqueLayout::kCutoffTextBoxHeight / 2);

        mSyncButton.setBounds (syncCentreX - (kSyncButtonSize / 2),
                               syncCentreY - (kSyncButtonSize / 2),
                               kSyncButtonSize,
                               kSyncButtonSize);
    }
    layoutDualKnobModule (takeModule(), mLowPassSlider, mLowPassLabel, mHighPassSlider, mHighPassLabel);
    layoutDualKnobModule (takeModule(), mGritSlider, mGritLabel, mWidthSlider, mWidthLabel);
    layoutDualKnobModule (takeModule(), mDiffusionSlider, mDiffusionLabel, mFeedbackDampSlider, mFeedbackDampLabel);
    layoutDualKnobModule (takeModule(), mWowSpeedSlider, mWowSpeedLabel, mWowDepthSlider, mWowDepthLabel);
    layoutDualKnobModule (takeModule(), mDuckDepthSlider, mDuckDepthLabel, mDuckReleaseSlider, mDuckReleaseLabel);

    masterArea.removeFromTop (kModuleTitleHeight);
    alignMixKnobCell (masterArea, mMixSlider, mMixLabel);

    mPrevButton.toFront (false);
    mNextButton.toFront (false);
    mSaveButton.toFront (false);
    mSetFolderButton.toFront (false);
    mPresetLabel.toFront (false);
    mSyncButton.toFront (false);
}
