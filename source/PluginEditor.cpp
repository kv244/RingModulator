#include "PluginProcessor.h"
#include "PluginEditor.h"

// JUCE default rotary angles (clockwise from 12-o'clock)
static const float kRotStart = juce::MathConstants<float>::pi * 1.2f;  // 216°
static const float kRotEnd   = juce::MathConstants<float>::pi * 2.8f;  // 504°

// ========================= GlowKnobLookAndFeel =========================

void GlowKnobLookAndFeel::drawRotarySlider (juce::Graphics& g,
                                             int x, int y, int width, int height,
                                             float pos, float startAngle, float endAngle,
                                             juce::Slider&)
{
    auto b = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (6.0f);
    float R  = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    float cx = b.getCentreX();
    float cy = b.getCentreY();
    float angle = startAngle + pos * (endAngle - startAngle);

    // Outer dark fill
    g.setColour (juce::Colour::fromRGB (16, 20, 26));
    g.fillEllipse (cx - R, cy - R, R * 2.0f, R * 2.0f);

    // Track background arc
    {
        juce::Path p;
        p.addCentredArc (cx, cy, R * 0.80f, R * 0.80f, 0.0f, startAngle, endAngle, true);
        g.setColour (track.withAlpha (0.22f));
        g.strokePath (p, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));
    }

    // Active glow arc (wide, dim)
    {
        juce::Path p;
        p.addCentredArc (cx, cy, R * 0.80f, R * 0.80f, 0.0f, startAngle, angle, true);
        g.setColour (thumb.withAlpha (0.35f));
        g.strokePath (p, juce::PathStrokeType (9.0f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));
    }

    // Active arc (sharp)
    {
        juce::Path p;
        p.addCentredArc (cx, cy, R * 0.80f, R * 0.80f, 0.0f, startAngle, angle, true);
        g.setColour (thumb);
        g.strokePath (p, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));
    }

    // Inner knob body
    float inner = R * 0.62f;
    {
        juce::ColourGradient grad (juce::Colour::fromRGB (52, 63, 74), cx - inner * 0.35f, cy - inner * 0.35f,
                                   juce::Colour::fromRGB (19, 24, 31), cx + inner * 0.25f, cy + inner * 0.25f, false);
        g.setGradientFill (grad);
        g.fillEllipse (cx - inner, cy - inner, inner * 2.0f, inner * 2.0f);
    }
    g.setColour (juce::Colour::fromRGB (58, 70, 84));
    g.drawEllipse (cx - inner, cy - inner, inner * 2.0f, inner * 2.0f, 1.0f);

    // Pointer
    {
        juce::Path ptr;
        ptr.addRectangle (-1.5f, -inner * 0.90f, 3.0f, inner * 0.52f);
        ptr.applyTransform (juce::AffineTransform::rotation (angle).translated (cx, cy));
        g.setColour (thumb);
        g.fillPath (ptr);
    }

    // Tip dot glow
    float dotX = cx + std::sin (angle) * inner * 0.70f;
    float dotY = cy - std::cos (angle) * inner * 0.70f;
    g.setColour (thumb.withAlpha (0.35f));
    g.fillEllipse (dotX - 5.5f, dotY - 5.5f, 11.0f, 11.0f);
    g.setColour (thumb);
    g.fillEllipse (dotX - 2.5f, dotY - 2.5f,  5.0f,  5.0f);
}

// ========================= Editor =========================

RingModAudioProcessorEditor::RingModAudioProcessorEditor (RingModAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    freqSlider.setLookAndFeel (&freqLAF);
    freqSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    freqSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible (freqSlider);
    freqAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.getAPVTS(), "freq", freqSlider);

    mixSlider.setLookAndFeel (&mixLAF);
    mixSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    mixSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible (mixSlider);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.getAPVTS(), "mix", mixSlider);

    // Waveform selector — items must match AudioParameterChoice order exactly
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

    setSize (640, 400);
    startTimerHz (30);
}

