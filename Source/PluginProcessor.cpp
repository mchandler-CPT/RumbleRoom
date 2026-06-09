#include "PluginProcessor.h"
#include "PluginEditor.h"

RumbleRoomAudioProcessor::RumbleRoomAudioProcessor()
    : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    mDelayTimeParam = apvts.getRawParameterValue ("delayTime");
    mFeedbackParam  = apvts.getRawParameterValue ("feedback");
    mDryWetParam    = apvts.getRawParameterValue ("dryWet");
    mGritParam      = apvts.getRawParameterValue ("grit");
    mWidthParam     = apvts.getRawParameterValue ("width");
    mCutoffParam    = apvts.getRawParameterValue ("cutoff");
    mHpCutoffParam  = apvts.getRawParameterValue ("hpCutoff");
    mReleaseParam   = apvts.getRawParameterValue ("release");
    mSyncParam      = apvts.getRawParameterValue ("sync");
    mSubdivisionParam = apvts.getRawParameterValue ("subdivision");
    mBpmParam       = apvts.getRawParameterValue ("bpm");
    mWowDepthParam  = apvts.getRawParameterValue ("wowDepth");
    mDiffusionParam = apvts.getRawParameterValue ("diffusion");
}

const juce::String RumbleRoomAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool RumbleRoomAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool RumbleRoomAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool RumbleRoomAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double RumbleRoomAudioProcessor::getTailLengthSeconds() const
{
    return 2.0;
}

int RumbleRoomAudioProcessor::getNumPrograms()
{
    return 1;
}

int RumbleRoomAudioProcessor::getCurrentProgram()
{
    return 0;
}

void RumbleRoomAudioProcessor::setCurrentProgram (int)
{
}

const juce::String RumbleRoomAudioProcessor::getProgramName (int)
{
    return {};
}

void RumbleRoomAudioProcessor::changeProgramName (int, const juce::String&)
{
}

void RumbleRoomAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const auto channelCount = juce::jmax (1, getTotalNumInputChannels(), getTotalNumOutputChannels());
    const auto delayBufferSize = juce::jmax (1, static_cast<int> (sampleRate * 2.0));

    mDelayBuffer.setSize (channelCount, delayBufferSize, false, false, true);
    mDelayBuffer.clear();
    mWritePosition = 0;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (juce::jmax (1, samplesPerBlock));
    spec.numChannels = static_cast<juce::uint32> (channelCount);

    mFilter.reset();
    mFilter.prepare (spec);
    mFilter.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    mHPFilter.reset();
    mHPFilter.prepare (spec);
    mHPFilter.setType (juce::dsp::StateVariableTPTFilterType::highpass);

    const auto initialCutoff = juce::jlimit (20.0f, 20000.0f, mCutoffParam != nullptr ? mCutoffParam->load() : 1380.0f);
    const auto initialHpCutoff = juce::jlimit (20.0f, 8000.0f, mHpCutoffParam != nullptr ? mHpCutoffParam->load() : 35.0f);
    const auto initialDelayMs = juce::jlimit (0.1f, 1000.0f, mDelayTimeParam != nullptr ? mDelayTimeParam->load() : 20.0f);
    const auto initialFeedback = juce::jlimit (0.0f, 0.99f, mFeedbackParam != nullptr ? mFeedbackParam->load() : 0.25f);
    const auto initialDryWet = juce::jlimit (0.0f, 1.0f, mDryWetParam != nullptr ? mDryWetParam->load() : 0.26f);
    const auto initialGrit = juce::jlimit (0.0f, 1.0f, mGritParam != nullptr ? mGritParam->load() : 0.41f);
    const auto initialWidth = juce::jlimit (0.0f, 1.0f, mWidthParam != nullptr ? mWidthParam->load() : 0.0f);
    const auto initialRelease = juce::jlimit (0.1f, 2.0f, mReleaseParam != nullptr ? mReleaseParam->load() : 0.5f);
    mSmoothedDelayTimeMs.reset (sampleRate, 0.04);
    mSmoothedDelayTimeMs.setCurrentAndTargetValue (initialDelayMs);
    mSmoothedFeedback.reset (sampleRate, 0.04);
    mSmoothedFeedback.setCurrentAndTargetValue (initialFeedback);
    mSmoothedDryWet.reset (sampleRate, 0.04);
    mSmoothedDryWet.setCurrentAndTargetValue (initialDryWet);
    mSmoothedGrit.reset (sampleRate, 0.06);
    mSmoothedGrit.setCurrentAndTargetValue (initialGrit);
    mSmoothedWidth.reset (sampleRate, 0.04);
    mSmoothedWidth.setCurrentAndTargetValue (initialWidth);
    mSmoothedRelease.reset (sampleRate, 0.04);
    mSmoothedRelease.setCurrentAndTargetValue (initialRelease);
    mSmoothedCutoff.reset (sampleRate, 0.04);
    mSmoothedCutoff.setCurrentAndTargetValue (initialCutoff);
    mSmoothedHpCutoff.reset (sampleRate, 0.04);
    mSmoothedHpCutoff.setCurrentAndTargetValue (initialHpCutoff);
    mFilter.setCutoffFrequency (initialCutoff);
    mHPFilter.setCutoffFrequency (initialHpCutoff);
    mEnvelopeLevel = 0.0f;
    mJitterAmount = 0.0f;
    mModPhase = 0.0f;

    for (int i = 0; i < kNumDiffStages; ++i)
    {
        mDiffBuffersL[i].assign (static_cast<size_t> (mDiffLengths[i]), 0.0f);
        mDiffBuffersR[i].assign (static_cast<size_t> (mDiffLengths[i]), 0.0f);
        mDiffWritePositionsL[i] = 0;
        mDiffWritePositionsR[i] = 0;
    }
}

