#include "PluginProcessor.h"
#include "PluginEditor.h"

// =============================================================================
// Constructor
// =============================================================================

/**
 * Registers stereo I/O buses and binds raw atomic parameter pointers for
 * lock-free audio-thread access.
 *
 * The APVTS is initialised here via its constructor; createParameterLayout()
 * is called inside the initialiser list before the body executes.
 */
RingModAudioProcessor::RingModAudioProcessor()
    : AudioProcessor (BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "PARAMS", createParameterLayout())
{
    // Cache atomic pointers to avoid repeated APVTS look-ups on the audio thread.
    freqParam     = parameters.getRawParameterValue ("freq");
    mixParam      = parameters.getRawParameterValue ("mix");
    waveformParam = parameters.getRawParameterValue ("waveform");
}

RingModAudioProcessor::~RingModAudioProcessor() {}

// =============================================================================
// Parameter layout
// =============================================================================

/**
 * Builds the AudioProcessorValueTreeState parameter layout.
 *
 * Parameters
 * ----------
 * freq     — Carrier frequency in Hz (20–2 000 Hz).
 *            A skew factor of 0.3 gives a log-like response curve so the
 *            knob feels perceptually linear across decades.
 * mix      — Dry/wet blend (0.0 = fully dry, 1.0 = fully ring-modulated).
 * waveform — Carrier waveform choice: Sine (0), Saw (1), Square (2), Triangle (3).
 */
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

// =============================================================================
// Bus layout
// =============================================================================

/**
 * Accepts mono-in/mono-out or stereo-in/stereo-out; the input and output
 * channel sets must be identical.
 *
 * This restriction is required by Ableton Live and Renoise FX chains which
 * expect symmetric side-chain-free layouts for standard insert effects.
 */
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

// =============================================================================
// Lifecycle
// =============================================================================

/**
 * Called by the host before audio streaming begins.
 *
 * - Caches the sample rate and pre-computes 2π/SR to save a division per sample.
 * - Resets the oscillator phase to 0 so the first block always starts cleanly.
 * - Reads the current waveform choice so the first block uses the correct shape.
 * - Resets and primes the frequency smoother (20 ms ramp to avoid clicks).
 * - Sizes the carrier scratch buffer to the host block size.
 * - Prepares the DryWetMixer; channel count covers both bus directions to
 *   handle asymmetric I/O configurations gracefully.
 */
void RingModAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    twoPiOverSR       = juce::MathConstants<double>::twoPi / sampleRate;
    oscillatorPhase   = 0.0;
    currentWaveform   = (int) std::round (waveformParam->load());

    freqSmoothed.reset (sampleRate, 0.02);   // 20 ms smoothing window
    freqSmoothed.setCurrentAndTargetValue (freqParam->load());

    carrierBuffer.setSize (1, samplesPerBlock);

    // DryWetMixer channel count covers both bus directions to handle asymmetric configs.
    const uint32 numChannels = (uint32) juce::jmax (1,
        juce::jmax (getTotalNumInputChannels(), getTotalNumOutputChannels()));
    juce::dsp::ProcessSpec mainSpec { sampleRate, (uint32) samplesPerBlock, numChannels };
    dryWetMixer.prepare (mainSpec);
}

/**
 * Called by the host after audio streaming ends.
 *
 * Resets the DryWetMixer delay buffer so stale samples do not bleed into
 * the next audio stream (e.g., after a DAW transport stop/start).
 */
void RingModAudioProcessor::releaseResources()
{
    dryWetMixer.reset();
}

// =============================================================================
// Audio processing
// =============================================================================

/**
 * Real-time audio callback: ring-modulates the input buffer in place.
 *
 * Processing order
 * ----------------
 * 1. Generate the carrier waveform sample-by-sample into carrierBuffer.
 *    Waveform switches (when the user changes the selector) happen only at
 *    phase zero-crossings to avoid discontinuity clicks.
 * 2. Push carrier samples into the lock-free scope FIFO for the oscilloscope.
 *    Excess samples are silently dropped if the UI is slow rather than blocking.
 * 3. Hand the dry buffer to the DryWetMixer before overwriting it.
 * 4. Multiply every channel by the mono carrier using SIMD FloatVectorOperations.
 * 5. Let the DryWetMixer blend wet (ring-modulated) and dry (original) signals.
 *
 * @note ScopedNoDenormals is enabled to keep the phase accumulator fast on x86
 *       targets that would otherwise trap on subnormal floating-point values.
 */
void RingModAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    if (numChannels == 0 || numSamples == 0) return;

    // Snap the target waveform; only apply it at the next cycle boundary.
    const int targetWaveform = (int) std::round (waveformParam->load());
    freqSmoothed.setTargetValue (freqParam->load());

    // --- Step 1: Generate carrier ---
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

    // --- Step 2: Push carrier samples into the lock-free scope FIFO ---
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

    // --- Step 3: Latch dry signal before overwriting the buffer ---
    // Hand the unmodified audio to the DryWetMixer so it can blend it back in later.
    juce::dsp::AudioBlock<float> mainBlock (buffer);
    dryWetMixer.setWetMixProportion (mixParam->load());
    dryWetMixer.pushDrySamples (mainBlock);

    // --- Step 4: Ring modulation — SIMD-multiply every channel by the mono carrier ---
    auto* carrier = carrierBuffer.getReadPointer (0);
    for (int ch = 0; ch < numChannels; ++ch)
        juce::FloatVectorOperations::multiply (buffer.getWritePointer (ch), carrier, numSamples);

    // --- Step 5: Blend wet (ring-modulated) back with the preserved dry signal ---
    dryWetMixer.mixWetSamples (mainBlock);
}

