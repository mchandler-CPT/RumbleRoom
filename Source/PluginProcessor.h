#pragma once

#include <vector>

#include <JuceHeader.h>
#include "PresetManager.h"

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

    PresetManager& getPresetManager() { return mPresetManager; }

    juce::AudioProcessorValueTreeState apvts;

private:
    PresetManager mPresetManager { apvts, "RumbleRoom", "BoutiqueAudio", ".rr" };

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    float applyWarmSaturation (float input, float gritAmount) const noexcept;
    static float applySoftSafetyLimit (float sample) noexcept;

    // Delay "sushi belt" state.
    juce::AudioBuffer<float> mDelayBuffer;
    int mWritePosition { 0 };

    // Raw parameter pointers for fast reads in processBlock.
    std::atomic<float>* mDelayTimeParam { nullptr };
    std::atomic<float>* mFeedbackParam { nullptr };
    std::atomic<float>* mDryWetParam { nullptr };
    std::atomic<float>* mGritParam { nullptr };
    std::atomic<float>* mWidthParam { nullptr };
    std::atomic<float>* mCutoffParam { nullptr };
    std::atomic<float>* mHpCutoffParam { nullptr };
    std::atomic<float>* mReleaseParam { nullptr };
    std::atomic<float>* mDuckDepthParam { nullptr };
    std::atomic<float>* mSyncParam { nullptr };
    std::atomic<float>* mSubdivisionParam { nullptr };
    std::atomic<float>* mBpmParam { nullptr };
    std::atomic<float>* mWowDepthParam { nullptr };
    std::atomic<float>* mWowSpeedParam { nullptr };
    std::atomic<float>* mDiffusionParam { nullptr };
    std::atomic<float>* mDampingParam { nullptr };

    // 1-Pole Lowpass Filter state variables (history buffers)
    float mDampStateL = 0.0f;
    float mDampStateR = 0.0f;

    // Tape pitch modulation (Wow & Flutter) variables
    float mModPhase { 0.0f };      // Fast mechanical warble tracker
    float mModPhaseSlow { 0.0f };  // Slow thermal voice sag tracker

    struct ModulatedAllpass
    {
        std::vector<float> buffer;
        int writePos = 0;
        int length = 0;
        float lfoPhase = 0.0f;
        float lfoSpeed = 0.0f;

        void init (int size, float speed)
        {
            length = size;
            buffer.assign (static_cast<size_t> (length), 0.0f);
            writePos = 0;
            lfoPhase = 0.0f;
            lfoSpeed = speed;
        }

        float process (float input, float kG, float sampleRate)
        {
            if (length <= 0)
                return input;

            lfoPhase += lfoSpeed / sampleRate;
            if (lfoPhase >= 1.0f)
                lfoPhase -= 1.0f;

            const auto lfoMod = std::sin (lfoPhase * juce::MathConstants<float>::twoPi) * 8.0f;
            auto readPos = static_cast<float> (writePos) - (static_cast<float> (length) * 0.5f) + lfoMod;

            while (readPos < 0.0f)
                readPos += static_cast<float> (length);
            while (readPos >= static_cast<float> (length))
                readPos -= static_cast<float> (length);

            const auto idxA = static_cast<int> (readPos);
            const auto idxB = (idxA + 1) % length;
            const auto frac = readPos - static_cast<float> (idxA);

            const auto bufSample = buffer[static_cast<size_t> (idxA)]
                                 + frac * (buffer[static_cast<size_t> (idxB)] - buffer[static_cast<size_t> (idxA)]);

            const auto x = input;
            const auto y = (-kG * x) + bufSample;
            buffer[static_cast<size_t> (writePos)] = x + (kG * y);

            if (++writePos >= length)
                writePos = 0;

            return y;
        }
    };

    static constexpr int kNumDiffStages = 4;
    ModulatedAllpass mFiltersL[kNumDiffStages];
    ModulatedAllpass mFiltersR[kNumDiffStages];

    juce::dsp::StateVariableTPTFilter<float> mFilter;
    juce::dsp::StateVariableTPTFilter<float> mHPFilter;
    juce::LinearSmoothedValue<float> mSmoothedDelayTimeMs;
    juce::LinearSmoothedValue<float> mSmoothedFeedback;
    juce::LinearSmoothedValue<float> mSmoothedDryWet;
    juce::LinearSmoothedValue<float> mSmoothedGrit;
    juce::LinearSmoothedValue<float> mSmoothedWidth;
    juce::LinearSmoothedValue<float> mSmoothedRelease;
    juce::LinearSmoothedValue<float> mSmoothedDuckDepth;
    juce::LinearSmoothedValue<float> mSmoothedDiffusion;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mSmoothedCutoff;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mSmoothedHpCutoff;
    std::atomic<float> mInputMeterLevel { 0.0f };
    float mEnvelopeLevel { 0.0f };

    // Sidechain ducking engine: tracks the DRY input only, blind to wet/feedback.
    float mDuckEnvelope { 0.0f };
    int mHoldSamplesLeft { 0 };
    float mJitterAmount { 0.0f };
    juce::Random mRandom;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RumbleRoomAudioProcessor)
};
