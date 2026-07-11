#include "PluginEditor.h"

// Rotary sweep: 225° start (≈ 7-o'clock) to 495° end (≈ 5-o'clock), matching JUCE's default.
// kRotStart / kRotEnd are used by drawRotarySlider via startAngle / endAngle parameters.
static const float kRotStart = juce::MathConstants<float>::pi * 1.25f;
static const float kRotEnd   = juce::MathConstants<float>::pi * 2.75f;

// =============================================================================
// FuturisticLookAndFeel
// =============================================================================

/**
 * Sets global theme colours for ComboBox and PopupMenu components.
 * These apply to every instance of these components that inherits this LAF,
 * so individual colour overrides on child controls are not needed.
 */
FuturisticLookAndFeel::FuturisticLookAndFeel()
{
    setColour(juce::ComboBox::backgroundColourId, juce::Colour::fromRGB(12, 30, 50));
    setColour(juce::ComboBox::textColourId, juce::Colour::fromRGB(0, 200, 255));
    setColour(juce::ComboBox::outlineColourId, juce::Colour::fromRGB(30, 74, 106));
    setColour(juce::ComboBox::arrowColourId, juce::Colour::fromRGB(0, 200, 255));
    setColour(juce::PopupMenu::backgroundColourId, juce::Colour::fromRGB(10, 24, 40));
    setColour(juce::PopupMenu::textColourId, juce::Colour::fromRGB(74, 112, 144));
    setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour::fromRGB(0, 200, 255).withAlpha(0.08f));
    setColour(juce::PopupMenu::highlightedTextColourId, juce::Colour::fromRGB(0, 200, 255));
}

/**
 * Renders the rotary knob in seven visual layers (back to front):
 *
 *  1. Outer radial glow   — large translucent gradient fill centred on the knob
 *  2. Outer bezel          — dark filled ellipse + single-pixel dim border
 *  3. Inner gradient body  — raised dark-to-darker radial gradient face
 *  4. Track arc            — full-range dim arc at 15% of radius stroke width
 *  5. Value arc (glow)     — wide (25% radius), semi-transparent accent arc
 *  6. Value arc (sharp)    — narrow (15% radius), fully opaque accent arc
 *  7. Needle               — thin line rotated to the current value angle
 *  8. Centre cap           — small dot with accent ring for a hardware feel
 *
 * Accent colour is resolved from slider.getName():
 *   "freq" → cyan  (#00C8FF)
 *   any other name → green (#00FF88)
 */
void FuturisticLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                              float sliderPos, float startAngle, float endAngle,
                                              juce::Slider& slider)
{
    // Reduce bounding box to leave room for the outer glow without clipping.
    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(10.0f);
    float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    float cx = bounds.getCentreX();
    float cy = bounds.getCentreY();

    // Accent colour depends on which parameter this slider controls.
    juce::Colour accent = slider.getName() == "freq"
        ? juce::Colour::fromRGB(0, 200, 255)   // cyan for carrier frequency
        : juce::Colour::fromRGB(0, 255, 136);  // green for dry/wet mix
    float angle = startAngle + sliderPos * (endAngle - startAngle);

    // --- Layer 1: outer radial glow halo ---
    juce::ColourGradient rimGrad(juce::Colours::transparentBlack, cx, cy,
                                  accent.withAlpha(0.12f), cx, cy - radius, true);
    g.setGradientFill(rimGrad);
    g.fillEllipse(cx - radius - 8, cy - radius - 8, (radius + 8) * 2, (radius + 8) * 2);

    // --- Layer 2: outer bezel (dark filled ring + dim border) ---
    g.setColour(juce::Colour::fromRGB(8, 12, 20));
    g.fillEllipse(cx - radius - 5, cy - radius - 5, (radius + 5) * 2, (radius + 5) * 2);
    g.setColour(juce::Colour::fromRGB(28, 40, 64));
    g.drawEllipse(cx - radius - 5, cy - radius - 5, (radius + 5) * 2, (radius + 5) * 2, 1.5f);

    // --- Layer 3: inner gradient knob face ---
    float innerR = radius * 0.7f;
    juce::ColourGradient bodyGrad(juce::Colour::fromRGB(36, 46, 62), cx - innerR * 0.3f, cy - innerR * 0.3f,
                                  juce::Colour::fromRGB(8, 12, 20), cx + innerR, cy + innerR, true);
    g.setGradientFill(bodyGrad);
    g.fillEllipse(cx - innerR, cy - innerR, innerR * 2, innerR * 2);

    // --- Layer 4: track arc (full range, dim) ---
    juce::Path trackArc;
    trackArc.addCentredArc(cx, cy, radius, radius, 0.0f, startAngle, endAngle, true);
    g.setColour(juce::Colour::fromRGB(18, 28, 44));
    g.strokePath(trackArc, juce::PathStrokeType(radius * 0.15f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // --- Layers 5 + 6: value arc (only drawn when value > 0 to avoid a zero-width stroke) ---
    if (sliderPos > 0.0f)
    {
        juce::Path valArc;
        valArc.addCentredArc(cx, cy, radius, radius, 0.0f, startAngle, angle, true);

        // Wide, dim pass for the bloom/glow effect
        g.setColour(accent.withAlpha(0.3f));
        g.strokePath(valArc, juce::PathStrokeType(radius * 0.25f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Narrow, bright pass for the crisp active arc
        g.setColour(accent);
        g.strokePath(valArc, juce::PathStrokeType(radius * 0.15f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // --- Layer 7: needle (thin bar rotated to current angle) ---
    juce::Path needle;
    needle.addLineSegment(juce::Line<float>(0, -innerR * 0.2f, 0, -innerR * 0.9f), radius * 0.04f);
    g.setColour(juce::Colour::fromRGB(220, 235, 255));
    g.fillPath(needle, juce::AffineTransform::rotation(angle).translated(cx, cy));

    // --- Layer 8: centre cap (dark dot + accent ring + bright inner dot) ---
    g.setColour(juce::Colour::fromRGB(7, 9, 14));
    g.fillEllipse(cx - innerR * 0.2f, cy - innerR * 0.2f, innerR * 0.4f, innerR * 0.4f);
    g.setColour(accent.withAlpha(0.8f));
    g.drawEllipse(cx - innerR * 0.2f, cy - innerR * 0.2f, innerR * 0.4f, innerR * 0.4f, 1.2f);
    g.setColour(accent.withAlpha(0.5f));
    g.fillEllipse(cx - innerR * 0.1f, cy - innerR * 0.1f, innerR * 0.2f, innerR * 0.2f);
}

/**
 * Draws the combo box background and a custom downward-pointing arrow.
 *
 * The background is a subtle top-to-bottom gradient to give a slight 3D lift.
 * The arrow is a filled triangle in the right margin, coloured with 70% opacity
 * cyan to distinguish it from the text without being distracting.
 */
void FuturisticLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool,
                                          int, int, int, int, juce::ComboBox&)
{
    auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat();

    // Gradient background
    juce::ColourGradient bgGrad(juce::Colour::fromRGB(12, 30, 50), 0, 0,
                                juce::Colour::fromRGB(8, 22, 40), 0, (float)height, false);
    g.setGradientFill(bgGrad);
    g.fillRoundedRectangle(bounds, 3.0f);

    // Border
    g.setColour(juce::Colour::fromRGB(30, 74, 106));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 3.0f, 1.0f);

    // Downward-pointing triangle arrow
    juce::Path path;
    float arrowW = 8.0f;
    float arrowH = 5.0f;
    float cx = width - 16.0f;
    float cy = height * 0.5f;
    path.addTriangle(cx - arrowW * 0.5f, cy - arrowH * 0.5f,
                     cx + arrowW * 0.5f, cy - arrowH * 0.5f,
                     cx, cy + arrowH * 0.5f);
    g.setColour(juce::Colour::fromRGB(0, 200, 255).withAlpha(0.7f));
    g.fillPath(path);
}

/**
 * Positions the combo box text label to the left of the custom arrow (30 px margin)
 * and sets a bold 14 pt font for legibility in the small selector control.
 */
void FuturisticLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (1, 1, box.getWidth() - 30, box.getHeight() - 2);
    label.setFont (juce::FontOptions(14.0f).withStyle ("Bold"));
}

/**
 * Draws the horizontal LED mix bar (LinearBar style slider).
 *
 * Layers:
 *  1. Dark rounded track (background)
 *  2. Left-to-right gradient fill up to the current position
 *  3. A bright 1.5 px vertical edge line at the fill boundary for a crisp end cap
 *
 * The accent colour follows the same slider.getName() convention as the rotary.
 */
void FuturisticLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                              float sliderPos, float minSliderPos, float maxSliderPos,
                                              const juce::Slider::SliderStyle, juce::Slider& slider)
{
    juce::ignoreUnused (minSliderPos, maxSliderPos);

    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();
    juce::Colour accent = slider.getName() == "freq"
        ? juce::Colour::fromRGB(0, 200, 255)
        : juce::Colour::fromRGB(0, 255, 136);

    // --- Track (full width, dark background) ---
    g.setColour(juce::Colour::fromRGB(5, 14, 8));
    g.fillRoundedRectangle(bounds, 2.0f);
    // Dim border blended toward a dark teal so it's visible but not distracting
    g.setColour(accent.withAlpha(0.35f).interpolatedWith(juce::Colour::fromRGB(20, 56, 40), 0.5f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 2.0f, 1.0f);

    // --- Active fill (proportional to value) ---
    // sliderPos is the pixel x-position of the thumb within [bounds.getX(), bounds.getRight()].
    float fillW = juce::jlimit(0.0f, bounds.getWidth(), sliderPos - bounds.getX());
    if (fillW > 0.5f)
    {
        auto fillBounds = bounds.withWidth(fillW);
        // Gradient: slightly dimmer at the left, full accent at the right edge
        juce::ColourGradient fillGrad(accent.withAlpha(0.55f), fillBounds.getX(), 0,
                                      accent, fillBounds.getRight(), 0, false);
        g.setGradientFill(fillGrad);
        g.fillRoundedRectangle(fillBounds, 2.0f);

        // Edge line for a sharp right boundary on the fill
        g.setColour(accent.withAlpha(0.5f));
        g.drawLine(fillBounds.getRight(), bounds.getY(), fillBounds.getRight(), bounds.getBottom(), 1.5f);
    }
}

// =============================================================================
// RingModAudioProcessorEditor — constructor / destructor
// =============================================================================

RingModAudioProcessorEditor::RingModAudioProcessorEditor (RingModAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Apply the futuristic LAF globally to this component and all its children.
    setLookAndFeel(&customLAF);

    // ---- Carrier frequency rotary knob ----
    // Named "freq" so FuturisticLookAndFeel renders the cyan accent.
    freqSlider.setName("freq");
    freqSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    freqSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    freqSlider.setRange(20, 2000);  // explicit range mirrors the APVTS parameter range
    addAndMakeVisible(freqSlider);
    freqAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "freq", freqSlider);

    // ---- Dry/wet mix rotary knob ----
    // Named "mix" so FuturisticLookAndFeel renders the green accent.
    mixSlider.setName("mix");
    mixSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    mixSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(mixSlider);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "mix", mixSlider);

    // ---- Mix LED bar slider (horizontal strip showing the same mix value) ----
    // Attached to the same "mix" parameter as mixSlider; dragging either control
    // updates both.  Uses LinearBar style so FuturisticLookAndFeel::drawLinearSlider
    // is called instead of the rotary renderer.
    mixBarSlider.setName("mix");
    mixBarSlider.setSliderStyle(juce::Slider::LinearBar);
    mixBarSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(mixBarSlider);
    mixBarAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "mix", mixBarSlider);

    // Populate waveform choices directly from the AudioParameterChoice so the
    // editor list and the parameter definition can never silently diverge.
    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*> (
            audioProcessor.getAPVTS().getParameter ("waveform")))
    {
        for (int i = 0; i < choiceParam->choices.size(); ++i)
            waveformBox.addItem (choiceParam->choices[i], i + 1);
    }
    waveformBox.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (waveformBox);
    waveformAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        audioProcessor.getAPVTS(), "waveform", waveformBox);

    // Fixed 700×470 window — the plug-in does not support resizing.
    setSize(700, 470);

    // Start the display refresh timer at 30 Hz so the oscilloscope updates smoothly.
    startTimerHz(30);
}