void RumbleRoomAudioProcessor::releaseResources()
{
}

#if ! JucePlugin_IsMidiEffect
bool RumbleRoomAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsSynth
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void RumbleRoomAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    juce::ScopedNoDenormals noDenormals;
    const auto totalInputChannels = getTotalNumInputChannels();
    const auto totalOutputChannels = getTotalNumOutputChannels();
    const auto numSamples = buffer.getNumSamples();
    const auto delayBufferLength = mDelayBuffer.getNumSamples();
    const auto delayChannels = mDelayBuffer.getNumChannels();
    const auto sampleRate = static_cast<float> (getSampleRate());

    for (auto channel = totalInputChannels; channel < totalOutputChannels; ++channel)
        buffer.clear (channel, 0, numSamples);

    if (delayBufferLength <= 1 || sampleRate <= 0.0f)
        return;

    const auto feedbackTarget = juce::jlimit (0.0f, 0.99f, mFeedbackParam != nullptr ? mFeedbackParam->load() : 0.25f);
    const auto dryWetTarget = juce::jlimit (0.0f, 1.0f, mDryWetParam != nullptr ? mDryWetParam->load() : 0.26f);
    const auto gritTarget = juce::jlimit (0.0f, 1.0f, mGritParam != nullptr ? mGritParam->load() : 0.41f);
    const auto releaseTarget = juce::jlimit (0.1f, 2.0f, mReleaseParam != nullptr ? mReleaseParam->load() : 0.5f);
    const auto cutoff = juce::jlimit (20.0f, 20000.0f, mCutoffParam != nullptr ? mCutoffParam->load() : 1380.0f);
    const auto hpCutoff = juce::jlimit (20.0f, 8000.0f, mHpCutoffParam != nullptr ? mHpCutoffParam->load() : 35.0f);
    auto delayTargetMs = juce::jlimit (0.1f, 1000.0f, mDelayTimeParam != nullptr ? mDelayTimeParam->load() : 20.0f);
    const auto syncEnabled = (mSyncParam != nullptr && mSyncParam->load() > 0.5f);

    if (syncEnabled)
    {
        constexpr float subdivisionMultipliers[] { 4.0f, 2.0f, 4.0f / 3.0f, 1.0f, 2.0f / 3.0f,
                                                   0.5f, 1.0f / 3.0f, 0.25f, 1.0f / 6.0f, 0.125f, 0.0625f };

        float bpm = juce::jlimit (40.0f, 260.0f, mBpmParam != nullptr ? mBpmParam->load() : 120.0f);
        if (auto* playHead = getPlayHead())
        {
            if (const auto position = playHead->getPosition())
            {
                if (const auto hostBpm = position->getBpm(); hostBpm.hasValue())
                    bpm = static_cast<float> (*hostBpm);
            }
        }

        const auto subdivisionIndex = juce::jlimit (0, 10, juce::roundToInt (mSubdivisionParam != nullptr ? mSubdivisionParam->load() : 3.0f));
        delayTargetMs = (60000.0f / juce::jmax (1.0f, bpm)) * subdivisionMultipliers[subdivisionIndex];
        delayTargetMs = juce::jlimit (0.1f, 1000.0f, delayTargetMs);
    }

    mSmoothedDelayTimeMs.setTargetValue (delayTargetMs);
    mSmoothedFeedback.setTargetValue (feedbackTarget);
    mSmoothedDryWet.setTargetValue (dryWetTarget);
    mSmoothedGrit.setTargetValue (gritTarget);
    mSmoothedWidth.setTargetValue (juce::jlimit (0.0f, 1.0f, mWidthParam != nullptr ? mWidthParam->load() : 0.0f));
    mSmoothedRelease.setTargetValue (releaseTarget);
    mSmoothedCutoff.setTargetValue (cutoff);
    mSmoothedHpCutoff.setTargetValue (hpCutoff);
    float blockPeak = 0.0f;
    const auto attackCoeff = std::exp (-1.0f / (0.005f * sampleRate));
    constexpr auto boutiqueEnvDepth = 0.7f;

    constexpr int maxProcessChannels = 2;
    float channelInput[maxProcessChannels] {};
    float channelFiltered[maxProcessChannels] {};
    float channelHpFiltered[maxProcessChannels] {};

    for (int sample = 0; sample < numSamples; ++sample)
    {
        mModPhase += 1.2f / sampleRate;
        if (mModPhase >= 1.0f)
            mModPhase -= 1.0f;

        const auto diffuseActive = mDiffusionParam != nullptr ? mDiffusionParam->load() > 0.5f : false;

        const auto smoothedCutoff = mSmoothedCutoff.getNextValue();
        const auto smoothedHpCutoff = mSmoothedHpCutoff.getNextValue();

        const auto feedback = mSmoothedFeedback.getNextValue();
        const auto dryWet = mSmoothedDryWet.getNextValue();
        const auto grit = mSmoothedGrit.getNextValue();
        const auto releaseSeconds = mSmoothedRelease.getNextValue();
        const auto filterSmoothSeconds = juce::jmax (0.05f, releaseSeconds);
        const auto releaseCoeff = std::exp (-1.0f / (filterSmoothSeconds * sampleRate));
        const auto gritTiltOffset = juce::jmap (grit, 0.0f, 1.0f, 1.0f, 0.7f);
        const auto dryGain = 1.0f - dryWet;
        const auto wetGain = dryWet;
        const auto delayTimeMs = mSmoothedDelayTimeMs.getNextValue();
        const auto delayTimeSeconds = delayTimeMs * 0.001f;
        const auto delaySamples = delayTimeSeconds * sampleRate;
        const auto decorrelationAmount = juce::jmap (delayTimeMs, 0.1f, 1000.0f, 0.0f, 0.015f);

        mJitterAmount += (mRandom.nextFloat() * 2.0f - 1.0f) * 0.001f;
        mJitterAmount *= 0.999f;
        const auto jitterSamples = mJitterAmount * grit * 2.5f;
        const auto widthBlend = mSmoothedWidth.getNextValue();
        const auto useStereoWidthMatrix = (totalOutputChannels == 2 && delayChannels >= 2);
        constexpr auto widthTimeSpread = 0.2f;
        const auto channelDelaySamplesL = delaySamples * (1.0f - (widthBlend * widthTimeSpread));
        const auto channelDelaySamplesR = delaySamples * (1.0f + (widthBlend * widthTimeSpread));
        const auto autoDampedFeedback = juce::jlimit (0.0f, 0.99f, feedback);
        float inputL = 0.0f;
        float inputR = 0.0f;
        float fbL = 0.0f;
        float fbR = 0.0f;

        for (int channel = 0; channel < totalOutputChannels; ++channel)
        {
            const auto channelIndex = juce::jmin (channel, maxProcessChannels - 1);
            auto* channelData = buffer.getWritePointer (channel);
            auto* delayData = mDelayBuffer.getWritePointer (channel % delayChannels);

            float channelDelaySamples = delaySamples;
            if (useStereoWidthMatrix)
                channelDelaySamples = (channel == 0) ? channelDelaySamplesL : channelDelaySamplesR;
            else if (channel % 2 == 1)
                channelDelaySamples = delaySamples * (1.0f + decorrelationAmount);

            const auto wowDepth = mWowDepthParam != nullptr ? mWowDepthParam->load() : 0.15f;
            const auto maxSafeSwing = channelDelaySamples * 0.80f;
            const auto targetSwing = wowDepth * 150.0f;
            const auto safeSwing = juce::jmin (targetSwing, maxSafeSwing);
            const auto lfoOffsetSamples = std::sin (mModPhase * juce::MathConstants<float>::twoPi) * safeSwing;

            float readPosition = static_cast<float> (mWritePosition) - channelDelaySamples;
            while (readPosition < 0.0f)
                readPosition += static_cast<float> (delayBufferLength);
            while (readPosition >= static_cast<float> (delayBufferLength))
                readPosition -= static_cast<float> (delayBufferLength);

            float jitteredReadPos = readPosition + jitterSamples + lfoOffsetSamples;
            while (jitteredReadPos < 0.0f)
                jitteredReadPos += static_cast<float> (delayBufferLength);
            while (jitteredReadPos >= static_cast<float> (delayBufferLength))
                jitteredReadPos -= static_cast<float> (delayBufferLength);

            const auto readIndexA = static_cast<int> (jitteredReadPos);
            const auto readIndexB = (readIndexA + 1) % delayBufferLength;
            const auto frac = jitteredReadPos - static_cast<float> (readIndexA);

            const auto inputSample = channelData[sample];
            blockPeak = juce::jmax (blockPeak, std::abs (inputSample));
            const auto delayedA = delayData[readIndexA];
            const auto delayedB = delayData[readIndexB];
            const auto delayedSample = delayedA + frac * (delayedB - delayedA);

            const auto grittySample = applyWarmSaturation (delayedSample, grit);
            const auto filteredSample = mFilter.processSample (channel % delayChannels, grittySample);
            const auto hpFilteredSample = mHPFilter.processSample (channel % delayChannels, filteredSample);

            channelInput[channelIndex] = inputSample;
            channelFiltered[channelIndex] = filteredSample;
            channelHpFiltered[channelIndex] = hpFilteredSample;
        }

        float absInput = 0.0f;
        for (int inCh = 0; inCh < totalInputChannels; ++inCh)
        {
            const auto channelIndex = juce::jmin (inCh, maxProcessChannels - 1);
            absInput = juce::jmax (absInput,
                                   std::abs (buffer.getSample (inCh, sample)),
                                   std::abs (channelHpFiltered[channelIndex]));
        }

        if (absInput > mEnvelopeLevel)
            mEnvelopeLevel = attackCoeff * mEnvelopeLevel + (1.0f - attackCoeff) * absInput;
        else
            mEnvelopeLevel = releaseCoeff * mEnvelopeLevel + (1.0f - releaseCoeff) * absInput;

        const auto envModulation = mEnvelopeLevel * 8000.0f * boutiqueEnvDepth;
        const auto modulatedCutoff = juce::jlimit (20.0f, 20000.0f, (smoothedCutoff * gritTiltOffset) + envModulation);
        mFilter.setCutoffFrequency (modulatedCutoff);
        mHPFilter.setCutoffFrequency (smoothedHpCutoff);

        for (int channel = 0; channel < totalOutputChannels; ++channel)
        {
            const auto channelIndex = juce::jmin (channel, maxProcessChannels - 1);
            auto* channelData = buffer.getWritePointer (channel);
            auto* delayData = mDelayBuffer.getWritePointer (channel % delayChannels);

            const auto inputSample = channelInput[channelIndex];
            const auto filteredSample = channelFiltered[channelIndex];
            const auto hpFilteredSample = channelHpFiltered[channelIndex];
            auto processedFb = std::tanh (hpFilteredSample * autoDampedFeedback) * 0.98f;

            if (diffuseActive)
            {
                constexpr float kG = 0.65f;

                for (int stage = 0; stage < kNumDiffStages; ++stage)
                {
                    const auto maxLen = mDiffLengths[stage];

                    auto& diffBuf = (channel == 0) ? mDiffBuffersL[stage] : mDiffBuffersR[stage];
                    auto& writePos = (channel == 0) ? mDiffWritePositionsL[stage] : mDiffWritePositionsR[stage];

                    const auto bufSample = diffBuf[static_cast<size_t> (writePos)];
                    const auto x = processedFb;
                    const auto y = (-kG * x) + bufSample;

                    diffBuf[static_cast<size_t> (writePos)] = x + (kG * y);
                    processedFb = y;

                    if (++writePos >= maxLen)
                        writePos = 0;
                }
            }

            if (useStereoWidthMatrix)
            {
                if (channel == 0)
                {
                    inputL = inputSample;
                    fbL = processedFb;
                }
                else if (channel == 1)
                {
                    inputR = inputSample;
                    fbR = processedFb;
                }
            }
            else
            {
                delayData[mWritePosition] = inputSample + processedFb;
            }

            const auto mixedSample = (inputSample * dryGain) + (filteredSample * wetGain);
            channelData[sample] = applySoftSafetyLimit (mixedSample);
        }

        if (useStereoWidthMatrix)
        {
            const auto parallelL = inputL + fbL;
            const auto parallelR = inputR + fbR;
            const auto pingPongL = inputL + fbR;
            const auto pingPongR = fbL;
            const auto parallelMix = 1.0f - widthBlend;

            auto* delayL = mDelayBuffer.getWritePointer (0);
            auto* delayR = mDelayBuffer.getWritePointer (1);
            delayL[mWritePosition] = (parallelMix * parallelL) + (widthBlend * pingPongL);
            delayR[mWritePosition] = (parallelMix * parallelR) + (widthBlend * pingPongR);
        }

        ++mWritePosition;
        if (mWritePosition >= delayBufferLength)
            mWritePosition = 0;
    }

    // Smoothed input meter for the editor jewel light.
    const auto previous = mInputMeterLevel.load();
    const auto smoothed = juce::jmax (blockPeak, previous * 0.92f);
    mInputMeterLevel.store (smoothed);
}

