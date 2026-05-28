#include "PluginProcessor.h"
#include "PluginEditor.h"

// Default JUCE rotary knob sweep: starts at 216° (≈ 7-o'clock) and ends at 504° (≈ 5-o'clock).
// These must match the startAngle / endAngle passed to GlowKnobLookAndFeel::drawRotarySlider.
static const float kRotStart = juce::MathConstants<float>::pi * 1.2f;  // 216°
static const float kRotEnd   = juce::MathConstants<float>::pi * 2.8f;  // 504°

// =============================================================================
// GlowKnobLookAndFeel
// =============================================================================

/**
 * Custom rotary slider renderer.
 *
 * Layers (back to front):
 *  1. Dark circular fill — the knob body background
 *  2. Track arc          — dim full-range arc showing the total sweep
 *  3. Wide glow arc      — broad, semi-transparent active arc for a bloom effect
 *  4. Sharp active arc   — narrow, fully-opaque active arc for crisp definition
 *  5. Gradient inner body — raised-looking knob face with a subtle gradient
 *  6. Border ellipse     — single-pixel border around the inner body
 *  7. Pointer rectangle  — thin bar rotated to the current value
 *  8. Tip dot            — two-pass (glow + bright) dot at the pointer tip
 */
void GlowKnobLookAndFeel::drawRotarySlider (juce::Graphics& g,
                                             int x, int y, int width, int height,
                                             float pos, float startAngle, float endAngle,
                                             juce::Slider&)
{
    // Reduce the bounding box by 6 px on each side so the knob doesn't clip.
    auto b = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (6.0f);
    float R  = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;  // outer radius
    float cx = b.getCentreX();
    float cy = b.getCentreY();
    float angle = startAngle + pos * (endAngle - startAngle);     // current pointer angle

    // --- Layer 1: outer dark fill ---
    g.setColour (juce::Colour::fromRGB (16, 20, 26));
    g.fillEllipse (cx - R, cy - R, R * 2.0f, R * 2.0f);

    // --- Layer 2: track background arc (full range, dim) ---
    {
        juce::Path p;
        p.addCentredArc (cx, cy, R * 0.80f, R * 0.80f, 0.0f, startAngle, endAngle, true);
        g.setColour (track.withAlpha (0.22f));
        g.strokePath (p, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));
    }

    // --- Layer 3: active glow arc (wide, dim) — creates a bloom halo ---
    {
        juce::Path p;
        p.addCentredArc (cx, cy, R * 0.80f, R * 0.80f, 0.0f, startAngle, angle, true);
        g.setColour (thumb.withAlpha (0.35f));
        g.strokePath (p, juce::PathStrokeType (9.0f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));
    }

    // --- Layer 4: active arc (sharp, fully opaque) ---
    {
        juce::Path p;
        p.addCentredArc (cx, cy, R * 0.80f, R * 0.80f, 0.0f, startAngle, angle, true);
        g.setColour (thumb);
        g.strokePath (p, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));
    }

    // --- Layer 5 + 6: gradient inner knob body + border ---
    float inner = R * 0.62f;  // radius of the raised knob face
    {
        juce::ColourGradient grad (juce::Colour::fromRGB (52, 63, 74), cx - inner * 0.35f, cy - inner * 0.35f,
                                   juce::Colour::fromRGB (19, 24, 31), cx + inner * 0.25f, cy + inner * 0.25f, false);
        g.setGradientFill (grad);
        g.fillEllipse (cx - inner, cy - inner, inner * 2.0f, inner * 2.0f);
    }
    g.setColour (juce::Colour::fromRGB (58, 70, 84));
    g.drawEllipse (cx - inner, cy - inner, inner * 2.0f, inner * 2.0f, 1.0f);

    // --- Layer 7: pointer rectangle (thin bar from centre toward the arc) ---
    {
        juce::Path ptr;
        ptr.addRectangle (-1.5f, -inner * 0.90f, 3.0f, inner * 0.52f);
        ptr.applyTransform (juce::AffineTransform::rotation (angle).translated (cx, cy));
        g.setColour (thumb);
        g.fillPath (ptr);
    }

    // --- Layer 8: tip dot (glow pass then bright centre dot) ---
    // Position the dot along the pointer direction, just inside the active arc.
    float dotX = cx + std::sin (angle) * inner * 0.70f;
    float dotY = cy - std::cos (angle) * inner * 0.70f;
    g.setColour (thumb.withAlpha (0.35f));
    g.fillEllipse (dotX - 5.5f, dotY - 5.5f, 11.0f, 11.0f);  // wide, dim halo
    g.setColour (thumb);
    g.fillEllipse (dotX - 2.5f, dotY - 2.5f,  5.0f,  5.0f);  // bright centre
}