RingModAudioProcessorEditor::~RingModAudioProcessorEditor()
{
    // Reset the global LAF pointer before customLAF goes out of scope
    // to prevent JUCE from holding a dangling pointer to the destroyed object.
    setLookAndFeel(nullptr);
}

// =============================================================================
// Timer callback — scope FIFO drain + repaint
// =============================================================================

void RingModAudioProcessorEditor::timerCallback()
{
    // Cache display parameter values on the message thread so paint() stays
    // free of APVTS map lookups on every frame.
    auto& apvts = audioProcessor.getAPVTS();
    cachedFreqHz      = apvts.getRawParameterValue ("freq")->load (std::memory_order_relaxed);
    cachedMixVal      = apvts.getRawParameterValue ("mix")->load  (std::memory_order_relaxed);
    cachedWaveformIdx = juce::jlimit (0, 3, (int) std::round (
        apvts.getRawParameterValue ("waveform")->load (std::memory_order_relaxed)));

    // Drain at most one frame's worth of samples so the oscilloscope time scale
    // stays consistent regardless of the host's sample rate (Fix #7).
    // At 44100 Hz / 30 Hz ≈ 1470 samples/frame; at 96000 Hz ≈ 3200 samples/frame.
    static constexpr int kN = RingModAudioProcessor::kScopeSize;
    const double sr = audioProcessor.getSampleRate();
    const int samplesPerFrame = (sr > 0.0) ? juce::jmax (1, (int) (sr / 30.0)) : kN;
    float temp[kN];
    int totalDrained = 0;
    int got;
    while (totalDrained < samplesPerFrame
        && (got = audioProcessor.drainScopeData (temp, juce::jmin (kN, samplesPerFrame - totalDrained))) > 0)
    {
        const int keep = kN - got;
        if (keep > 0)
            std::memmove (displayBuf.data(), displayBuf.data() + got, (size_t) keep * sizeof (float));
        std::memcpy (displayBuf.data() + keep, temp, (size_t) got * sizeof (float));
        totalDrained += got;
    }

    ++frameCounter;
    repaint();
}


