#pragma once
#include <JuceHeader.h>

class RingModAudioProcessor  : public juce::AudioProcessor
{
public:
    RingModAudioProcessor();
    ~RingModAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock         (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

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

    // Scope feed — written by the audio thread, drained by the UI thread.
    // kScopeFifoSize is large enough to buffer ~185 ms at 44100 Hz so the
    // 30 Hz UI timer always drains before it overflows.
    static constexpr int kScopeSize     = 512;
    static constexpr int kScopeFifoSize = 8192;

    // Drains up to maxSamples from the FIFO into dest.
    // Safe to call from the message thread only.
    int drainScopeData (float* dest, int maxSamples) noexcept;

private:
    juce::AbstractFifo                    scopeFifo { kScopeFifoSize };
    std::array<float, kScopeFifoSize>     scopeRing {};

    juce::AudioProcessorValueTreeState parameters;
    std::atomic<float>* freqParam = nullptr;
    std::atomic<float>* mixParam  = nullptr;

    juce::SmoothedValue<float>    freqSmoothed;
    juce::dsp::Oscillator<float>  carrierOsc;
    juce::AudioBuffer<float>      carrierBuffer;
    juce::dsp::DryWetMixer<float> dryWetMixer;

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RingModAudioProcessor)
};