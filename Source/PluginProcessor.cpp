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
    mCutoffParam    = apvts.getRawParameterValue ("cutoff");
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

    const auto initialCutoff = juce::jlimit (20.0f, 20000.0f, mCutoffParam != nullptr ? mCutoffParam->load() : 1380.0f);
    const auto initialDelayMs = juce::jlimit (1.0f, 1000.0f, mDelayTimeParam != nullptr ? mDelayTimeParam->load() : 20.0f);
    const auto initialFeedback = juce::jlimit (0.0f, 0.99f, mFeedbackParam != nullptr ? mFeedbackParam->load() : 0.25f);
    const auto initialDryWet = juce::jlimit (0.0f, 1.0f, mDryWetParam != nullptr ? mDryWetParam->load() : 0.26f);
    const auto initialGrit = juce::jlimit (0.0f, 1.0f, mGritParam != nullptr ? mGritParam->load() : 0.41f);
    mSmoothedDelayTimeMs.reset (sampleRate, 0.04);
    mSmoothedDelayTimeMs.setCurrentAndTargetValue (initialDelayMs);
    mSmoothedFeedback.reset (sampleRate, 0.04);
    mSmoothedFeedback.setCurrentAndTargetValue (initialFeedback);
    mSmoothedDryWet.reset (sampleRate, 0.04);
    mSmoothedDryWet.setCurrentAndTargetValue (initialDryWet);
    mSmoothedGrit.reset (sampleRate, 0.04);
    mSmoothedGrit.setCurrentAndTargetValue (initialGrit);
    mSmoothedCutoff.reset (sampleRate, 0.04);
    mSmoothedCutoff.setCurrentAndTargetValue (initialCutoff);
    mFilter.setCutoffFrequency (initialCutoff);
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
    const auto cutoff = juce::jlimit (20.0f, 20000.0f, mCutoffParam != nullptr ? mCutoffParam->load() : 1380.0f);
    const auto delayTargetMs = juce::jlimit (1.0f, 1000.0f, mDelayTimeParam != nullptr ? mDelayTimeParam->load() : 20.0f);

    mSmoothedDelayTimeMs.setTargetValue (delayTargetMs);
    mSmoothedFeedback.setTargetValue (feedbackTarget);
    mSmoothedDryWet.setTargetValue (dryWetTarget);
    mSmoothedGrit.setTargetValue (gritTarget);
    mSmoothedCutoff.setTargetValue (cutoff);
    float blockPeak = 0.0f;

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const auto smoothedCutoff = mSmoothedCutoff.getNextValue();
        mFilter.setCutoffFrequency (smoothedCutoff);

        const auto feedback = mSmoothedFeedback.getNextValue();
        const auto dryWet = mSmoothedDryWet.getNextValue();
        const auto grit = mSmoothedGrit.getNextValue();
        const auto gritCurve = grit * grit;
        const auto autoDampedFeedback = feedback * (1.0f - (gritCurve * 0.15f));
        const auto dryGain = 1.0f - dryWet;
        const auto wetGain = dryWet;
        const auto delayTimeMs = mSmoothedDelayTimeMs.getNextValue();
        const auto delayTimeSeconds = delayTimeMs * 0.001f;
        const auto delaySamples = delayTimeSeconds * sampleRate;
        const auto decorrelationAmount = juce::jmap (delayTimeMs, 1.0f, 1000.0f, 0.0f, 0.015f);

        for (int channel = 0; channel < totalOutputChannels; ++channel)
        {
            auto* channelData = buffer.getWritePointer (channel);
            auto* delayData = mDelayBuffer.getWritePointer (channel % delayChannels);

            const auto channelDelaySamples = (channel % 2 == 1)
                                               ? delaySamples * (1.0f + decorrelationAmount)
                                               : delaySamples;

            float readPosition = static_cast<float> (mWritePosition) - channelDelaySamples;
            while (readPosition < 0.0f)
                readPosition += static_cast<float> (delayBufferLength);
            while (readPosition >= static_cast<float> (delayBufferLength))
                readPosition -= static_cast<float> (delayBufferLength);

            const auto readIndexA = static_cast<int> (readPosition);
            const auto readIndexB = (readIndexA + 1) % delayBufferLength;
            const auto frac = readPosition - static_cast<float> (readIndexA);

            const auto inputSample = channelData[sample];
            blockPeak = juce::jmax (blockPeak, std::abs (inputSample));
            const auto delayedA = delayData[readIndexA];
            const auto delayedB = delayData[readIndexB];
            const auto delayedSample = delayedA + frac * (delayedB - delayedA);

            const auto grittySample = applyWarmSaturation (delayedSample, grit);
            const auto filteredSample = mFilter.processSample (channel % delayChannels, grittySample);
            const auto feedbackPath = std::tanh (filteredSample * autoDampedFeedback) * 0.97f;

            delayData[mWritePosition] = inputSample + feedbackPath;
            channelData[sample] = (inputSample * dryGain) + (filteredSample * wetGain);
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
                                                                    juce::NormalisableRange<float> (1.0f, 1000.0f, 0.01f, 0.35f), 20.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("feedback", "Feedback",
                                                                    juce::NormalisableRange<float> (0.0f, 0.99f, 0.0001f), 0.25f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("dryWet", "Dry/Wet",
                                                                    juce::NormalisableRange<float> (0.0f, 1.0f, 0.0001f), 0.26f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("grit", "Grit",
                                                                    juce::NormalisableRange<float> (0.0f, 1.0f, 0.0001f), 0.41f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("cutoff", "Cutoff",
                                                                    juce::NormalisableRange<float> (20.0f, 20000.0f, 1.0f, 0.35f), 1380.0f));
    return { params.begin(), params.end() };
}

float RumbleRoomAudioProcessor::applyWarmSaturation (float input, float gritAmount) const noexcept
{
    if (gritAmount <= 0.001f)
        return input;

    const auto drive = 1.0f + (gritAmount * 7.0f);
    const auto x = input * drive;
    const auto shaped = (x >= 0.0f) ? std::tanh (x)
                                    : (std::atan (x) * (2.0f / juce::MathConstants<float>::pi));

    // Hotter than the old fixed dry blend, but still compensated for loop stability.
    const auto saturatorMix = juce::jmap (gritAmount, 0.0f, 1.0f, 0.55f, 0.90f);
    const auto blended = (input * (1.0f - saturatorMix)) + (shaped * saturatorMix);
    const auto levelComp = juce::jmap (gritAmount, 0.0f, 1.0f, 1.0f, 0.92f);
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