// =============================================================================
// paint
// =============================================================================

void RingModAudioProcessorEditor::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // =========================================================================
    // Background layer
    // =========================================================================

    // --- Solid near-black base ---
    g.fillAll (juce::Colour::fromRGB(4, 6, 12));

    // --- Colour blooms: radial gradients that give the background colour depth ---
    // Each bloom is drawn as a full-rect gradient fill so they additively overlay.
    auto glow = [&] (float cxProp, float cyProp, float rProp, juce::Colour c)
    {
        float cx = (float) getWidth()  * cxProp;
        float cy = (float) getHeight() * cyProp;
        float r  = (float) juce::jmax(getWidth(), getHeight()) * rProp;
        juce::ColourGradient grad(c, cx, cy, juce::Colours::transparentBlack, cx + r, cy, true);
        g.setGradientFill(grad);
        g.fillRect(bounds);
    };
    glow(0.22f, 0.48f, 0.48f, juce::Colour::fromRGB(0, 170, 255).withAlpha(0.16f));   // cyan bloom (left/centre)
    glow(0.75f, 0.60f, 0.44f, juce::Colour::fromRGB(102, 0, 221).withAlpha(0.15f));  // purple bloom (right)
    glow(0.82f, 0.12f, 0.32f, juce::Colour::fromRGB(68, 0, 187).withAlpha(0.11f));   // violet bloom (top-right)
    glow(0.10f, 0.88f, 0.30f, juce::Colour::fromRGB(0, 85, 204).withAlpha(0.10f));   // blue bloom (bottom-left)

    // --- Diagonal metallic streaks (three passes at different angles and spacing) ---
    // These simulate the brushed-metal / holographic foil texture of real hardware panels.
    g.setColour(juce::Colour::fromRGB(0, 80, 160).withAlpha(0.05f));
    for (int i = -getHeight(); i < getWidth() + getHeight(); i += 38)
        g.drawLine((float) i, 0.0f, (float) i + (float) getHeight() * 0.32f, (float) getHeight(), 1.0f);
    g.setColour(juce::Colour::fromRGB(40, 0, 100).withAlpha(0.045f));
    for (int i = -getHeight(); i < getWidth() + getHeight(); i += 52)
        g.drawLine((float) i, 0.0f, (float) i + (float) getHeight() * 0.40f, (float) getHeight(), 1.0f);
    g.setColour(juce::Colour::fromRGB(0, 160, 255).withAlpha(0.025f));
    for (int i = -getHeight(); i < getWidth() + getHeight(); i += 70)
        g.drawLine((float) i, 0.0f, (float) i + (float) getHeight() * 0.27f, (float) getHeight(), 1.0f);

    // --- Vignette: darkens the edges of the panel to draw focus to the centre ---
    {
        float cx = (float) getWidth() * 0.5f, cy = (float) getHeight() * 0.5f;
        float r  = (float) juce::jmax(getWidth(), getHeight()) * 0.62f;
        juce::ColourGradient vignette(juce::Colours::transparentBlack, cx, cy,
                                      juce::Colour::fromRGB(0, 0, 8).withAlpha(0.55f), cx + r, cy, true);
        g.setGradientFill(vignette);
        g.fillRect(bounds);
    }

    // =========================================================================
    // Header: title bar + subtitle strip
    // =========================================================================

    // --- Title bar (top 40 px) ---
    juce::Rectangle<int> titleBar(0, 0, getWidth(), 40);
    g.setColour(juce::Colour::fromRGB(4, 8, 18).withAlpha(0.8f));
    g.fillRect(titleBar);
    g.setColour(juce::Colour::fromRGB(24, 40, 64));
    g.fillRect(0, 39, getWidth(), 1);  // 1 px separator line

    g.setColour(juce::Colour::fromRGB(184, 212, 238));
    g.setFont(juce::FontOptions(18.0f).withStyle ("Bold"));
    g.drawText("RING MOD", titleBar, juce::Justification::centred);

    // "JUCE" badge in the top-right corner of the title bar
    g.setColour(juce::Colour::fromRGB(40, 64, 96));
    g.setFont(juce::FontOptions(12.0f).withStyle ("Bold"));
    g.drawText("JUCE", titleBar.reduced(16, 0), juce::Justification::centredRight);

    // --- Subtitle strip (next 20 px, below title bar) ---
    juce::Rectangle<int> subStrip(0, 40, getWidth(), 20);
    g.setColour(juce::Colour::fromRGB(3, 6, 14).withAlpha(0.7f));
    g.fillRect(subStrip);
    g.setColour(juce::Colour::fromRGB(14, 27, 44));
    g.fillRect(0, 59, getWidth(), 1);  // 1 px separator line

    g.setColour(juce::Colour::fromRGB(46, 80, 112));
    g.setFont(juce::FontOptions(11.0f).withStyle ("Bold"));
    g.drawText("JUCE DSP | Phase Accumulator", subStrip.withTrimmedLeft(16), juce::Justification::centredLeft);

    // =========================================================================
    // Three-column content layout
    // =========================================================================
    // The content area sits between the header (60 px) and the footer (34 px),
    // with 24 px horizontal and 20 px vertical padding.
    // The width is split equally into three columns with 16 px gutters.

    // Column bounds are pre-computed in resized(); take local mutable copies here
    // so removeFromTop() calls below do not mutate the stored member rectangles.
    auto leftCol   = colLeft;
    auto centerCol = colCenter;
    auto rightCol  = colRight;

    // =========================================================================
    // LEFT COLUMN — Carrier frequency knob + digital readout
    // =========================================================================

    // Frequency LED readout box (dark rectangle at the top of the column)
    auto freqBox = leftCol.removeFromTop(40);
    g.setColour(juce::Colour::fromRGB(2, 4, 8));
    g.fillRect(freqBox);
    g.setColour(juce::Colour::fromRGB(26, 56, 80));
    g.drawRect(freqBox, 1);

    // Small frame counter in the top-right corner of the box — confirms the UI is live.
    // Shows a 3-digit zero-padded number that cycles 000–999 at 30 fps.
    g.setColour(juce::Colour::fromRGB(30, 64, 96));
    g.setFont(juce::FontOptions(8.0f));
    g.drawText(juce::String(frameCounter % 1000).paddedLeft('0', 3), freqBox.reduced(10, 5), juce::Justification::topRight);

    // Frequency value: show "X.XX kHz" when ≥ 1000 Hz, otherwise "X.XX Hz".
    float freqHz = cachedFreqHz;
    juce::String freqStr = freqHz >= 1000.0f
        ? juce::String(freqHz / 1000.0f, 2) + " kHz"
        : juce::String(freqHz, 2) + " Hz";
    g.setColour(juce::Colour::fromRGB(0, 200, 255));
    g.setFont(juce::FontOptions(26.0f));
    g.drawText(freqStr, freqBox, juce::Justification::centred);

    leftCol.removeFromTop(8);
    g.setColour(juce::Colour::fromRGB(32, 64, 80));
    g.setFont(juce::FontOptions(9.0f));
    g.drawText("Range: 20 Hz - 2000 Hz", leftCol.removeFromTop(16), juce::Justification::topLeft);

    leftCol.removeFromTop(10);
    leftCol.removeFromTop(152);  // freqSlider knob component paints itself here (see resized())

    leftCol.removeFromTop(10);
    g.setColour(juce::Colour::fromRGB(58, 104, 128));
    g.setFont(juce::FontOptions(12.0f).withStyle ("Bold"));
    g.drawText("CARRIER FREQUENCY", leftCol.removeFromTop(18), juce::Justification::centred);

    // Min/max frequency labels below the knob caption
    leftCol.removeFromTop(8);
    auto minMaxRow = leftCol.removeFromTop(14);
    g.setColour(juce::Colour::fromRGB(30, 64, 96));
    g.setFont(juce::FontOptions(8.0f));
    g.drawText("20", minMaxRow, juce::Justification::centredLeft);
    g.drawText("kHz", minMaxRow, juce::Justification::centredRight);

    // =========================================================================
    // CENTRE COLUMN — Oscilloscope + waveform selector
    // =========================================================================

    // "CARRIER BUFFER" section label
    g.setColour(juce::Colour::fromRGB(40, 72, 96));
    g.setFont(juce::FontOptions(9.0f).withStyle ("Bold"));
    g.drawText("CARRIER BUFFER", centerCol.removeFromTop(16), juce::Justification::centred);

    centerCol.removeFromTop(10);

    // Oscilloscope box — 168×128 px, centred in the column
    auto scopeBox = centerCol.removeFromTop(128).withSizeKeepingCentre(168, 128);
    g.setColour(juce::Colour::fromRGB(1, 4, 8));
    g.fillRect(scopeBox);
    g.setColour(juce::Colour::fromRGB(24, 46, 64));
    g.drawRect(scopeBox, 1);

    // --- Oscilloscope grid lines ---
    // Horizontal lines every 32 px (excluding the first)
    g.setColour(juce::Colour::fromRGB(12, 30, 42));
    for (int y = 32; y < 128; y += 32)
        g.drawLine((float)scopeBox.getX(), (float)(scopeBox.getY() + y),
                   (float)scopeBox.getRight(), (float)(scopeBox.getY() + y));
    // Vertical lines every 42 px (excluding the first)
    for (int x = 42; x < 168; x += 42)
        g.drawLine((float)(scopeBox.getX() + x), (float)scopeBox.getY(),
                   (float)(scopeBox.getX() + x), (float)scopeBox.getBottom());
    // Centre (zero-amplitude) horizontal rule — slightly brighter than the grid
    g.setColour(juce::Colour::fromRGB(22, 40, 56));
    g.drawLine((float)scopeBox.getX(), (float)(scopeBox.getY() + 64),
               (float)scopeBox.getRight(), (float)(scopeBox.getY() + 64));

    // Corner registration tick squares (decorative hardware aesthetic)
    g.setColour(juce::Colour::fromRGB(30, 64, 96));
    g.drawRect(scopeBox.getX() + 4,          scopeBox.getY() + 4,           6, 6, 1);
    g.drawRect(scopeBox.getRight() - 10,      scopeBox.getY() + 4,           6, 6, 1);
    g.drawRect(scopeBox.getX() + 4,          scopeBox.getBottom() - 10,     6, 6, 1);
    g.drawRect(scopeBox.getRight() - 10,      scopeBox.getBottom() - 10,     6, 6, 1);

    // --- Waveform trace (two-pass: wide glow then sharp line) ---
    juce::Path wavePath;
    float scopeW  = (float)scopeBox.getWidth();
    float scopeH  = scopeBox.getHeight() * 0.5f;
    float scopeCy = (float)scopeBox.getCentreY();
    float scopeX  = (float)scopeBox.getX();

    bool started = false;
    for (int i = 0; i < RingModAudioProcessor::kScopeSize; ++i)
    {
        float x = scopeX + ((float)i / RingModAudioProcessor::kScopeSize) * scopeW;
        float y = scopeCy - displayBuf[i] * scopeH * 0.9f;  // 90% of half-height so trace never clips
        if (!started) { wavePath.startNewSubPath(x, y); started = true; }
        else wavePath.lineTo(x, y);
    }

    // Glow pass
    g.setColour(juce::Colour::fromRGB(0, 200, 255).withAlpha(0.2f));
    g.strokePath(wavePath, juce::PathStrokeType(4.0f));
    // Sharp line pass
    g.setColour(juce::Colour::fromRGB(0, 200, 255));
    g.strokePath(wavePath, juce::PathStrokeType(1.5f));

    centerCol.removeFromTop(10);
    g.setColour(juce::Colour::fromRGB(32, 64, 80));
    g.setFont(juce::FontOptions(10.0f));
    g.drawText("Waveform", centerCol.removeFromTop(14), juce::Justification::centredLeft);
    // waveformBox ComboBox component paints itself in the space reserved by resized()

    // =========================================================================
    // RIGHT COLUMN — Dry/wet mix knob + LED bar + readouts
    // =========================================================================

    float mixVal = cachedMixVal;
    int   mixPct = juce::roundToInt(mixVal * 100.0f);

    // "MIX:" label + large decimal value on the same row
    auto mixHeader = rightCol.removeFromTop(26);
    g.setColour(juce::Colour::fromRGB(46, 112, 80));
    g.setFont(juce::FontOptions(12.0f).withStyle ("Bold"));
    g.drawText("MIX:", mixHeader.removeFromLeft(40), juce::Justification::centredLeft);

    g.setColour(juce::Colour::fromRGB(0, 255, 136));
    g.setFont(juce::FontOptions(22.0f));
    g.drawText(juce::String(mixVal, 2), mixHeader, juce::Justification::centredRight);

    // "Value: X.X (XX% Wet)" detailed readout
    rightCol.removeFromTop(6);
    g.setColour(juce::Colour::fromRGB(35, 80, 64));
    g.setFont(juce::FontOptions(9.0f));
    g.drawText("Value: " + juce::String(mixVal, 1) + " (" + juce::String(mixPct) + "% Wet)",
                rightCol.removeFromTop(14), juce::Justification::topLeft);

    // "0%" / "100%" edge labels above the LED bar
    rightCol.removeFromTop(10);
    auto mixLabelsRow = rightCol.removeFromTop(12);
    g.setColour(juce::Colour::fromRGB(26, 56, 48));
    g.setFont(juce::FontOptions(7.0f));
    g.drawText("0%",   mixLabelsRow, juce::Justification::centredLeft);
    g.drawText("100%", mixLabelsRow, juce::Justification::centredRight);

    rightCol.removeFromTop(4);
    rightCol.removeFromTop(9);    // mixBarSlider paints itself here (see resized())
    rightCol.removeFromTop(14);
    rightCol.removeFromTop(116);  // mixSlider rotary knob paints itself here (see resized())

    // "DRY/WET MIX" caption below the knob
    rightCol.removeFromTop(8);
    g.setColour(juce::Colour::fromRGB(46, 96, 80));
    g.setFont(juce::FontOptions(10.0f).withStyle ("Bold"));
    g.drawText("DRY/WET MIX", rightCol.removeFromTop(16), juce::Justification::centred);

    // =========================================================================
    // Footer status bar
    // =========================================================================

    juce::Rectangle<int> footer(0, getHeight() - 34, getWidth(), 34);
    g.setColour(juce::Colour::fromRGB(2, 4, 10).withAlpha(0.68f));
    g.fillRect(footer);
    g.setColour(juce::Colour::fromRGB(14, 27, 44));
    g.fillRect(0, footer.getY(), getWidth(), 1);  // top separator

    auto footerContent = footer.reduced(16, 0);

    // "PARAMETERS" section label
    g.setColour(juce::Colour::fromRGB(30, 48, 80));
    g.setFont(juce::FontOptions(8.0f).withStyle ("Bold"));
    g.drawText("PARAMETERS", footerContent.removeFromLeft(80), juce::Justification::centredLeft);

    // Helper lambda: renders a coloured pip dot + "label value" text in the footer.
    auto drawPip = [&] (juce::Rectangle<int> area, juce::Colour c, const juce::String& label, const juce::String& value)
    {
        auto dotArea = area.removeFromLeft(10);
        g.setColour(c);
        g.fillEllipse(dotArea.withSizeKeepingCentre(6, 6).toFloat());
        area.removeFromLeft(4);
        g.setFont(juce::FontOptions(8.0f));
        g.setColour(juce::Colour::fromRGB(42, 80, 96));
        g.drawText(label + " " + value, area, juce::Justification::centredLeft);
    };
    drawPip(footerContent.removeFromLeft(90), juce::Colour::fromRGB(0, 200, 255), "freq", juce::String(freqHz, 1));
    drawPip(footerContent.removeFromLeft(90), juce::Colour::fromRGB(0, 255, 136), "mix",  juce::String(mixVal, 1));

    // Right-aligned status text: active waveform + processor readiness
    static const char* waveformNames[] = { "Sine", "Saw", "Square", "Triangle" };
    const int waveformIdx = cachedWaveformIdx;
    g.setColour(juce::Colour::fromRGB(30, 56, 72));
    g.setFont(juce::FontOptions(8.0f));
    g.drawText(juce::String("Waveform: ") + waveformNames[waveformIdx] + " | Phase Accumulator | carrierBuffer (Ready)",
               footerContent, juce::Justification::centredRight);

    // =========================================================================
    // Overlay layers (drawn over all content)
    // =========================================================================

    // --- Scanline effect: horizontal 1 px lines every 4 px at 10% black ---
    // Mimics the sub-pixel structure of CRT / LCD panels for a retro-tech feel.
    g.setColour(juce::Colours::black.withAlpha(0.10f));
    for (int y = 0; y < getHeight(); y += 4)
        g.drawLine(0.0f, (float) y, (float) getWidth(), (float) y, 1.0f);

    // --- Corner accent brackets (L-shaped lines in the four corners) ---
    // Pure decorative detail that frames the panel with a cyan "sight" motif.
    g.setColour(juce::Colour::fromRGB(0, 200, 255).withAlpha(0.33f));
    // Top-left
    g.drawLine(1.0f,                  1.0f,  13.0f,                 1.0f,  1.0f);
    g.drawLine(1.0f,                  1.0f,  1.0f,                  13.0f, 1.0f);
    // Top-right
    g.drawLine((float)getWidth()-13.0f, 1.0f,  (float)getWidth()-1.0f, 1.0f,  1.0f);
    g.drawLine((float)getWidth()-1.0f,  1.0f,  (float)getWidth()-1.0f, 13.0f, 1.0f);
    // Bottom-left
    g.drawLine(1.0f,                  (float)getHeight()-1.0f,  13.0f,                 (float)getHeight()-1.0f, 1.0f);
    g.drawLine(1.0f,                  (float)getHeight()-13.0f, 1.0f,                  (float)getHeight()-1.0f, 1.0f);
    // Bottom-right
    g.drawLine((float)getWidth()-13.0f, (float)getHeight()-1.0f,  (float)getWidth()-1.0f, (float)getHeight()-1.0f, 1.0f);
    g.drawLine((float)getWidth()-1.0f,  (float)getHeight()-13.0f, (float)getWidth()-1.0f, (float)getHeight()-1.0f, 1.0f);
}