// =============================================================================
// RingModAudioProcessorEditor — constructor / destructor
// =============================================================================

RingModAudioProcessorEditor::RingModAudioProcessorEditor (RingModAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // ---- Frequency knob ----
    freqSlider.setLookAndFeel (&freqLAF);
    freqSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    freqSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible (freqSlider);
    // Attach slider to the "freq" APVTS parameter so DAW automation is reflected immediately.
    freqAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.getAPVTS(), "freq", freqSlider);

    // ---- Mix knob ----
    mixSlider.setLookAndFeel (&mixLAF);
    mixSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    mixSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible (mixSlider);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.getAPVTS(), "mix", mixSlider);

    // ---- Waveform selector ComboBox ----
    // Item IDs are 1-based (JUCE ComboBox convention).
    // The order must match the AudioParameterChoice index order in PluginProcessor::createParameterLayout().
    waveformBox.addItem ("Sine",     1);
    waveformBox.addItem ("Saw",      2);
    waveformBox.addItem ("Square",   3);
    waveformBox.addItem ("Triangle", 4);
    waveformBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour::fromRGB (14, 18, 24));
    waveformBox.setColour (juce::ComboBox::textColourId,       juce::Colour::fromRGB (0, 220, 255));
    waveformBox.setColour (juce::ComboBox::outlineColourId,    juce::Colour::fromRGB (36, 56, 72));
    waveformBox.setColour (juce::ComboBox::arrowColourId,      juce::Colour::fromRGB (0, 160, 200));
    waveformBox.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (waveformBox);
    waveformAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        audioProcessor.getAPVTS(), "waveform", waveformBox);

    // Fixed 640×400 window; the plug-in does not support resizing.
    setSize (640, 400);

    // Start the display refresh timer at 30 Hz so the oscilloscope updates smoothly.
    startTimerHz (30);
}

RingModAudioProcessorEditor::~RingModAudioProcessorEditor()
{
    // Detach LookAndFeel objects before they go out of scope to prevent dangling pointers.
    freqSlider.setLookAndFeel (nullptr);
    mixSlider.setLookAndFeel (nullptr);
}

// =============================================================================
// Timer callback — scope FIFO drain + repaint
// =============================================================================

void RingModAudioProcessorEditor::timerCallback()
{
    // Drain all available carrier samples from the processor's lock-free FIFO.
    // We maintain a rolling window of kScopeSize samples in displayBuf by
    // shifting old data left and appending new data on the right — entirely
    // on the message thread, so no extra synchronisation is required here.
    static constexpr int kN = RingModAudioProcessor::kScopeSize;
    float temp[kN];
    int got;

    while ((got = audioProcessor.drainScopeData (temp, kN)) > 0)
    {
        const int keep = kN - got;                            // samples to preserve at the start
        if (keep > 0)
            std::memmove (displayBuf.data(), displayBuf.data() + got, (size_t) keep * sizeof (float));
        std::memcpy (displayBuf.data() + keep, temp, (size_t) got * sizeof (float));
    }

    repaint();
}

// =============================================================================
// Utility
// =============================================================================

/**
 * Maps a frequency value in Hz to a rotary angle.
 *
 * The skew exponent (0.3) mirrors the NormalisableRange skew on the "freq"
 * APVTS parameter, so the tick marks drawn by drawFreqTickMarks() are visually
 * aligned with the knob position at each labelled frequency.
 */
