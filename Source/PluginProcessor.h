#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "SansAmpDSP.h"
#include "SansAmpPresets.h"

class SansAmpPSA1AudioProcessor final : public juce::AudioProcessor
{
public:
    SansAmpPSA1AudioProcessor();
    ~SansAmpPSA1AudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return kNumFactoryPresets; }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }
    juce::AudioProcessorParameter* getBypassParameter() const override { return bypassParam; }

    static constexpr const char* kParamIDs[8] = {
        "preamp", "buzz", "punch", "crunch", "drive", "low", "high", "level"
    };
    static constexpr const char* kParamNames[8] = {
        "Pre-Amp", "Buzz", "Punch", "Crunch", "Drive", "Low", "High", "Level"
    };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void updateDSPCoefficients() noexcept;

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float>* paramPtrs[8] {};
    std::atomic<float>* bypassParamPtr = nullptr;
    juce::AudioProcessorParameter* bypassParam = nullptr;

    SansAmpDSP::Coeffs coeffs;
    SansAmpDSP::ChannelState channels[2];
    float currentSampleRate = 44100.0f;
    int currentProgram = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SansAmpPSA1AudioProcessor)
};