// =============================================================================
// resized
// =============================================================================

void RingModAudioProcessorEditor::resized()
{
    // Compute the 3-column layout geometry once and store it in member variables
    // so paint() can read the column bounds directly instead of recomputing them.
    auto contentArea = getLocalBounds().withTrimmedTop(60).withTrimmedBottom(34).reduced(24, 20);
    const int colWidth = (contentArea.getWidth() - 32) / 3;

    colLeft   = contentArea.removeFromLeft (colWidth);
    contentArea.removeFromLeft (16);   // gutter
    colCenter = contentArea.removeFromLeft (colWidth);
    contentArea.removeFromLeft (16);   // gutter
    colRight  = contentArea;

    // Use local copies for component placement (consuming removeFromTop() calls).
    // ---- Left column: freq knob ----
    auto lc = colLeft;
    lc.removeFromTop (74);   // readout box + range label + padding
    freqSlider.setBounds (lc.removeFromTop (152).withSizeKeepingCentre (152, 152));

    // ---- Centre column: waveform ComboBox ----
    auto cc = colCenter;
    cc.removeFromTop (186);  // section label + scope box + padding
    waveformBox.setBounds (cc.removeFromTop (36).withSizeKeepingCentre (168, 36));

    // ---- Right column: mix LED bar + mix knob ----
    auto rc = colRight;
    rc.removeFromTop (72);   // header text rows + labels
    mixBarSlider.setBounds (rc.removeFromTop (9));
    rc.removeFromTop (14);   // padding
    mixSlider.setBounds (rc.removeFromTop (116).withSizeKeepingCentre (116, 116));
}