float RingModAudioProcessorEditor::hzToAngle (float hz, float startAngle, float endAngle) noexcept
{
    float proportion = std::pow ((hz - 20.0f) / (2000.0f - 20.0f), 0.3f);
    return startAngle + proportion * (endAngle - startAngle);
}

// =============================================================================
// Paint helpers
// =============================================================================

/** Renders the small JUCE "J" badge in the top-right corner of the header. */
void RingModAudioProcessorEditor::drawJuceLogo (juce::Graphics& g)
{
    float cx = 607.0f, cy = 35.0f, r = 14.0f;

    // Orange circle background
    g.setColour (juce::Colour::fromRGB (255, 140, 0));
    g.fillEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f);

    // "J" initial centred in the circle
    g.setColour (juce::Colours::white.withAlpha (0.9f));
    g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    g.drawText ("J", (int)(cx - r), (int)(cy - r), (int)(r * 2.0f), (int)(r * 2.0f),
                juce::Justification::centred);

    // "JUCE" text label to the left of the circle
    g.setColour (juce::Colour::fromRGB (200, 210, 225));
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.drawText ("JUCE", 548, 27, 52, 18, juce::Justification::right);
}

/**
 * Draws the LED-style digital frequency display in the header area.
 *
 * Two sections:
 *  - A rounded dark box showing the current frequency value in monospaced cyan text.
 *  - A small grey "Range: 20 Hz – 2000 Hz" label to the right of the box.
 */
void RingModAudioProcessorEditor::drawDigitalDisplay (juce::Graphics& g)
{
    float freq = (float) freqSlider.getValue();

    // LED box background and border
    juce::Rectangle<float> box (38.0f, 56.0f, 205.0f, 30.0f);
    g.setColour (juce::Colour::fromRGB (6, 18, 23));
    g.fillRoundedRectangle (box, 4.0f);
    g.setColour (juce::Colour::fromRGB (0, 72, 95));
    g.drawRoundedRectangle (box, 4.0f, 1.0f);

    // Frequency value in Courier New for an LED readout feel
    g.setColour (juce::Colour::fromRGB (0, 240, 255));
    g.setFont (juce::FontOptions ("Courier New", 17.0f, juce::Font::bold));
    g.drawText (juce::String (freq, 2) + " Hz", box.reduced (6.0f, 2.0f), juce::Justification::centred);

    // Range label shown to the right of the LED box
    g.setColour (juce::Colour::fromRGB (88, 108, 128));
    g.setFont (juce::FontOptions (10.5f));
    g.drawText ("Range: 20 Hz - 2000 Hz", 252, 62, 195, 16, juce::Justification::left);
}

/**
 * Draws calibrated tick marks around the outer rim of the frequency knob.
 *
 * Each tick is drawn as a short radial line segment, and each is labelled
 * with its corresponding frequency string.  The angular positions are computed
 * with hzToAngle() so they align precisely with the knob's skewed NormalisableRange.
 */
