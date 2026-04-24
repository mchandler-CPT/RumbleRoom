#pragma once

#include <JuceHeader.h>

class RumbleRoomAudioProcessor : public juce::AudioProcessor
{
public:
    RumbleRoomAudioProcessor();
    ~RumbleRoomAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #if ! JucePlugin_IsMidiEffect
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    float getInputMeterLevel() const noexcept;

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    float applyWarmSaturation (float input, float gritAmount) const noexcept;

    // Delay "sushi belt" state.
    juce::AudioBuffer<float> mDelayBuffer;
    int mWritePosition { 0 };

    // Raw parameter pointers for fast reads in processBlock.
    std::atomic<float>* mDelayTimeParam { nullptr };
    std::atomic<float>* mFeedbackParam { nullptr };
    std::atomic<float>* mDryWetParam { nullptr };
    std::atomic<float>* mGritParam { nullptr };
    std::atomic<float>* mCutoffParam { nullptr };

    juce::dsp::StateVariableTPTFilter<float> mFilter;
    juce::LinearSmoothedValue<float> mSmoothedDelayTimeMs;
    juce::LinearSmoothedValue<float> mSmoothedFeedback;
    juce::LinearSmoothedValue<float> mSmoothedDryWet;
    juce::LinearSmoothedValue<float> mSmoothedGrit;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mSmoothedCutoff;
    std::atomic<float> mInputMeterLevel { 0.0f };
    float mEnvelopeLevel { 0.0f };
    float mJitterAmount { 0.0f };
    juce::Random mRandom;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RumbleRoomAudioProcessor)
};
