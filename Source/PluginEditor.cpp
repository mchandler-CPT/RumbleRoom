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
constexpr int kControlPanelHeight = kModuleTitleHeight + (2 * kKnobCellHeight) + kModuleBottomInset;
constexpr int kHeaderChromeHeight = BoutiqueLayout::kZoneGap + BoutiqueLayout::kHeaderHeight + BoutiqueLayout::kZoneGap;
constexpr int kEditorHeight = kHeaderChromeHeight + kControlPanelHeight + (2 * BoutiqueLayout::kPanelHorizontalMargin);

const juce::StringArray& subdivisionLabels()
{
    static const juce::StringArray labels { "8/1", "4/1", "2/1", "1/1", "1/2", "1/2T", "1/4", "1/4T",
                                            "1/8", "1/8T", "1/16", "1/16T", "1/32", "1/64" };
    return labels;
}
} // namespace

RumbleRoomAudioProcessorEditor::RumbleRoomAudioProcessorEditor (RumbleRoomAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (kEditorWidth, kEditorHeight);

    if (auto svgXml = juce::parseXML (juce::String::createStringFromData (RumbleRoomAssets::logo_svg,
                                                                           RumbleRoomAssets::logo_svgSize)))
        mLogoDrawable = juce::Drawable::createFromSVG (*svgXml);

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
    g.fillAll (HydraPalette::colour (HydraPalette::backgroundBase));

    auto bounds = getLocalBounds();

    bounds.removeFromTop (BoutiqueLayout::kZoneGap);
    auto headerArea = bounds.removeFromTop (BoutiqueLayout::kHeaderHeight);
    bounds.removeFromTop (BoutiqueLayout::kZoneGap);

    g.setColour (HydraPalette::colour (HydraPalette::backgroundTop));
    g.fillRect (headerArea);

    g.setColour (HydraPalette::colour (HydraPalette::borderMuted));
    g.drawHorizontalLine (headerArea.getBottom() - 1, 0.0f, static_cast<float> (getWidth()));

    if (mLogoDrawable != nullptr)
    {
        constexpr int logoVerticalPad = 6;
        const auto maxLogoHeight = headerArea.getHeight() - (2 * logoVerticalPad);
        const auto drawableBounds = mLogoDrawable->getDrawableBounds();
        const auto aspect = drawableBounds.getWidth() / drawableBounds.getHeight();
        const auto logoWidth = juce::roundToInt (static_cast<float> (maxLogoHeight) * aspect);

        const auto logoBounds = juce::Rectangle<float> (static_cast<float> (BoutiqueLayout::kPanelHorizontalMargin),
                                                        static_cast<float> (headerArea.getCentreY()) - static_cast<float> (maxLogoHeight) / 2.0f,
                                                        static_cast<float> (logoWidth),
                                                        static_cast<float> (maxLogoHeight));
        mLogoDrawable->drawWithin (g, logoBounds, juce::RectanglePlacement::centred, 1.0f);
    }

    bounds.reduce (BoutiqueLayout::kPanelHorizontalMargin, 0);

    auto masterBounds = bounds.removeFromRight (kMasterStripWidth);
    bounds.removeFromRight (kModuleGap);

    const auto moduleWidth = (bounds.getWidth() - (kModuleGap * (kModuleCount - 1))) / kModuleCount;

    static const juce::StringArray moduleTitles { "LOOP GENERATION", "SUPPRESSION", "FREQUENCY",
                                                  "REFLECTION", "DRIFT", "CHARACTER" };

    const auto drawModuleFrame = [&g] (juce::Rectangle<int> moduleBounds, const juce::String& title)
    {
        g.setColour (HydraPalette::colour (HydraPalette::panelRaised));
        g.fillRoundedRectangle (moduleBounds.toFloat(), 6.0f);

        g.setColour (HydraPalette::colour (HydraPalette::borderMuted));
        g.drawRoundedRectangle (moduleBounds.toFloat(), 6.0f, 1.0f);

        auto titleArea = moduleBounds.removeFromTop (kModuleTitleHeight).reduced (BoutiqueLayout::kZoneCardInset + 2, 0);
        g.setColour (HydraPalette::colour (HydraPalette::textMuted));
        g.setFont (juce::Font (juce::FontOptions { 9.5f, juce::Font::bold }));
        g.drawText (title, titleArea, juce::Justification::centred);
    };

    for (int i = 0; i < kModuleCount; ++i)
    {
        auto moduleBounds = bounds.removeFromLeft (moduleWidth);
        if (i < kModuleCount - 1)
            bounds.removeFromLeft (kModuleGap);

        drawModuleFrame (moduleBounds, moduleTitles[i]);
    }

    drawModuleFrame (masterBounds, "MIX");
}

void RumbleRoomAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop (BoutiqueLayout::kZoneGap);
    auto headerArea = bounds.removeFromTop (BoutiqueLayout::kHeaderHeight);
    bounds.removeFromTop (BoutiqueLayout::kZoneGap);

    layoutPresetHeader (headerArea);

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
    layoutDualKnobModule (takeModule(), mDuckDepthSlider, mDuckDepthLabel, mDuckReleaseSlider, mDuckReleaseLabel);
    layoutDualKnobModule (takeModule(), mLowPassSlider, mLowPassLabel, mHighPassSlider, mHighPassLabel);
    layoutDualKnobModule (takeModule(), mDiffusionSlider, mDiffusionLabel, mFeedbackDampSlider, mFeedbackDampLabel);
    layoutDualKnobModule (takeModule(), mWowSpeedSlider, mWowSpeedLabel, mWowDepthSlider, mWowDepthLabel);
    layoutDualKnobModule (takeModule(), mGritSlider, mGritLabel, mWidthSlider, mWidthLabel);

    masterArea.removeFromTop (kModuleTitleHeight);
    alignMixKnobCell (masterArea, mMixSlider, mMixLabel);

    mPrevButton.toFront (false);
    mNextButton.toFront (false);
    mSaveButton.toFront (false);
    mSetFolderButton.toFront (false);
    mPresetLabel.toFront (false);
    mSyncButton.toFront (false);
}