void RingModAudioProcessorEditor::drawFreqTickMarks (juce::Graphics& g)
{
    // Geometry of the tick ring — must match the freqSlider bounds set in resized().
    const float cx      = 190.0f;   // knob centre x
    const float cy      = 200.0f;   // knob centre y
    const float outerR  = 120.0f;   // outer end of the tick line
    const float innerR  = 111.0f;   // inner end of the tick line
    const float labelR  = 135.0f;   // radius at which the frequency label is centred

    // Tick definitions: frequency in Hz and corresponding display string.
    struct Tick { float hz; const char* label; };
    constexpr Tick ticks[] = {
        { 20.0f,   "20"   },
        { 200.0f,  "200"  },
        { 400.0f,  "400"  },
        { 600.0f,  "600"  },
        { 1000.0f, "1k"   },
        { 1500.0f, "1.5k" },
        { 2000.0f, "kHz"  }   // labelled "kHz" at the 2 000 Hz endpoint
    };

    for (auto& t : ticks)
    {
        float a    = hzToAngle (t.hz, kRotStart, kRotEnd);
        float sinA = std::sin (a), cosA = std::cos (a);

        // Radial tick line
        g.setColour (juce::Colour::fromRGB (70, 86, 102));
        g.drawLine (cx + sinA * innerR, cy - cosA * innerR,
                    cx + sinA * outerR, cy - cosA * outerR, 1.3f);

        // Frequency label centred just outside the tick
        float lx = cx + sinA * labelR;
        float ly = cy - cosA * labelR;
        g.setColour (juce::Colour::fromRGB (110, 132, 152));
        g.setFont (juce::FontOptions (9.0f));
        g.drawText (t.label, (int)(lx - 18), (int)(ly - 7), 36, 14,
                    juce::Justification::centred);
    }

    // "Carrier Frequency" caption below the knob
    g.setColour (juce::Colour::fromRGB (138, 154, 172));
    g.setFont (juce::FontOptions (13.0f));
    g.drawText ("Carrier Frequency", 88, 324, 204, 18, juce::Justification::centred);
}

/**
 * Draws the carrier-waveform oscilloscope panel.
 *
 * Displays the most recent 256 samples from displayBuf as a continuous path
 * with two rendering passes:
 *  - A wide, semi-transparent pass for a soft glow effect.
 *  - A narrow, fully-opaque pass for a crisp trace line.
 *
 * A ScopedSaveState / reduceClipRegion pair clips the waveform path to the
 * inner wave area so it never draws outside the rounded box.
 */
void RingModAudioProcessorEditor::drawOscilloscope (juce::Graphics& g)
{
    juce::Rectangle<float> box (302.0f, 138.0f, 128.0f, 118.0f);

    // Box background and border
    g.setColour (juce::Colour::fromRGB (12, 16, 22));
    g.fillRoundedRectangle (box, 5.0f);
    g.setColour (juce::Colour::fromRGB (36, 46, 58));
    g.drawRoundedRectangle (box, 5.0f, 1.0f);

    // "Carrier Buffer" label at the top of the box
    g.setColour (juce::Colour::fromRGB (78, 98, 118));
    g.setFont (juce::FontOptions (9.5f));
    g.drawText ("Carrier Buffer", box.withHeight (15.0f), juce::Justification::centred);

    // Inner wave area (smaller than the box to leave padding)
    auto wave = box.reduced (6.0f, 18.0f);
    const auto& scope = displayBuf;           // message-thread-only snapshot
    const int   N     = RingModAudioProcessor::kScopeSize;
    const int   disp  = juce::jmin (N, 256);  // render at most 256 points for readability

    // Build the waveform path: sample index → horizontal position, amplitude → vertical position.
    juce::Path waveform;
    float midY  = wave.getCentreY();
    float ampY  = wave.getHeight() * 0.42f;   // scale: ±1 sample maps to ±42% of wave height

    for (int i = 0; i < disp; ++i)
    {
        float px = wave.getX() + (float)i / (float)(disp - 1) * wave.getWidth();
        float py = midY - scope[i] * ampY;
        if (i == 0) waveform.startNewSubPath (px, py);
        else         waveform.lineTo          (px, py);
    }

    {
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (wave.toNearestInt());

        // Glow pass — wide, dim stroke for bloom
        g.setColour (juce::Colour::fromRGB (0, 155, 190).withAlpha (0.28f));
        g.strokePath (waveform, juce::PathStrokeType (4.5f));

        // Main line — narrow, bright stroke for clarity
        g.setColour (juce::Colour::fromRGB (0, 215, 255));
        g.strokePath (waveform, juce::PathStrokeType (1.3f));
    }
}

/**
 * Draws the "Waveform" text label above the ComboBox.
 * The ComboBox itself is a child component; its position is set in resized().
 */