bool RumbleRoomAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* RumbleRoomAudioProcessor::createEditor()
{
    return new RumbleRoomAudioProcessorEditor (*this);
}

void RumbleRoomAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (const auto state = apvts.copyState(); state.isValid())
    {
        std::unique_ptr<juce::XmlElement> xml (state.createXml());
        copyXmlToBinary (*xml, destData);
    }
}

void RumbleRoomAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    const std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr)
    {
        const auto newTree = juce::ValueTree::fromXml (*xmlState);
        if (newTree.isValid())
            apvts.replaceState (newTree);
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout RumbleRoomAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("delayTime", "Delay Time",
                                                                    juce::NormalisableRange<float> (0.1f, 1000.0f, 0.01f, 0.35f), 20.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("feedback", "Feedback",
                                                                    juce::NormalisableRange<float> (0.0f, 0.99f, 0.0001f), 0.25f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("dryWet", "Dry/Wet",
                                                                    juce::NormalisableRange<float> (0.0f, 1.0f, 0.0001f), 0.26f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("grit", "Grit",
                                                                    juce::NormalisableRange<float> (0.0f, 1.0f, 0.0001f), 0.41f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("width", "Width",
                                                                    juce::NormalisableRange<float> (0.0f, 1.0f, 0.0001f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("release", "Release",
                                                                    juce::NormalisableRange<float> (0.1f, 2.0f, 0.001f), 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("cutoff", "Cutoff",
                                                                    juce::NormalisableRange<float> (20.0f, 20000.0f, 1.0f, 0.35f), 1380.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("hpCutoff", "Bright",
                                                                    juce::NormalisableRange<float> (20.0f, 8000.0f, 1.0f, 0.35f), 35.0f));
    params.push_back (std::make_unique<juce::AudioParameterBool> ("sync", "Sync", false));
    params.push_back (std::make_unique<juce::AudioParameterChoice> ("subdivision", "Subdivision",
                                                                     juce::StringArray { "1/1", "1/2", "1/2T", "1/4", "1/4T",
                                                                                         "1/8", "1/8T", "1/16", "1/16T", "1/32", "1/64" },
                                                                     3));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("bpm", "BPM",
                                                                    juce::NormalisableRange<float> (40.0f, 260.0f, 0.1f), 120.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("wowDepth", "Wow & Flutter",
                                                                    juce::NormalisableRange<float> (0.0f, 1.0f, 0.0001f), 0.15f));
    params.push_back (std::make_unique<juce::AudioParameterBool> ("diffusion", "Diffuse Space", false));
    return { params.begin(), params.end() };
}

float RumbleRoomAudioProcessor::applySoftSafetyLimit (float sample) noexcept
{
    constexpr auto threshold = 0.92f;
    const auto absSample = std::abs (sample);

    if (absSample <= threshold)
        return sample;

    const auto excess = absSample - threshold;
    const auto limited = threshold + (std::tanh (excess * 3.0f) * (1.0f - threshold));
    return std::copysign (limited, sample);
}

float RumbleRoomAudioProcessor::applyWarmSaturation (float input, float gritAmount) const noexcept
{
    if (gritAmount <= 0.001f)
        return input;

    const auto drive = 1.0f + (gritAmount * 5.5f);
    const auto x = input * drive;
    const auto shaped = (x >= 0.0f) ? std::tanh (x)
                                    : (std::atan (x) * (2.0f / juce::MathConstants<float>::pi));

    const auto saturatorMix = juce::jmap (gritAmount, 0.0f, 1.0f, 0.45f, 0.82f);
    const auto blended = (input * (1.0f - saturatorMix)) + (shaped * saturatorMix);
    const auto levelComp = juce::jmap (gritAmount, 0.0f, 1.0f, 1.0f, 0.88f);
    return blended * levelComp;
}

float RumbleRoomAudioProcessor::getInputMeterLevel() const noexcept
{
    return mInputMeterLevel.load();
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RumbleRoomAudioProcessor();
}
