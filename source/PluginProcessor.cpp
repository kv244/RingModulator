#include "PluginProcessor.h"
#include "PluginEditor.h"

// Registers stereo buses and binds raw parameter pointers for lock-free audio-thread access.
RingModAudioProcessor::RingModAudioProcessor()
    : AudioProcessor (BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "PARAMS", createParameterLayout())
{
    freqParam     = parameters.getRawParameterValue ("freq");
    mixParam      = parameters.getRawParameterValue ("mix");
    waveformParam = parameters.getRawParameterValue ("waveform");
}

RingModAudioProcessor::~RingModAudioProcessor() {}

// Skew factor 0.3 on freq gives a log-like curve so the knob feels linear across decades.
juce::AudioProcessorValueTreeState::ParameterLayout RingModAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "freq", "Carrier Frequency", juce::NormalisableRange<float> (20.0f, 2000.0f, 0.01f, 0.3f), 440.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "mix", "Dry/Wet Mix", 0.0f, 1.0f, 1.0f));
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        "waveform", "Waveform",
        juce::StringArray { "Sine", "Saw", "Square", "Triangle" }, 0));
    return layout;
}

// Accepts mono or stereo only; input and output must match (required by Ableton Live and Renoise FX chains).
bool RingModAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out == juce::AudioChannelSet::disabled())
        return false;
    // Accept mono or stereo only; input and output must match.
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

// Resets phase and smoothers; DryWetMixer channel count covers asymmetric bus configs.
void RingModAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    twoPiOverSR       = juce::MathConstants<double>::twoPi / sampleRate;
    oscillatorPhase   = 0.0;
    currentWaveform   = (int) std::round (waveformParam->load());

    freqSmoothed.reset (sampleRate, 0.02);
    freqSmoothed.setCurrentAndTargetValue (freqParam->load());

    carrierBuffer.setSize (1, samplesPerBlock);

    // DryWetMixer channel count covers both bus directions to handle asymmetric configs
    const uint32 numChannels = (uint32) juce::jmax (1,
        juce::jmax (getTotalNumInputChannels(), getTotalNumOutputChannels()));
    juce::dsp::ProcessSpec mainSpec { sampleRate, (uint32) samplesPerBlock, numChannels };
    dryWetMixer.prepare (mainSpec);
}

// Flushes the DryWetMixer delay buffer so stale samples don't bleed into the next stream.
void RingModAudioProcessor::releaseResources()
{
    dryWetMixer.reset();
}

// Generates the carrier, writes it to the scope FIFO, then SIMD-multiplies every channel in-place.
// Waveform switches only at phase zero-crossing to avoid mid-cycle discontinuities.
void RingModAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    if (numChannels == 0 || numSamples == 0) return;

    const int targetWaveform = (int) std::round (waveformParam->load());
    freqSmoothed.setTargetValue (freqParam->load());
    auto* carrierOut = carrierBuffer.getWritePointer (0);
    for (int n = 0; n < numSamples; ++n)
    {
        carrierOut[n]    = generateSample (oscillatorPhase, currentWaveform);
        oscillatorPhase += twoPiOverSR * (double) freqSmoothed.getNextValue();
        if (oscillatorPhase >= juce::MathConstants<double>::twoPi)
        {
            oscillatorPhase -= juce::MathConstants<double>::twoPi;
            currentWaveform  = targetWaveform;  // switch only at cycle boundary
        }
    }

    // Push carrier samples into the lock-free scope FIFO.
    // Only write as many samples as the FIFO currently has room for;
    // if the UI is slow the excess is silently dropped rather than blocking.
    {
        auto* src = carrierBuffer.getReadPointer (0);
        const int toWrite = juce::jmin (numSamples, scopeFifo.getFreeSpace());
        if (toWrite > 0)
        {
            int start1, size1, start2, size2;
            scopeFifo.prepareToWrite (toWrite, start1, size1, start2, size2);
            for (int i = 0; i < size1; ++i) scopeRing[start1 + i] = src[i];
            for (int i = 0; i < size2; ++i) scopeRing[start2 + i] = src[size1 + i];
            scopeFifo.finishedWrite (size1 + size2);
        }
    }

    // Latch the mix value and hand the dry signal to the mixer before we
    // overwrite the buffer with the ring-modulated (wet) signal.
    juce::dsp::AudioBlock<float> mainBlock (buffer);
    dryWetMixer.setWetMixProportion (mixParam->load());
    dryWetMixer.pushDrySamples (mainBlock);

    // Ring modulation: SIMD-multiply every channel by the mono carrier in-place.
    auto* carrier = carrierBuffer.getReadPointer (0);
    for (int ch = 0; ch < numChannels; ++ch)
        juce::FloatVectorOperations::multiply (buffer.getWritePointer (ch), carrier, numSamples);

    dryWetMixer.mixWetSamples (mainBlock);
}

// Pumps the DryWetMixer at mix=0 while bypassed so its internal delay buffer stays in sync,
// preventing a click when bypass is toggled back off.
void RingModAudioProcessor::processBlockBypassed (juce::AudioBuffer<float>& buffer,
                                                   juce::MidiBuffer&)
{
    // Keep the DryWetMixer delay buffer in sync so bypass toggling is click-free.
    juce::dsp::AudioBlock<float> mainBlock (buffer);
    dryWetMixer.setWetMixProportion (0.0f);
    dryWetMixer.pushDrySamples (mainBlock);
    dryWetMixer.mixWetSamples (mainBlock);
}

juce::AudioProcessorEditor* RingModAudioProcessor::createEditor() { return new RingModAudioProcessorEditor (*this); }

// Serialises the APVTS state to XML binary for DAW preset save.
void RingModAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

// Restores APVTS state from DAW preset; guards against malformed or mismatched XML.
void RingModAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xmlState));
}

// Pure function: maps phase ∈ [0, 2π) to a normalised sample for the chosen waveform.
// Called once per sample on the audio thread — must stay allocation-free and branch-predictable.
float RingModAudioProcessor::generateSample (double phase, int waveform) noexcept
{
    // phase is in [0, 2π)
    using M = juce::MathConstants<double>;
    switch (waveform)
    {
        case 1:  // Saw: ramps linearly from -1 to +1
            return (float) (phase / M::pi - 1.0);

        case 2:  // Square
            return phase < M::pi ? 1.0f : -1.0f;

        case 3:  // Triangle: -1 at 0, +1 at π, -1 at 2π
            return (float) (phase < M::pi ? (2.0 * phase / M::pi - 1.0)
                                          : (3.0 - 2.0 * phase / M::pi));

        default: // Sine
            return (float) std::sin (phase);
    }
}

// Drains up to maxSamples from the lock-free FIFO into dest; returns the count actually read.
// Safe to call on the message thread while the audio thread writes concurrently.
int RingModAudioProcessor::drainScopeData (float* dest, int maxSamples) noexcept
{
    const int toRead = juce::jmin (scopeFifo.getNumReady(), maxSamples);
    if (toRead == 0) return 0;

    int start1, size1, start2, size2;
    scopeFifo.prepareToRead (toRead, start1, size1, start2, size2);
    for (int i = 0; i < size1; ++i) dest[i]         = scopeRing[start1 + i];
    for (int i = 0; i < size2; ++i) dest[size1 + i] = scopeRing[start2 + i];
    scopeFifo.finishedRead (size1 + size2);

    return size1 + size2;
}

// DAW entry point: JUCE calls this once to instantiate the processor when the plugin is loaded.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RingModAudioProcessor();
}