void RingModAudioProcessorEditor::drawWaveformSelector (juce::Graphics& g)
{
    g.setColour (juce::Colour::fromRGB (110, 132, 152));
    g.setFont (juce::FontOptions (10.0f));
    g.drawText ("Waveform", 302, 264, 128, 14, juce::Justification::centred);
}

/**
 * Draws the mix section: value label, percentage readout, and LED progress bar.
 *
 * The progress bar is rendered as a background rounded rectangle followed by
 * a filled rectangle scaled to the current mix value (0 = no fill, 1 = full).
 */
void RingModAudioProcessorEditor::drawMixSection (juce::Graphics& g)
{
    float mix = (float) mixSlider.getValue();
    int   pct = juce::roundToInt (mix * 100.0f);

    // "Mix: X.X" bold label
    g.setColour (juce::Colour::fromRGB (0, 245, 115));
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.drawText ("Mix: " + juce::String (mix, 1), 460, 57, 155, 18, juce::Justification::left);

    // Decimal + percentage readout on the line below
    g.setColour (juce::Colour::fromRGB (95, 118, 138));
    g.setFont (juce::FontOptions (10.0f));
    g.drawText ("Value: " + juce::String (mix, 1) + " (" + juce::String (pct) + "% Wet)",
                460, 76, 165, 14, juce::Justification::left);

    // LED bar background
    juce::Rectangle<float> barBg (460.0f, 94.0f, 148.0f, 10.0f);
    g.setColour (juce::Colour::fromRGB (8, 28, 16));
    g.fillRoundedRectangle (barBg, 3.0f);
    g.setColour (juce::Colour::fromRGB (18, 46, 28));
    g.drawRoundedRectangle (barBg, 3.0f, 1.0f);

    // LED bar active fill — only draw when mix > 0 to avoid a 0-width fill artefact.
    if (mix > 0.0f)
    {
        juce::Rectangle<float> fill = barBg.withWidth (barBg.getWidth() * mix);
        g.setColour (juce::Colour::fromRGB (0, 215, 105));
        g.fillRoundedRectangle (fill, 3.0f);
    }

    // "0%" and "100%" edge labels
    g.setColour (juce::Colour::fromRGB (72, 92, 108));
    g.setFont (juce::FontOptions (9.0f));
    g.drawText ("0%",   432, 92, 26, 12, juce::Justification::right);
    g.drawText ("100%", 610, 92, 30, 12, juce::Justification::left);

    // "Dry/Wet Mix" caption below the mix knob
    g.setColour (juce::Colour::fromRGB (138, 154, 172));
    g.setFont (juce::FontOptions (13.0f));
    g.drawText ("Dry/Wet Mix", 448, 312, 148, 18, juce::Justification::centred);
}

/**
 * Draws the PARAMETERS status strip at the bottom of the plug-in window.
 *
 * Displays:
 *  - A "PARAMETERS" section label
 *  - A cyan-dot + "freq" + current value row
 *  - A green-dot + "mix" + current value row
 *  - Right-aligned status text showing the active waveform and processor state
 */
