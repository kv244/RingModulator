#include "PluginProcessor.h"
#include "PluginEditor.h"

RingModAudioProcessor::RingModAudioProcessor()
    : AudioProcessor (BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "PARAMS", createParameterLayout())
{
    freqParam = parameters.getRawParameterValue ("freq");
    mixParam  = parameters.getRawParameterValue ("mix");
}

RingModAudioProcessor::~RingModAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout RingModAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "freq", "Carrier Frequency", juce::NormalisableRange<float> (20.0f, 2000.0f, 0.01f, 0.3f), 440.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "mix", "Dry/Wet Mix", 0.0f, 1.0f, 1.0f));
    return layout;
}

void RingModAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Carrier is mono — one sine wave applied identically to all channels
    juce::dsp::ProcessSpec spec { sampleRate, (uint32) samplesPerBlock, 1u };
    carrierOsc.initialise ([](float x) { return std::sin (x); });
    carrierOsc.prepare (spec);
    carrierBuffer.setSize (1, samplesPerBlock);
}

void RingModAudioProcessor::releaseResources() {}

void RingModAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    if (numChannels == 0 || numSamples == 0) return;

    carrierOsc.setFrequency (freqParam->load());

    juce::dsp::AudioBlock<float> carrierBlock (carrierBuffer);
    juce::dsp::AudioBlock<float> clippedCarrierBlock = carrierBlock.getSubBlock (0, (size_t) numSamples);
    juce::dsp::ProcessContextReplacing<float> carrierContext (clippedCarrierBlock);
    carrierOsc.process (carrierContext);

    {
        auto* src = carrierBuffer.getReadPointer (0);
        int n = juce::jmin (numSamples, kScopeSize);
        for (int i = 0; i < n; ++i) scopeData[i] = src[i];
    }

    const float currentMix = mixParam->load();

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* mainSignal = buffer.getWritePointer (ch);
        auto* carrierSignal = carrierBuffer.getReadPointer (0); // mono carrier shared across all channels

        for (int n = 0; n < numSamples; ++n)
        {
            float drySample = mainSignal[n];
            float wetSample = drySample * carrierSignal[n];
            mainSignal[n] = drySample + currentMix * (wetSample - drySample);
        }
    }
}

juce::AudioProcessorEditor* RingModAudioProcessor::createEditor() { return new RingModAudioProcessorEditor (*this); }

void RingModAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void RingModAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RingModAudioProcessor();
}