/**
 * Bypass callback: keeps the DryWetMixer delay buffer in sync.
 *
 * When the plug-in is bypassed, the host still calls this method instead of
 * processBlock().  By pumping the mixer at mix = 0 we advance its internal
 * delay counter, preventing a click when the user toggles bypass back off.
 */
void RingModAudioProcessor::processBlockBypassed (juce::AudioBuffer<float>& buffer,
                                                   juce::MidiBuffer&)
{
    // Advance the DryWetMixer at mix = 0 (100% dry) to stay in sync with the audio clock.
    juce::dsp::AudioBlock<float> mainBlock (buffer);
    dryWetMixer.setWetMixProportion (0.0f);
    dryWetMixer.pushDrySamples (mainBlock);
    dryWetMixer.mixWetSamples (mainBlock);
}

// =============================================================================
// Editor
// =============================================================================

/** Returns a newly allocated editor; ownership is transferred to the caller. */
juce::AudioProcessorEditor* RingModAudioProcessor::createEditor() { return new RingModAudioProcessorEditor (*this); }

// =============================================================================
// State persistence
// =============================================================================

/**
 * Serialises the APVTS parameter state to binary XML for DAW preset save.
 *
 * The state is represented as a ValueTree, converted to an XmlElement, then
 * packed into @p destData by JUCE's copyXmlToBinary().
 */
void RingModAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

/**
 * Restores APVTS parameter state from a DAW preset.
 *
 * Guards against malformed or mismatched XML: the restored tree is only
 * applied if the binary data parses successfully and the root tag matches
 * the current state's type identifier.
 */
void RingModAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xmlState));
}

// =============================================================================
// Waveform generator
// =============================================================================

/**
 * Pure waveform generator: maps @p phase ∈ [0, 2π) to a normalised sample.
 *
 * Waveform equations
 * ------------------
 *  Sine     (0) : sin(phase)                               — standard sinusoid
 *  Sawtooth (1) : phase / π − 1                            — ramps −1 → +1 linearly
 *  Square   (2) : +1 if phase < π, else −1                 — 50% duty cycle
 *  Triangle (3) : linear up (−1 → +1) then back (1 → −1)  — piecewise linear
 *
 * @param phase    Current oscillator phase in radians; must be in [0, 2π).
 * @param waveform Waveform index (0 = Sine, 1 = Saw, 2 = Square, 3 = Triangle).
 * @return         Sample value in [-1, +1].
 *
 * @note Called once per sample on the audio thread.
 *       Must remain allocation-free, exception-free, and branch-predictable.
 */
float RingModAudioProcessor::generateSample (double phase, int waveform) noexcept
{
    using M = juce::MathConstants<double>;
    switch (waveform)
    {
        case 1:  // Saw: ramps linearly from -1 to +1
            return (float) (phase / M::pi - 1.0);

        case 2:  // Square: bang-bang between +1 and -1 at the halfway point
            return phase < M::pi ? 1.0f : -1.0f;

        case 3:  // Triangle: -1 at 0, +1 at π, -1 at 2π
            return (float) (phase < M::pi ? (2.0 * phase / M::pi - 1.0)
                                          : (3.0 - 2.0 * phase / M::pi));

        default: // Sine (case 0 and any unrecognised value)
            return (float) std::sin (phase);
    }
}

// =============================================================================
// Scope FIFO drain
// =============================================================================

/**
 * Drains up to @p maxSamples carrier samples from the lock-free FIFO into @p dest.
 *
 * Uses AbstractFifo::prepareToRead / finishedRead to handle the case where
 * the readable region wraps around the end of the ring buffer (two segments).
 *
 * @param dest       Caller-owned destination buffer (must be ≥ maxSamples floats).
 * @param maxSamples Maximum number of samples to read in one call.
 * @return           Actual number of samples copied; 0 if the FIFO is empty.
 *
 * @note Must only be called from the message thread while the audio thread writes.
 *       AbstractFifo is designed for exactly this producer–consumer pattern.
 */
int RingModAudioProcessor::drainScopeData (float* dest, int maxSamples) noexcept
{
    const int toRead = juce::jmin (scopeFifo.getNumReady(), maxSamples);
    if (toRead == 0) return 0;

    int start1, size1, start2, size2;
    scopeFifo.prepareToRead (toRead, start1, size1, start2, size2);
    // Copy the first contiguous segment
    for (int i = 0; i < size1; ++i) dest[i]         = scopeRing[start1 + i];
    // Copy the (optional) wrap-around second segment
    for (int i = 0; i < size2; ++i) dest[size1 + i] = scopeRing[start2 + i];
    scopeFifo.finishedRead (size1 + size2);

    return size1 + size2;
}

// =============================================================================
// DAW entry point
// =============================================================================

/**
 * JUCE plug-in factory function.
 *
 * Called once by the host when the VST3/Standalone binary is loaded.
 * Returns a heap-allocated RingModAudioProcessor; the host takes ownership.
 */
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RingModAudioProcessor();
}