RingModAudioProcessorEditor::~RingModAudioProcessorEditor()
{
    freqSlider.setLookAndFeel (nullptr);
    mixSlider.setLookAndFeel (nullptr);
}

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
        const int keep = kN - got;
        if (keep > 0)
            std::memmove (displayBuf.data(), displayBuf.data() + got, (size_t) keep * sizeof (float));
        std::memcpy (displayBuf.data() + keep, temp, (size_t) got * sizeof (float));
    }

    repaint();
}

float RingModAudioProcessorEditor::hzToAngle (float hz, float startAngle, float endAngle) noexcept
{
    float proportion = std::pow ((hz - 20.0f) / (2000.0f - 20.0f), 0.3f);
    return startAngle + proportion * (endAngle - startAngle);
}

// ---- Feature 8: JUCE logo ----
void RingModAudioProcessorEditor::drawJuceLogo (juce::Graphics& g)
{
    float cx = 607.0f, cy = 35.0f, r = 14.0f;
    // Orange circle
    g.setColour (juce::Colour::fromRGB (255, 140, 0));
    g.fillEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f);
    g.setColour (juce::Colours::white.withAlpha (0.9f));
    g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    g.drawText ("J", (int)(cx - r), (int)(cy - r), (int)(r * 2.0f), (int)(r * 2.0f),
                juce::Justification::centred);
    // "JUCE" label beside circle
    g.setColour (juce::Colour::fromRGB (200, 210, 225));
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.drawText ("JUCE", 548, 27, 52, 18, juce::Justification::right);
}

// ---- Feature 1 + 2: digital frequency display + range label ----
void RingModAudioProcessorEditor::drawDigitalDisplay (juce::Graphics& g)
{
    float freq = (float) freqSlider.getValue();

    // LED box
    juce::Rectangle<float> box (38.0f, 56.0f, 205.0f, 30.0f);
    g.setColour (juce::Colour::fromRGB (6, 18, 23));
    g.fillRoundedRectangle (box, 4.0f);
    g.setColour (juce::Colour::fromRGB (0, 72, 95));
    g.drawRoundedRectangle (box, 4.0f, 1.0f);

    // Frequency value — Feature 1
    g.setColour (juce::Colour::fromRGB (0, 240, 255));
    g.setFont (juce::FontOptions ("Courier New", 17.0f, juce::Font::bold));
    g.drawText (juce::String (freq, 2) + " Hz", box.reduced (6.0f, 2.0f), juce::Justification::centred);

    // Feature 2: range label
    g.setColour (juce::Colour::fromRGB (88, 108, 128));
    g.setFont (juce::FontOptions (10.5f));
    g.drawText ("Range: 20 Hz - 2000 Hz", 252, 62, 195, 16, juce::Justification::left);
}

// ---- Feature 3: frequency tick marks ----
void RingModAudioProcessorEditor::drawFreqTickMarks (juce::Graphics& g)
{
    const float cx      = 190.0f;
    const float cy      = 200.0f;
    const float outerR  = 120.0f;
    const float innerR  = 111.0f;
    const float labelR  = 135.0f;

    struct Tick { float hz; const char* label; };
    constexpr Tick ticks[] = {
        { 20.0f,   "20"   },
        { 200.0f,  "200"  },
        { 400.0f,  "400"  },
        { 600.0f,  "600"  },
        { 1000.0f, "1k"   },
        { 1500.0f, "1.5k" },
        { 2000.0f, "kHz"  }
    };

    for (auto& t : ticks)
    {
        float a   = hzToAngle (t.hz, kRotStart, kRotEnd);
        float sinA = std::sin (a), cosA = std::cos (a);

        g.setColour (juce::Colour::fromRGB (70, 86, 102));
        g.drawLine (cx + sinA * innerR, cy - cosA * innerR,
                    cx + sinA * outerR, cy - cosA * outerR, 1.3f);

        float lx = cx + sinA * labelR;
        float ly = cy - cosA * labelR;
        g.setColour (juce::Colour::fromRGB (110, 132, 152));
        g.setFont (juce::FontOptions (9.0f));
        g.drawText (t.label, (int)(lx - 18), (int)(ly - 7), 36, 14,
                    juce::Justification::centred);
    }

    // "Carrier Frequency" label — Feature 3 companion
    g.setColour (juce::Colour::fromRGB (138, 154, 172));
    g.setFont (juce::FontOptions (13.0f));
    g.drawText ("Carrier Frequency", 88, 324, 204, 18, juce::Justification::centred);
}

