#pragma once
#include <JuceHeader.h>

/**
 * @class RingModAudioProcessor
 * @brief Core audio processor for the RingMod VST3/Standalone plug-in.
 *
 * Implements a ring modulator by multiplying every input channel by a
 * mono carrier oscillator that can produce Sine, Sawtooth, Square, or
 * Triangle waveforms.  The carrier frequency and dry/wet mix are exposed
 * as APVTS parameters so DAWs can automate and save them.
 *
 * Threading model
 * ---------------
 * - processBlock() runs on the **audio thread** (real-time, no allocation).
 * - All UI reads happen on the **message thread** via drainScopeData().
 * - Data flows audio → UI through a lock-free FIFO (scopeFifo / scopeRing).
 */
class RingModAudioProcessor  : public juce::AudioProcessor
{
public:
    RingModAudioProcessor();
    ~RingModAudioProcessor() override;

    // =========================================================================
    // AudioProcessor lifecycle
    // =========================================================================

    /** Called by the host before streaming begins; resets phase, smoothers, and the DryWetMixer. */
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;

    /** Called by the host after streaming ends; resets the DryWetMixer delay buffer. */
    void releaseResources() override;

    /** Real-time audio callback: generates the carrier, ring-modulates the buffer, then blends dry/wet. */
    void processBlock         (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    /** Bypass callback: pumps the DryWetMixer at mix = 0 to keep its delay buffer in sync. */
    void processBlockBypassed (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    // =========================================================================
    // Bus / format support
    // =========================================================================

    /** Accepts mono-in/mono-out or stereo-in/stereo-out only; input and output sets must match. */
    bool isBusesLayoutSupported (const BusesLayout&) const override;

    // =========================================================================
    // Editor
    // =========================================================================

    /** Creates and returns the plug-in GUI editor. */
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    // =========================================================================
    // Plug-in identity
    // =========================================================================

    const juce::String getName() const override { return "RingMod"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    // =========================================================================
    // Programs (not used — single preset only)
    // =========================================================================

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int index) override {}
    const juce::String getProgramName (int index) override { return {}; }
    void changeProgramName (int index, const juce::String& newName) override {}

    // =========================================================================
    // State persistence
    // =========================================================================

    /** Serialises the APVTS parameter state to binary XML for DAW preset save. */
    void getStateInformation (juce::MemoryBlock& destData) override;

    /** Restores the APVTS parameter state from binary XML; guards against malformed data. */
    void setStateInformation (const void* data, int sizeInBytes) override;

    // =========================================================================
    // Parameter access
    // =========================================================================

    /** Returns a reference to the AudioProcessorValueTreeState so the editor can attach sliders. */
    juce::AudioProcessorValueTreeState& getAPVTS() { return parameters; }

    // =========================================================================
    // Lock-free scope FIFO
    // =========================================================================

    /**
     * Number of samples kept in the oscilloscope display buffer.
     * The editor shows the most recent kScopeSize carrier samples.
     */
    static constexpr int kScopeSize     = 512;

    /**
     * Capacity of the lock-free FIFO that bridges the audio and UI threads.
     * At 44 100 Hz this is ~185 ms — enough for the 30 Hz UI timer to drain
     * between ticks even if the host delivers large buffer sizes.
     */
    static constexpr int kScopeFifoSize = 8192;

    /**
     * Drains up to @p maxSamples carrier samples from the FIFO into @p dest.
     *
     * @param dest       Caller-owned buffer to receive samples (must be ≥ maxSamples).
     * @param maxSamples Maximum number of samples to read.
     * @return           Actual number of samples written to @p dest.
     *
     * @note Must only be called from the message thread.
     */
    int drainScopeData (float* dest, int maxSamples) noexcept;

private:
    // =========================================================================
    // Scope FIFO (audio thread writes, message thread reads)
    // =========================================================================

    /** JUCE lock-free FIFO controller (indices, read/write cursors). */
    juce::AbstractFifo                    scopeFifo { kScopeFifoSize };

    /** Ring buffer backing the FIFO; samples are written here by the audio thread. */
    std::array<float, kScopeFifoSize>     scopeRing {};

    // =========================================================================
    // Parameters
    // =========================================================================

    /** APVTS that owns and synchronises all automatable parameters. */
    juce::AudioProcessorValueTreeState parameters;

    /** Raw atomic pointer to the carrier frequency parameter (Hz, 20–2000). */
    std::atomic<float>* freqParam     = nullptr;

    /** Raw atomic pointer to the dry/wet mix parameter (0.0 = fully dry, 1.0 = fully wet). */
    std::atomic<float>* mixParam      = nullptr;

    /** Raw atomic pointer to the waveform choice parameter (0 = Sine, 1 = Saw, 2 = Square, 3 = Triangle). */
    std::atomic<float>* waveformParam = nullptr;

    // =========================================================================
    // Oscillator state
    // =========================================================================

    /**
     * Smoothed carrier frequency.
     * Ramps over 20 ms to prevent zipper noise when the knob is moved.
     */
    juce::SmoothedValue<float>    freqSmoothed;

    /** Current phase accumulator in [0, 2π). Advanced every sample. */
    double                        oscillatorPhase    = 0.0;

    /** Host sample rate cached in prepareToPlay(); used by generateSample(). */
    double                        currentSampleRate  = 44100.0;

    /** Pre-computed 2π / sampleRate to avoid a division inside the per-sample loop. */
    double                        twoPiOverSR        = juce::MathConstants<double>::twoPi / 44100.0;

    /**
     * Active waveform index (0–3).
     * Updated only at phase zero-crossings to avoid mid-cycle discontinuities.
     */
    int                           currentWaveform    = 0;

    /** Single-channel scratch buffer that holds the generated carrier signal. */
    juce::AudioBuffer<float>      carrierBuffer;

    /** JUCE DSP dry/wet blending utility; maintains an internal latency-compensation delay. */
    juce::dsp::DryWetMixer<float> dryWetMixer;

    // =========================================================================
    // Helpers
    // =========================================================================

    /**
     * Pure waveform generator: maps @p phase ∈ [0, 2π) to a normalised sample.
     *
     * @param phase    Current oscillator phase in radians.
     * @param waveform Waveform index (0 = Sine, 1 = Saw, 2 = Square, 3 = Triangle).
     * @return         Sample value in [-1, +1].
     *
     * @note This is called once per sample on the audio thread; it must remain
     *       allocation-free, exception-free, and branch-predictable.
     */
    static float generateSample (double phase, int waveform) noexcept;

    /**
     * Builds the APVTS parameter layout (frequency, mix, waveform).
     * Called once from the constructor initialiser list.
     */
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RingModAudioProcessor)
};