void RingModAudioProcessorEditor::drawParamsPanel (juce::Graphics& g)
{
    // Dark strip background and top separator line
    g.setColour (juce::Colour::fromRGB (12, 15, 20));
    g.fillRect (0, 350, 640, 50);
    g.setColour (juce::Colour::fromRGB (32, 40, 50));
    g.drawLine (0.0f, 350.0f, 640.0f, 350.0f, 1.0f);

    float freq = (float) freqSlider.getValue();
    float mix  = (float) mixSlider.getValue();

    // Section label
    g.setColour (juce::Colour::fromRGB (60, 76, 92));
    g.setFont (juce::FontOptions (9.5f));
    g.drawText ("PARAMETERS", 16, 354, 80, 12, juce::Justification::left);

    // Cyan dot + "freq" label + current frequency value
    g.setColour (juce::Colour::fromRGB (0, 200, 240));
    g.fillEllipse (16.0f, 369.0f, 7.0f, 7.0f);
    g.setColour (juce::Colour::fromRGB (128, 150, 172));
    g.setFont (juce::FontOptions (10.5f));
    g.drawText ("freq", 28, 365, 36, 15, juce::Justification::left);
    g.setColour (juce::Colour::fromRGB (195, 210, 228));
    g.drawText (juce::String (freq, 1), 66, 365, 55, 15, juce::Justification::left);

    // Green dot + "mix" label + current mix value
    g.setColour (juce::Colour::fromRGB (0, 215, 105));
    g.fillEllipse (16.0f, 383.0f, 7.0f, 7.0f);
    g.setColour (juce::Colour::fromRGB (128, 150, 172));
    g.drawText ("mix", 28, 379, 36, 15, juce::Justification::left);
    g.setColour (juce::Colour::fromRGB (195, 210, 228));
    g.drawText (juce::String (mix, 1), 66, 379, 55, 15, juce::Justification::left);

    // Right side: active waveform name and processor readiness indicator
    g.setColour (juce::Colour::fromRGB (58, 76, 94));
    g.setFont (juce::FontOptions (9.5f));
    g.drawText ("Waveform: " + waveformBox.getText() + "  |  Phase Accumulator", 200, 365, 310, 14, juce::Justification::left);
    g.drawText ("carrierBuffer (Ready)",                                          200, 379, 200, 14, juce::Justification::left);
}

// =============================================================================
// paint / resized
// =============================================================================

void RingModAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Flat dark background for the whole plug-in window
    g.fillAll (juce::Colour::fromRGB (21, 25, 31));

    // Outer rounded-rectangle frame
    {
        juce::Path frame;
        frame.addRoundedRectangle (getLocalBounds().toFloat().reduced (4.0f), 7.0f);
        g.setColour (juce::Colour::fromRGB (36, 44, 54));
        g.strokePath (frame, juce::PathStrokeType (2.0f));
    }

    // Header gradient band (top 74 px fades from a slightly lighter dark to the background colour)
    {
        juce::ColourGradient hdr (juce::Colour::fromRGB (28, 34, 43), 0.0f, 0.0f,
                                   juce::Colour::fromRGB (21, 25, 31), 0.0f, 74.0f, false);
        g.setGradientFill (hdr);
        g.fillRect (0, 0, getWidth(), 74);
    }
    // Thin separator line between the header gradient and the body
    g.setColour (juce::Colour::fromRGB (36, 44, 55));
    g.drawLine (12.0f, 74.0f, 628.0f, 74.0f, 1.0f);

    // Plug-in title
    g.setColour (juce::Colour::fromRGB (198, 212, 228));
    g.setFont (juce::FontOptions ("Helvetica Neue", 20.0f, juce::Font::bold));
    g.drawText ("Ring Modulator", 35, 17, 390, 28, juce::Justification::left);

    // Subtitle — sits at y=42 so it clears the LED frequency box which starts at y=56
    g.setColour (juce::Colour::fromRGB (75, 95, 115));
    g.setFont (juce::FontOptions (10.5f));
    g.drawText ("JUCE DSP  |  Phase Accumulator", 35, 42, 220, 12, juce::Justification::left);

    // Delegate each UI region to its dedicated helper
    drawJuceLogo        (g);
    drawDigitalDisplay  (g);
    drawFreqTickMarks   (g);
    drawOscilloscope    (g);
    drawWaveformSelector(g);
    drawMixSection      (g);
    drawParamsPanel     (g);
    // Note: GlowKnobLookAndFeel renders the knob visuals directly during Slider::paint(),
    // so no explicit call is needed here.
}

void RingModAudioProcessorEditor::resized()
{
    // Freq knob: large rotary covering a 224×230 region; visual centre lands at (190, 200).
    freqSlider.setBounds (78, 85, 224, 230);

    // Mix knob: smaller rotary; visual centre lands at approximately (516, 215).
    mixSlider.setBounds (448, 130, 136, 170);

    // Waveform ComboBox: positioned below the oscilloscope box (which ends at y ≈ 256).
    waveformBox.setBounds (302, 280, 128, 26);
}