// ---- Feature 4: carrier buffer oscilloscope ----
void RingModAudioProcessorEditor::drawOscilloscope (juce::Graphics& g)
{
    juce::Rectangle<float> box (302.0f, 138.0f, 128.0f, 118.0f);

    g.setColour (juce::Colour::fromRGB (12, 16, 22));
    g.fillRoundedRectangle (box, 5.0f);
    g.setColour (juce::Colour::fromRGB (36, 46, 58));
    g.drawRoundedRectangle (box, 5.0f, 1.0f);

    g.setColour (juce::Colour::fromRGB (78, 98, 118));
    g.setFont (juce::FontOptions (9.5f));
    g.drawText ("Carrier Buffer", box.withHeight (15.0f), juce::Justification::centred);

    auto wave = box.reduced (6.0f, 18.0f);
    const auto& scope = displayBuf;           // message-thread-only snapshot
    const int   N     = RingModAudioProcessor::kScopeSize;
    const int   disp  = juce::jmin (N, 256);

    juce::Path waveform;
    float midY  = wave.getCentreY();
    float ampY  = wave.getHeight() * 0.42f;

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

        // glow pass
        g.setColour (juce::Colour::fromRGB (0, 155, 190).withAlpha (0.28f));
        g.strokePath (waveform, juce::PathStrokeType (4.5f));

        // main line
        g.setColour (juce::Colour::fromRGB (0, 215, 255));
        g.strokePath (waveform, juce::PathStrokeType (1.3f));
    }
}

// ---- Feature 5 + 6: mix LED bar + value readout ----
void RingModAudioProcessorEditor::drawMixSection (juce::Graphics& g)
{
    float mix = (float) mixSlider.getValue();
    int   pct = juce::roundToInt (mix * 100.0f);

    // Feature 6: "Mix: X.X" label
    g.setColour (juce::Colour::fromRGB (0, 245, 115));
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.drawText ("Mix: " + juce::String (mix, 1), 460, 57, 155, 18, juce::Justification::left);

    // Feature 6: value readout
    g.setColour (juce::Colour::fromRGB (95, 118, 138));
    g.setFont (juce::FontOptions (10.0f));
    g.drawText ("Value: " + juce::String (mix, 1) + " (" + juce::String (pct) + "% Wet)",
                460, 76, 165, 14, juce::Justification::left);

    // Feature 5: LED bar — background
    juce::Rectangle<float> barBg (460.0f, 94.0f, 148.0f, 10.0f);
    g.setColour (juce::Colour::fromRGB (8, 28, 16));
    g.fillRoundedRectangle (barBg, 3.0f);
    g.setColour (juce::Colour::fromRGB (18, 46, 28));
    g.drawRoundedRectangle (barBg, 3.0f, 1.0f);

    // Feature 5: active fill
    if (mix > 0.0f)
    {
        juce::Rectangle<float> fill = barBg.withWidth (barBg.getWidth() * mix);
        g.setColour (juce::Colour::fromRGB (0, 215, 105));
        g.fillRoundedRectangle (fill, 3.0f);
    }

    // "0%" / "100%" edge labels
    g.setColour (juce::Colour::fromRGB (72, 92, 108));
    g.setFont (juce::FontOptions (9.0f));
    g.drawText ("0%",   432, 92, 26, 12, juce::Justification::right);
    g.drawText ("100%", 610, 92, 30, 12, juce::Justification::left);

    // "Dry/Wet Mix" label below the mix knob
    g.setColour (juce::Colour::fromRGB (138, 154, 172));
    g.setFont (juce::FontOptions (13.0f));
    g.drawText ("Dry/Wet Mix", 448, 312, 148, 18, juce::Justification::centred);
}

