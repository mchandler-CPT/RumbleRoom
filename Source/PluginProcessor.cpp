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
    mDuckDepthParam = apvts.getRawParameterValue ("duckDepth");
    mSyncParam      = apvts.getRawParameterValue ("sync");
    mSubdivisionParam = apvts.getRawParameterValue ("subdivision");
    mBpmParam       = apvts.getRawParameterValue ("bpm");
    mWowDepthParam  = apvts.getRawParameterValue ("wowDepth");
    mWowSpeedParam  = apvts.getRawParameterValue ("wowSpeed");
    mDiffusionParam = apvts.getRawParameterValue ("diffusion");
    mDampingParam   = apvts.getRawParameterValue ("damping");
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
    const auto delayBufferSize = juce::jmax (1, static_cast<int> (sampleRate * 9.0));

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
    const auto initialDelayMs = juce::jlimit (0.1f, 8000.0f, mDelayTimeParam != nullptr ? mDelayTimeParam->load() : 20.0f);
    const auto initialFeedback = juce::jlimit (0.0f, 0.99f, mFeedbackParam != nullptr ? mFeedbackParam->load() : 0.25f);
    const auto initialDryWet = juce::jlimit (0.0f, 1.0f, mDryWetParam != nullptr ? mDryWetParam->load() : 0.26f);
    const auto initialGrit = juce::jlimit (0.0f, 1.0f, mGritParam != nullptr ? mGritParam->load() : 0.41f);
    const auto initialWidth = juce::jlimit (0.0f, 1.0f, mWidthParam != nullptr ? mWidthParam->load() : 0.0f);
    const auto initialRelease = juce::jlimit (0.001f, 2.0f, mReleaseParam != nullptr ? mReleaseParam->load() : 0.5f);
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
    const auto initialDuckDepth = juce::jlimit (0.0f, 1.0f, mDuckDepthParam != nullptr ? mDuckDepthParam->load() : 0.0f);
    mSmoothedDuckDepth.reset (sampleRate, 0.04);
    mSmoothedDuckDepth.setCurrentAndTargetValue (initialDuckDepth);
    mSmoothedCutoff.reset (sampleRate, 0.04);
    mSmoothedCutoff.setCurrentAndTargetValue (initialCutoff);
    mSmoothedHpCutoff.reset (sampleRate, 0.04);
    mSmoothedHpCutoff.setCurrentAndTargetValue (initialHpCutoff);
    const auto initialDiffusion = juce::jlimit (0.0f, 1.0f, mDiffusionParam != nullptr ? mDiffusionParam->load() : 0.0f);
    mSmoothedDiffusion.reset (sampleRate, 0.05);
    mSmoothedDiffusion.setCurrentAndTargetValue (initialDiffusion);
    mFilter.setCutoffFrequency (initialCutoff);
    mHPFilter.setCutoffFrequency (initialHpCutoff);
    mEnvelopeLevel = 0.0f;
    mDuckEnvelope = 0.0f;
    mHoldSamplesLeft = 0;
    mJitterAmount = 0.0f;
    mModPhase = 0.0f;
    mModPhaseSlow = 0.0f;
    mDampStateL = 0.0f;
    mDampStateR = 0.0f;

    const int diffLengths[kNumDiffStages] = { 557, 893, 1123, 1399 };
    const float diffSpeeds[kNumDiffStages] = { 0.35f, 0.57f, 0.79f, 1.11f };

    for (int i = 0; i < kNumDiffStages; ++i)
    {
        mFiltersL[i].init (diffLengths[i], diffSpeeds[i]);
        mFiltersR[i].init (diffLengths[i], diffSpeeds[i]);
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
    const auto releaseTarget = juce::jlimit (0.001f, 2.0f, mReleaseParam != nullptr ? mReleaseParam->load() : 0.5f);
    const auto duckDepthTarget = juce::jlimit (0.0f, 1.0f, mDuckDepthParam != nullptr ? mDuckDepthParam->load() : 0.0f);
    const auto cutoff = juce::jlimit (20.0f, 20000.0f, mCutoffParam != nullptr ? mCutoffParam->load() : 1380.0f);
    const auto hpCutoff = juce::jlimit (20.0f, 8000.0f, mHpCutoffParam != nullptr ? mHpCutoffParam->load() : 35.0f);
    const auto wowDepth = juce::jlimit (0.0f, 1.0f, mWowDepthParam != nullptr ? mWowDepthParam->load() : 0.15f);
    const auto wowSpeed = juce::jlimit (0.1f, 6.0f, mWowSpeedParam != nullptr ? mWowSpeedParam->load() : 1.0f);
    const auto currentDiffusion = juce::jlimit (0.0f, 1.0f, mDiffusionParam != nullptr ? mDiffusionParam->load() : 0.0f);
    const auto dampingHz = juce::jlimit (200.0f, 20000.0f, mDampingParam != nullptr ? mDampingParam->load() : 12000.0f);

    auto delayTargetMs = juce::jlimit (0.1f, 8000.0f, mDelayTimeParam != nullptr ? mDelayTimeParam->load() : 20.0f);
    const auto syncEnabled = (mSyncParam != nullptr && mSyncParam->load() > 0.5f);

    if (syncEnabled)
    {
        constexpr float subdivisionMultipliers[] { 32.0f, 16.0f, 8.0f, 4.0f, 2.0f, 4.0f / 3.0f, 1.0f, 2.0f / 3.0f,
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

        const auto subdivisionIndex = juce::jlimit (0, 13, juce::roundToInt (mSubdivisionParam != nullptr ? mSubdivisionParam->load() : 6.0f));
        delayTargetMs = (60000.0f / juce::jmax (1.0f, bpm)) * subdivisionMultipliers[subdivisionIndex];
        delayTargetMs = juce::jlimit (0.1f, 8000.0f, delayTargetMs);
    }

    mSmoothedDelayTimeMs.setTargetValue (delayTargetMs);
    mSmoothedFeedback.setTargetValue (feedbackTarget);
    mSmoothedDryWet.setTargetValue (dryWetTarget);
    mSmoothedGrit.setTargetValue (gritTarget);
    mSmoothedWidth.setTargetValue (juce::jlimit (0.0f, 1.0f, mWidthParam != nullptr ? mWidthParam->load() : 0.0f));
    mSmoothedRelease.setTargetValue (releaseTarget);
    mSmoothedDuckDepth.setTargetValue (duckDepthTarget);
    mSmoothedCutoff.setTargetValue (cutoff);
    mSmoothedHpCutoff.setTargetValue (hpCutoff);
    mSmoothedDiffusion.setTargetValue (currentDiffusion);

    float blockPeak = 0.0f;
    const auto attackCoeff = std::exp (-1.0f / (0.005f * sampleRate));
    constexpr auto boutiqueEnvDepth = 0.7f;

    // Brightness-following filter envelope: decoupled from Release so that the
    // Release knob is reserved strictly for the ducking recovery time.
    const auto filterReleaseCoeff = std::exp (-1.0f / (0.25f * sampleRate));

    // Sidechain ducking: fast attack clamps onto fresh transients instantly,
    // recovery time is driven per-sample by the Release parameter.
    const auto duckAttackCoeff = std::exp (-1.0f / (0.007f * sampleRate));
    // 50ms peak-hold window bridges waveform zero-crossings so mid-wave valleys
    // can't prematurely reopen the gate at short Release settings.
    const int holdWindowSamples = static_cast<int> (0.05f * sampleRate);

    const auto dampAlpha = juce::jlimit (0.01f, 0.99f,
                                         std::exp (-juce::MathConstants<float>::twoPi * dampingHz / sampleRate));

    constexpr int maxProcessChannels = 2;
    float channelHpFiltered[maxProcessChannels] {};

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const auto activeDiffusion = mSmoothedDiffusion.getNextValue();
        const auto smoothedCutoff = mSmoothedCutoff.getNextValue();
        const auto smoothedHpCutoff = mSmoothedHpCutoff.getNextValue();
        const auto feedback = mSmoothedFeedback.getNextValue();
        const auto dryWet = mSmoothedDryWet.getNextValue();
        const auto grit = mSmoothedGrit.getNextValue();
        const auto widthBlend = mSmoothedWidth.getNextValue();
        const auto releaseSeconds = mSmoothedRelease.getNextValue();
        const auto duckDepth = mSmoothedDuckDepth.getNextValue();

        // --- Sidechain ducking envelope (DRY input only with Peak-Hold) ---
        // Sampled before the channel loop overwrites the buffer with the wet mix,
        // so the follower stays completely blind to the wet output and feedback path.
        float dryPeak = 0.0f;
        for (int inCh = 0; inCh < totalInputChannels; ++inCh)
            dryPeak = juce::jmax (dryPeak, std::abs (buffer.getSample (inCh, sample)));

        if (dryPeak > mDuckEnvelope)
        {
            mDuckEnvelope = duckAttackCoeff * mDuckEnvelope + (1.0f - duckAttackCoeff) * dryPeak;
            mHoldSamplesLeft = holdWindowSamples; // Lock and reset the hold window on peaks
        }
        else
        {
            if (mHoldSamplesLeft > 0)
            {
                // Freeze the envelope decay completely while the wave is crossing valleys
                --mHoldSamplesLeft;
            }
            else
            {
                // Only decay after the 50ms hold window expires safely
                const auto duckReleaseCoeff = std::exp (-1.0f / (juce::jmax (0.001f, releaseSeconds) * sampleRate));
                mDuckEnvelope = duckReleaseCoeff * mDuckEnvelope + (1.0f - duckReleaseCoeff) * dryPeak;
            }
        }

        // --- STUDIO-GRADE COMPRESSOR ENGINE ---
        // 1. Convert our linear tracking envelope value to Decibels (dBFS)
        // We use a safety floor of 0.0001f (-80 dB) to prevent log10(0) infinity explosions.
        const float envelopeDb = 20.0f * std::log10 (juce::jmax (0.0001f, mDuckEnvelope));

        // 2. Define our Compression Threshold (-30 dB is an excellent musical pocket)
        constexpr float thresholdDb = -30.0f;

        // --- SKEWED PROGRESSIVE RATIO COMPONENT ---
        // Square the smoothed knob depth to give the bottom half of the dial
        // ultra-fine precision control, pushing aggressive compression to the top.
        const float skewedDepth = duckDepth * duckDepth;

        // 3. Scale our Compression Ratio directly off our progressive curve!
        // At 0.0 panel -> Ratio is 1:1 (True bypass)
        // At 0.5 panel -> Ratio is ~5.75:1 (Perfect smooth studio compression pocket)
        // At 1.0 panel -> Ratio hits 20:1 (Heavy downward limiting)
        const float ratio = 1.0f + (skewedDepth * 19.0f);

        float compressedOutputDb = envelopeDb;

        // 4. Apply Downward Gain Reduction above the Threshold
        if (envelopeDb > thresholdDb)
        {
            float excessDb = envelopeDb - thresholdDb;
            // Compress the over-shoot by our parameter-driven ratio
            compressedOutputDb = thresholdDb + (excessDb / ratio);
        }

        // 5. Calculate the Gain Reduction delta and convert back to a linear amplitude multiplier
        const float gainReductionDb = compressedOutputDb - envelopeDb;
        const float duckGain = std::pow (10.0f, gainReductionDb * 0.05f);

        const auto gritTiltOffset = juce::jmap (grit, 0.0f, 1.0f, 1.0f, 0.7f);
        const auto dryGain = 1.0f - dryWet;
        const auto wetGain = dryWet;

        const auto delayTimeMs = mSmoothedDelayTimeMs.getNextValue();
        const auto delaySamples = (delayTimeMs * 0.001f) * sampleRate;
        const auto decorrelationAmount = juce::jmap (delayTimeMs, 0.1f, 1000.0f, 0.0f, 0.015f);

        mModPhase += wowSpeed / sampleRate;
        if (mModPhase >= 1.0f)
            mModPhase -= 1.0f;

        mModPhaseSlow += (wowSpeed * 0.163f) / sampleRate;
        if (mModPhaseSlow >= 1.0f)
            mModPhaseSlow -= 1.0f;

        mJitterAmount += (mRandom.nextFloat() * 2.0f - 1.0f) * 0.001f;
        mJitterAmount *= 0.999f;
        const auto jitterSamples = mJitterAmount * grit * 2.5f;

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

            // CRITICAL FIX: Cap the max pointer swing to keep Wow musical at extreme lengths
            const auto maxSafeSwing = juce::jmin (channelDelaySamples * 0.80f, 150.0f);
            const auto safeSwing = juce::jmin (wowDepth * 150.0f, maxSafeSwing);
            const auto fastWarble = std::sin (mModPhase * juce::MathConstants<float>::twoPi);
            const auto slowSag = std::sin (mModPhaseSlow * juce::MathConstants<float>::twoPi);
            const auto asymmetricMod = fastWarble * (0.65f + (slowSag * 0.35f));
            const auto lfoOffsetSamples = asymmetricMod * safeSwing;

            const auto readPosition = static_cast<float> (mWritePosition) - channelDelaySamples;
            auto jitteredReadPos = readPosition + jitterSamples + lfoOffsetSamples;

            while (jitteredReadPos < 0.0f)
                jitteredReadPos += static_cast<float> (delayBufferLength);
            while (jitteredReadPos >= static_cast<float> (delayBufferLength))
                jitteredReadPos -= static_cast<float> (delayBufferLength);

            const auto readIndexA = static_cast<int> (jitteredReadPos);
            const auto readIndexB = (readIndexA + 1) % delayBufferLength;
            const auto frac = jitteredReadPos - static_cast<float> (readIndexA);

            const auto inputSample = channelData[sample];
            blockPeak = juce::jmax (blockPeak, std::abs (inputSample));

            const auto delayedSample = delayData[readIndexA] + frac * (delayData[readIndexB] - delayData[readIndexA]);

            const auto grittySample = applyWarmSaturation (delayedSample, grit);
            const auto filteredSample = mFilter.processSample (channel % delayChannels, grittySample);
            auto hpFilteredSample = mHPFilter.processSample (channel % delayChannels, filteredSample);

            if (activeDiffusion > 0.001f)
            {
                auto* filterArray = (channel == 0) ? mFiltersL : mFiltersR;
                const auto currentKG = activeDiffusion * 0.62f;

                hpFilteredSample = filterArray[0].process (hpFilteredSample, currentKG, sampleRate);
                hpFilteredSample = filterArray[1].process (hpFilteredSample, currentKG, sampleRate);
                hpFilteredSample = filterArray[2].process (hpFilteredSample, currentKG, sampleRate);
                hpFilteredSample = filterArray[3].process (hpFilteredSample, currentKG, sampleRate);
            }

            channelHpFiltered[channelIndex] = hpFilteredSample;

            auto processedFb = std::tanh (hpFilteredSample * autoDampedFeedback) * 0.98f;

            if (activeDiffusion > 0.001f)
            {
                auto& dampState = (channel == 0) ? mDampStateL : mDampStateR;
                processedFb = (processedFb * (1.0f - dampAlpha)) + (dampState * dampAlpha);
                dampState = processedFb;
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

            // Ducking is applied here, and ONLY here: as a clean output fader on the
            // wet signal just before it meets the dry path. The delay buffer write and
            // feedback loop above are untouched, so the tail keeps accumulating in the
            // background and swells back in once the dry input releases.
            const auto mixedSample = (inputSample * dryGain) + (filteredSample * wetGain * duckGain);
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

        // Brightness-following envelope drives the filter cutoff. It uses the dry peak
        // captured above (the buffer now holds the wet mix) plus the wet tap, and decays
        // on its own fixed time constant so the Release knob stays dedicated to ducking.
        float absInput = dryPeak;
        for (int inCh = 0; inCh < totalInputChannels; ++inCh)
        {
            const auto chIdx = juce::jmin (inCh, maxProcessChannels - 1);
            absInput = juce::jmax (absInput, std::abs (channelHpFiltered[chIdx]));
        }

        if (absInput > mEnvelopeLevel)
            mEnvelopeLevel = attackCoeff * mEnvelopeLevel + (1.0f - attackCoeff) * absInput;
        else
            mEnvelopeLevel = filterReleaseCoeff * mEnvelopeLevel + (1.0f - filterReleaseCoeff) * absInput;

        // --- BBD-STYLE HIGH END DAMPENING ---
        // As delay time approaches 8 seconds, we smoothly scale down the maximum filter ceiling
        // to mimic analog BBD clock filtering, keeping long loops dark and clear of mud.
        const float bbdFilterScale = juce::jmap (delayTimeMs, 0.1f, 8000.0f, 1.0f, 0.35f);

        const auto envModulation = mEnvelopeLevel * 8000.0f * boutiqueEnvDepth * bbdFilterScale;
        const auto bbdCutoffFloor = smoothedCutoff * bbdFilterScale;
        const auto modulatedCutoff = juce::jlimit (20.0f, 20000.0f, (bbdCutoffFloor * gritTiltOffset) + envModulation);
        mFilter.setCutoffFrequency (modulatedCutoff);
        mHPFilter.setCutoffFrequency (smoothedHpCutoff);

        if (++mWritePosition >= delayBufferLength)
            mWritePosition = 0;
    }

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
                                                                    juce::NormalisableRange<float> (0.1f, 8000.0f, 0.01f, 0.28f), 20.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("feedback", "Feedback",
                                                                    juce::NormalisableRange<float> (0.0f, 0.99f, 0.0001f), 0.25f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("dryWet", "Dry/Wet",
                                                                    juce::NormalisableRange<float> (0.0f, 1.0f, 0.0001f), 0.26f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("grit", "Grit",
                                                                    juce::NormalisableRange<float> (0.0f, 1.0f, 0.0001f), 0.41f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("width", "Width",
                                                                    juce::NormalisableRange<float> (0.0f, 1.0f, 0.0001f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("release", "Release",
                                                                    juce::NormalisableRange<float> (0.001f, 2.0f, 0.001f, 0.35f), 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("duckDepth", "Ducking Depth",
                                                                    juce::NormalisableRange<float> (0.0f, 1.0f, 0.0001f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("cutoff", "Cutoff",
                                                                    juce::NormalisableRange<float> (20.0f, 20000.0f, 1.0f, 0.35f), 1380.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("hpCutoff", "Bright",
                                                                    juce::NormalisableRange<float> (20.0f, 8000.0f, 1.0f, 0.35f), 35.0f));
    params.push_back (std::make_unique<juce::AudioParameterBool> ("sync", "Sync", false));
    params.push_back (std::make_unique<juce::AudioParameterChoice> ("subdivision", "Subdivision",
                                                                     juce::StringArray { "8/1", "4/1", "2/1", "1/1", "1/2", "1/2T", "1/4", "1/4T",
                                                                                         "1/8", "1/8T", "1/16", "1/16T", "1/32", "1/64" },
                                                                     6));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("bpm", "BPM",
                                                                    juce::NormalisableRange<float> (40.0f, 260.0f, 0.1f), 120.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("wowDepth", "Wow & Flutter",
                                                                    juce::NormalisableRange<float> (0.0f, 1.0f, 0.0001f), 0.15f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("wowSpeed", "Wow Speed",
                                                                    juce::NormalisableRange<float> (0.1f, 6.0f, 0.01f, 0.4f), 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("diffusion", "Diffusion",
                                                                    juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("damping", "Damping",
                                                                    juce::NormalisableRange<float> (200.0f, 20000.0f, 1.0f, 0.35f), 12000.0f));
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
