#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

/**
 * @class FuturisticLookAndFeel
 * @brief Custom LookAndFeel that applies a dark, neon-accented "sci-fi" style
 *        to rotary sliders, linear bar sliders, and combo boxes.
 *
 * All colours and geometry are tuned for the 700×470 plug-in window.
 * The accent colour (cyan for freq, green for mix) is chosen at render time
 * by reading the Slider's name, so a single LAF instance serves both controls.
 *
 * Methods overridden
 * ------------------
 *  drawRotarySlider   — multi-layer knob with outer glow, gradient body, and value arc
 *  drawComboBox       — gradient-filled box with a custom downward-pointing arrow
 *  positionComboBoxText — shifts the label left to make room for the arrow
 *  drawLinearSlider   — horizontal bar used for the mix LED strip
 */
class FuturisticLookAndFeel : public juce::LookAndFeel_V4
{
public:
    /**
     * Sets global PopupMenu and ComboBox colours so the dropdown matches the
     * dark neon theme without requiring per-instance colour overrides.
     */
    FuturisticLookAndFeel();

    /**
     * Draws a multi-layer rotary knob:
     *  1. Outer radial glow halo (colour-gradient fill over the full bounds)
     *  2. Outer bezel ring (dark fill + dim border ellipse)
     *  3. Gradient inner body (dark radial gradient)
     *  4. Track arc (full-range dim arc)
     *  5. Value arc with glow (two-pass: wide+dim, then narrow+bright)
     *  6. Needle (thin line rotated to current value)
     *  7. Centre cap (small dark circle with accent ring and inner dot)
     *
     * The accent colour is cyan (#00C8FF) for the "freq" slider and
     * green (#00FF88) for the "mix" slider, resolved by slider.getName().
     */
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float startAngle, float endAngle,
                           juce::Slider& slider) override;

    /**
     * Draws a dark gradient-filled rounded rectangle with a cyan downward
     * triangle arrow in the right margin.  The popup menu colours are set
     * globally in the constructor so they inherit automatically.
     */
    void drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH,
                       juce::ComboBox& box) override;

    /**
     * Positions the combo box text label to the left of the custom arrow,
     * and applies a bold 14 pt font for better legibility at small sizes.
     */
    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override;

    /**
     * Draws a horizontal LED-strip bar (used for the mix bar slider):
     *  - Dark rounded background track
     *  - Gradient-filled active region from left up to the current value
     *  - A bright vertical edge line at the fill boundary
     *
     * The accent colour follows the same name-based logic as drawRotarySlider.
     */
    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           const juce::Slider::SliderStyle style, juce::Slider& slider) override;
};

/**
 * @class RingModAudioProcessorEditor
 * @brief GUI editor for the RingMod plug-in.
 *
 * Renders a 700×470 dark sci-fi panel divided into three columns:
 *
 *  Left column   — Carrier frequency rotary knob, digital Hz readout, tick labels
 *  Centre column — Carrier-waveform oscilloscope, waveform ComboBox
 *  Right column  — Dry/wet mix rotary knob, LED bar slider, percentage readout
 *
 * Additional visual layers (drawn over the column layout):
 *  - Background colour blooms (radial gradients) for depth
 *  - Diagonal metallic streak lines
 *  - Edge vignette darkening
 *  - Scanline overlay (horizontal lines every 4 px at 10% opacity)
 *  - Corner accent brackets
 *  - Title bar / subtitle strip / footer status bar
 *
 * A 30 Hz juce::Timer drains the processor's lock-free scope FIFO into
 * @c displayBuf and calls repaint(), keeping the oscilloscope live.
 * A @c frameCounter is incremented each tick and displayed as a small
 * "frame number" readout to confirm the UI is updating.
 */
class RingModAudioProcessorEditor : public juce::AudioProcessorEditor,
                                     private juce::Timer
{
public:
    /** Constructs the editor, sets up all controls and APVTS attachments, starts the 30 Hz timer. */
    RingModAudioProcessorEditor (RingModAudioProcessor&);

    /** Tears down the editor; resets the global LookAndFeel pointer before the LAF is destroyed. */
    ~RingModAudioProcessorEditor() override;

    /** Full repaint: background, all column content, and overlay layers. */
    void paint (juce::Graphics&) override;

    /** Positions freqSlider, waveformBox, mixBarSlider, and mixSlider within the 3-column layout. */
    void resized() override;

private:
    // =========================================================================
    // Timer
    // =========================================================================

    /**
     * Called at 30 Hz.  Drains new carrier samples from the processor FIFO
     * into the rolling @c displayBuf snapshot, increments @c frameCounter,
     * then requests a repaint.
     */
    void timerCallback() override;

    // =========================================================================
    // Data members
    // =========================================================================

    /** Back-reference to the owning processor (APVTS access and FIFO draining). */
    RingModAudioProcessor& audioProcessor;

    /**
     * Rolling display buffer for the oscilloscope, holding the most recent
     * kScopeSize carrier samples.  Only ever touched on the message thread.
     */
    std::array<float, RingModAudioProcessor::kScopeSize> displayBuf {};

    /**
     * Incremented by timerCallback() every tick (~30 Hz).
     * Displayed as a 3-digit zero-padded counter in the frequency readout box
     * to give a visual "heartbeat" confirming the UI is refreshing.
     */
    int frameCounter = 0;

    /** Parameter display values cached in timerCallback() so paint() needs no APVTS lookups. */
    float cachedFreqHz      = 440.0f;
    float cachedMixVal      = 1.0f;
    int   cachedWaveformIdx = 0;

    /** Column bounds computed once in resized() and consumed as local copies in paint(). */
    juce::Rectangle<int> colLeft, colCenter, colRight;

    // ---- LookAndFeel (must outlive all child components) ----

    /** Single LAF instance shared by all controls via setLookAndFeel() on the editor itself. */
    FuturisticLookAndFeel customLAF;

    // ---- Controls ----

    /** Rotary knob for carrier frequency (20–2000 Hz). Named "freq" so the LAF picks cyan. */
    juce::Slider   freqSlider;

    /** Rotary knob for dry/wet mix (0.0–1.0). Named "mix" so the LAF picks green. */
    juce::Slider   mixSlider;

    /**
     * Horizontal linear bar slider mirroring the mix value.
     * Provides a secondary LED-strip readout alongside the rotary knob.
     * Named "mix" so the LAF renders the green accent colour.
     */
    juce::Slider   mixBarSlider;

    /** Dropdown for carrier waveform: Sine / Saw / Square / Triangle. */
    juce::ComboBox waveformBox;

    // ---- APVTS attachments ----

    /** Keeps freqSlider in sync with the "freq" APVTS parameter (supports DAW automation). */
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   freqAttachment;

    /** Keeps mixSlider in sync with the "mix" APVTS parameter. */
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   mixAttachment;

    /**
     * Keeps mixBarSlider in sync with the same "mix" APVTS parameter as mixSlider.
     * Both attachments listen to the same parameter, so moving either control
     * updates the other automatically.
     */
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   mixBarAttachment;

    /** Keeps waveformBox in sync with the "waveform" APVTS parameter. */
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> waveformAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RingModAudioProcessorEditor)
};
