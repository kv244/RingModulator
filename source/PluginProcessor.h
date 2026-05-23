#pragma once
#include <JuceHeader.h>

class RingModAudioProcessor  : public juce::AudioProcessor
{
public:
    RingModAudioProcessor();
    ~RingModAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "RingMod"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int index) override {}
    const juce::String getProgramName (int index) override { return {}; }
    void changeProgramName (int index, const juce::String& newName) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return parameters; }

    static constexpr int kScopeSize = 512;
    const std::array<float, kScopeSize>& getScopeData() const noexcept { return scopeData; }

private:
    std::array<float, kScopeSize> scopeData {};

    juce::AudioProcessorValueTreeState parameters;
    std::atomic<float>* freqParam = nullptr;
    std::atomic<float>* mixParam  = nullptr;

    juce::dsp::Oscillator<float> carrierOsc;
    juce::AudioBuffer<float> carrierBuffer;

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RingModAudioProcessor)
};