// ---- Feature 7: PARAMETERS panel ----
void RingModAudioProcessorEditor::drawParamsPanel (juce::Graphics& g)
{
    // Dark strip
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

    // Cyan dot + freq
    g.setColour (juce::Colour::fromRGB (0, 200, 240));
    g.fillEllipse (16.0f, 369.0f, 7.0f, 7.0f);
    g.setColour (juce::Colour::fromRGB (128, 150, 172));
    g.setFont (juce::FontOptions (10.5f));
    g.drawText ("freq", 28, 365, 36, 15, juce::Justification::left);
    g.setColour (juce::Colour::fromRGB (195, 210, 228));
    g.drawText (juce::String (freq, 1), 66, 365, 55, 15, juce::Justification::left);

    // Green dot + mix
    g.setColour (juce::Colour::fromRGB (0, 215, 105));
    g.fillEllipse (16.0f, 383.0f, 7.0f, 7.0f);
    g.setColour (juce::Colour::fromRGB (128, 150, 172));
    g.drawText ("mix", 28, 379, 36, 15, juce::Justification::left);
    g.setColour (juce::Colour::fromRGB (195, 210, 228));
    g.drawText (juce::String (mix, 1), 66, 379, 55, 15, juce::Justification::left);

    // Status text (right side) — show active waveform
    g.setColour (juce::Colour::fromRGB (58, 76, 94));
    g.setFont (juce::FontOptions (9.5f));
    g.drawText ("Waveform: " + waveformBox.getText() + "  |  Phase Accumulator", 200, 365, 310, 14, juce::Justification::left);
    g.drawText ("carrierBuffer (Ready)",                                          200, 379, 200, 14, juce::Justification::left);
}

// ========================= paint / resized =========================

void RingModAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Background
    g.fillAll (juce::Colour::fromRGB (21, 25, 31));

    // Outer frame
    {
        juce::Path frame;
        frame.addRoundedRectangle (getLocalBounds().toFloat().reduced (4.0f), 7.0f);
        g.setColour (juce::Colour::fromRGB (36, 44, 54));
        g.strokePath (frame, juce::PathStrokeType (2.0f));
    }

    // Header gradient band
    {
        juce::ColourGradient hdr (juce::Colour::fromRGB (28, 34, 43), 0.0f, 0.0f,
                                   juce::Colour::fromRGB (21, 25, 31), 0.0f, 74.0f, false);
        g.setGradientFill (hdr);
        g.fillRect (0, 0, getWidth(), 74);
    }
    g.setColour (juce::Colour::fromRGB (36, 44, 55));
    g.drawLine (12.0f, 74.0f, 628.0f, 74.0f, 1.0f);

    // Title
    g.setColour (juce::Colour::fromRGB (198, 212, 228));
    g.setFont (juce::FontOptions ("Helvetica Neue", 20.0f, juce::Font::bold));
    g.drawText ("RingModAudioProcessor", 35, 17, 390, 28, juce::Justification::left);

    g.setColour (juce::Colour::fromRGB (75, 95, 115));
    g.setFont (juce::FontOptions (10.5f));
    g.drawText ("JUCE DSP CORE", 35, 46, 160, 15, juce::Justification::left);

    // All 9 features
    drawJuceLogo      (g);   // 8
    drawDigitalDisplay(g);   // 1 + 2
    drawFreqTickMarks (g);   // 3
    drawOscilloscope  (g);   // 4
    drawMixSection    (g);   // 5 + 6
    drawParamsPanel   (g);   // 7
    // Feature 9 (custom knob style) is handled by GlowKnobLookAndFeel

    // Waveform selector label (above the ComboBox at y=280)
    g.setColour (juce::Colour::fromRGB (110, 132, 152));
    g.setFont (juce::FontOptions (10.0f));
    g.drawText ("Waveform", 302, 264, 128, 14, juce::Justification::centred);
}

void RingModAudioProcessorEditor::resized()
{
    // Freq knob: centre at (190, 200), radius ~106
    freqSlider.setBounds (78, 85, 224, 230);

    // Mix knob: centre at (516, 218), radius ~68
    mixSlider.setBounds (448, 130, 136, 170);

    // Waveform selector: below the oscilloscope box (which ends at y=256)
    waveformBox.setBounds (302, 280, 128, 26);
}
