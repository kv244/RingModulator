#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

/**
 * @class GlowKnobLookAndFeel
 * @brief Custom LookAndFeel that renders a dark knob with a coloured glow arc.
 *
 * Used for both the frequency and mix rotary sliders.  Each instance is
 * constructed with a "track" colour (the dim full-range arc) and a "thumb"
 * colour (the active glow arc and pointer), allowing the two knobs to use
 * visually distinct colour pairs without duplicating code.
 */
class GlowKnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    /**
     * @param trackColour  Colour used for the background arc (low opacity).
     * @param thumbColour  Colour used for the active arc, pointer, and tip dot.
     */
    GlowKnobLookAndFeel (juce::Colour trackColour, juce::Colour thumbColour)
        : track (trackColour), thumb (thumbColour) {}

    /**
     * Draws a custom rotary slider with:
     *  - A dark circular body
     *  - A dim track arc over the full parameter range
     *  - A double-layer glow arc (wide+dim, then narrow+bright) up to the current value
     *  - A gradient-filled inner knob body
     *  - A rectangular pointer and a glowing tip dot
     */
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float startAngle, float endAngle,
                           juce::Slider&) override;
private:
    juce::Colour track, thumb;  ///< Per-instance colours set at construction time.
};

/**
 * @class RingModAudioProcessorEditor
 * @brief GUI editor for the RingMod plug-in.
 *
 * Renders a dark, hardware-inspired panel containing:
 *  1. A digital LED frequency display with a range label
 *  2. Frequency tick marks drawn around the carrier-frequency knob
 *  3. An oscilloscope showing the live carrier waveform
 *  4. A waveform selector ComboBox (Sine / Saw / Square / Triangle)
 *  5. A Mix LED progress bar with percentage readout
 *  6. A PARAMETERS status strip at the bottom
 *  7. A JUCE logo badge in the top-right corner
 *
 * A 30 Hz juce::Timer drains the processor's lock-free scope FIFO into the
 * local @c displayBuf snapshot and triggers a repaint — all on the message
 * thread, so no additional synchronisation is required.
 */
class RingModAudioProcessorEditor : public juce::AudioProcessorEditor,
                                     private juce::Timer
{
public:
    /** Constructs the editor, attaches APVTS sliders/combo-box, and starts the 30 Hz repaint timer. */
    RingModAudioProcessorEditor (RingModAudioProcessor&);

    /** Destroys the editor; resets LookAndFeel pointers before the LAF objects are destroyed. */
    ~RingModAudioProcessorEditor() override;

    /** Full repaint: draws the background, header, and all labelled sub-sections. */
    void paint (juce::Graphics&) override;

    /** Positions the freq knob, mix knob, and waveform ComboBox within the fixed 640×400 window. */
    void resized() override;

private:
    // =========================================================================
    // Timer
    // =========================================================================

    /**
     * Called at 30 Hz by the JUCE timer system.
     * Drains the processor's FIFO into the rolling @c displayBuf window and
     * requests a repaint so the oscilloscope reflects recent carrier data.
     */
    void timerCallback() override;

    // =========================================================================
    // Paint helpers — each draws one labelled UI region
    // =========================================================================

    /** Draws the JUCE "J" logo badge in the top-right corner of the header. */
    void drawJuceLogo        (juce::Graphics&);

    /** Draws the LED-style digital frequency readout and the "Range: 20 Hz – 2 kHz" label. */
    void drawDigitalDisplay  (juce::Graphics&);

    /** Draws calibrated Hz tick marks and labels around the outer edge of the freq knob. */
    void drawFreqTickMarks   (juce::Graphics&);

    /** Draws the live carrier-waveform oscilloscope with a glow pass and a sharp line pass. */
    void drawOscilloscope    (juce::Graphics&);

    /** Draws the "Waveform" label above the ComboBox (the box itself is a child component). */
    void drawWaveformSelector(juce::Graphics&);

    /** Draws the Mix label, percentage value, LED bar, and "Dry/Wet Mix" caption. */
    void drawMixSection      (juce::Graphics&);

    /** Draws the PARAMETERS status strip at the very bottom of the panel. */
    void drawParamsPanel     (juce::Graphics&);

    // =========================================================================
    // Utilities
    // =========================================================================

    /**
     * Maps a carrier frequency in Hz to a rotary angle in radians, applying the
     * same skew (power 0.3) as the APVTS NormalisableRange so tick marks align
     * exactly with the knob's visual position.
     *
     * @param hz         Frequency to convert (clamped to [20, 2000]).
     * @param startAngle Starting angle of the rotary arc (radians).
     * @param endAngle   Ending angle of the rotary arc (radians).
     * @return           Corresponding rotary angle in radians.
     */
    static float hzToAngle (float hz, float startAngle, float endAngle) noexcept;

    // =========================================================================
    // Data members
    // =========================================================================

    /** Back-reference to the owning processor (for APVTS access and FIFO draining). */
    RingModAudioProcessor& audioProcessor;

    /**
     * Rolling display buffer populated by timerCallback().
     * Holds the most recent kScopeSize carrier samples.
     * Only ever touched on the message thread — no synchronisation needed.
     */
    std::array<float, RingModAudioProcessor::kScopeSize> displayBuf {};

    // ---- LookAndFeel instances (must outlive the sliders) ----

    /** Cyan/teal glow style for the carrier-frequency knob. */
    GlowKnobLookAndFeel freqLAF { juce::Colour::fromRGB (0, 170, 200), juce::Colour::fromRGB (0, 240, 255) };

    /** Green glow style for the dry/wet mix knob. */
    GlowKnobLookAndFeel mixLAF  { juce::Colour::fromRGB (0, 155, 75),  juce::Colour::fromRGB (0, 255, 120) };

    // ---- Controls ----

    /** Rotary slider controlling the carrier oscillator frequency (20–2000 Hz). */
    juce::Slider   freqSlider;

    /** Rotary slider controlling the dry/wet blend (0.0 = dry, 1.0 = fully ring-modulated). */
    juce::Slider   mixSlider;

    /** Dropdown that selects the carrier waveform (Sine / Saw / Square / Triangle). */
    juce::ComboBox waveformBox;

    // ---- APVTS attachments (keep controls synchronised with parameters) ----

    /** Bidirectional link between freqSlider and the "freq" APVTS parameter. */
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   freqAttachment;

    /** Bidirectional link between mixSlider and the "mix" APVTS parameter. */
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   mixAttachment;

    /** Bidirectional link between waveformBox and the "waveform" APVTS parameter. */
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> waveformAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RingModAudioProcessorEditor)
};
