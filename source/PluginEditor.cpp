#include "PluginEditor.h"

static const float kRotStart = juce::MathConstants<float>::pi * 1.25f; 
static const float kRotEnd   = juce::MathConstants<float>::pi * 2.75f;

// =============================================================================
// FuturisticLookAndFeel
// =============================================================================

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

void FuturisticLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                              float sliderPos, float startAngle, float endAngle,
                                              juce::Slider& slider)
{
    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(10.0f);
    float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    float cx = bounds.getCentreX();
    float cy = bounds.getCentreY();
    
    // Choose color based on slider name (Freq vs Mix)
    juce::Colour accent = slider.getName() == "freq" ? juce::Colour::fromRGB(0, 200, 255) : juce::Colour::fromRGB(0, 255, 136);
    float angle = startAngle + sliderPos * (endAngle - startAngle);

    // Outer Rim (glow)
    juce::ColourGradient rimGrad(juce::Colours::transparentBlack, cx, cy, accent.withAlpha(0.12f), cx, cy - radius, true);
    g.setGradientFill(rimGrad);
    g.fillEllipse(cx - radius - 8, cy - radius - 8, (radius + 8) * 2, (radius + 8) * 2);

    // Outer bezel
    g.setColour(juce::Colour::fromRGB(8, 12, 20));
    g.fillEllipse(cx - radius - 5, cy - radius - 5, (radius + 5) * 2, (radius + 5) * 2);
    g.setColour(juce::Colour::fromRGB(28, 40, 64));
    g.drawEllipse(cx - radius - 5, cy - radius - 5, (radius + 5) * 2, (radius + 5) * 2, 1.5f);

    // Inner knob body
    float innerR = radius * 0.7f;
    juce::ColourGradient bodyGrad(juce::Colour::fromRGB(36, 46, 62), cx - innerR*0.3f, cy - innerR*0.3f,
                                  juce::Colour::fromRGB(8, 12, 20), cx + innerR, cy + innerR, true);
    g.setGradientFill(bodyGrad);
    g.fillEllipse(cx - innerR, cy - innerR, innerR * 2, innerR * 2);

    // Track arc
    juce::Path trackArc;
    trackArc.addCentredArc(cx, cy, radius, radius, 0.0f, startAngle, endAngle, true);
    g.setColour(juce::Colour::fromRGB(18, 28, 44));
    g.strokePath(trackArc, juce::PathStrokeType(radius * 0.15f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Value arc with glow
    if (sliderPos > 0.0f)
    {
        juce::Path valArc;
        valArc.addCentredArc(cx, cy, radius, radius, 0.0f, startAngle, angle, true);
        
        g.setColour(accent.withAlpha(0.3f));
        g.strokePath(valArc, juce::PathStrokeType(radius * 0.25f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        g.setColour(accent);
        g.strokePath(valArc, juce::PathStrokeType(radius * 0.15f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Needle
    juce::Path needle;
    needle.addLineSegment(juce::Line<float>(0, -innerR * 0.2f, 0, -innerR * 0.9f), radius * 0.04f);
    g.setColour(juce::Colour::fromRGB(220, 235, 255));
    g.fillPath(needle, juce::AffineTransform::rotation(angle).translated(cx, cy));

    // Center cap
    g.setColour(juce::Colour::fromRGB(7, 9, 14));
    g.fillEllipse(cx - innerR * 0.2f, cy - innerR * 0.2f, innerR * 0.4f, innerR * 0.4f);
    g.setColour(accent.withAlpha(0.8f));
    g.drawEllipse(cx - innerR * 0.2f, cy - innerR * 0.2f, innerR * 0.4f, innerR * 0.4f, 1.2f);
    g.setColour(accent.withAlpha(0.5f));
    g.fillEllipse(cx - innerR * 0.1f, cy - innerR * 0.1f, innerR * 0.2f, innerR * 0.2f);
}

void FuturisticLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool,
                                          int, int, int, int, juce::ComboBox& box)
{
    auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat();
    juce::ColourGradient bgGrad(juce::Colour::fromRGB(12, 30, 50), 0, 0,
                                juce::Colour::fromRGB(8, 22, 40), 0, (float)height, false);
    g.setGradientFill(bgGrad);
    g.fillRoundedRectangle(bounds, 3.0f);
    
    g.setColour(juce::Colour::fromRGB(30, 74, 106));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 3.0f, 1.0f);

    // Arrow
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

void FuturisticLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (1, 1, box.getWidth() - 30, box.getHeight() - 2);
    label.setFont (juce::FontOptions(14.0f, juce::Font::bold));
}

void FuturisticLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                              float sliderPos, float minSliderPos, float maxSliderPos,
                                              const juce::Slider::SliderStyle, juce::Slider& slider)
{
    juce::ignoreUnused (minSliderPos, maxSliderPos);

    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();
    juce::Colour accent = slider.getName() == "freq" ? juce::Colour::fromRGB(0, 200, 255) : juce::Colour::fromRGB(0, 255, 136);

    // Track
    g.setColour(juce::Colour::fromRGB(5, 14, 8));
    g.fillRoundedRectangle(bounds, 2.0f);
    g.setColour(accent.withAlpha(0.35f).interpolatedWith(juce::Colour::fromRGB(20, 56, 40), 0.5f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 2.0f, 1.0f);

    // Fill, proportional to value
    float fillW = juce::jlimit(0.0f, bounds.getWidth(), sliderPos - bounds.getX());
    if (fillW > 0.5f)
    {
        auto fillBounds = bounds.withWidth(fillW);
        juce::ColourGradient fillGrad(accent.withAlpha(0.55f), fillBounds.getX(), 0,
                                      accent, fillBounds.getRight(), 0, false);
        g.setGradientFill(fillGrad);
        g.fillRoundedRectangle(fillBounds, 2.0f);

        g.setColour(accent.withAlpha(0.5f));
        g.drawLine(fillBounds.getRight(), bounds.getY(), fillBounds.getRight(), bounds.getBottom(), 1.5f);
    }
}

// =============================================================================
// RingModAudioProcessorEditor
// =============================================================================

RingModAudioProcessorEditor::RingModAudioProcessorEditor (RingModAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel(&customLAF);

    freqSlider.setName("freq");
    freqSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    freqSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    freqSlider.setRange(20, 2000);
    addAndMakeVisible(freqSlider);
    freqAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "freq", freqSlider);

    mixSlider.setName("mix");
    mixSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    mixSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(mixSlider);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "mix", mixSlider);

    mixBarSlider.setName("mix");
    mixBarSlider.setSliderStyle(juce::Slider::LinearBar);
    mixBarSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(mixBarSlider);
    mixBarAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "mix", mixBarSlider);

    waveformBox.addItem("Sine", 1);
    waveformBox.addItem("Saw", 2);
    waveformBox.addItem("Square", 3);
    waveformBox.addItem("Triangle", 4);
    waveformBox.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(waveformBox);
    waveformAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getAPVTS(), "waveform", waveformBox);

    setSize(700, 470);
    startTimerHz(30);
}

RingModAudioProcessorEditor::~RingModAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void RingModAudioProcessorEditor::timerCallback()
{
    static constexpr int kN = RingModAudioProcessor::kScopeSize;
    float temp[kN];
    int got;
    while ((got = audioProcessor.drainScopeData(temp, kN)) > 0) {
        const int keep = kN - got;
        if (keep > 0)
            std::memmove(displayBuf.data(), displayBuf.data() + got, (size_t)keep * sizeof(float));
        std::memcpy(displayBuf.data() + keep, temp, (size_t)got * sizeof(float));
    }
    ++frameCounter;
    repaint();
}

void RingModAudioProcessorEditor::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // ---- Background base + colour blooms ----
    g.fillAll (juce::Colour::fromRGB(4, 6, 12));

    auto glow = [&] (float cxProp, float cyProp, float rProp, juce::Colour c)
    {
        float cx = (float) getWidth() * cxProp;
        float cy = (float) getHeight() * cyProp;
        float r  = (float) juce::jmax(getWidth(), getHeight()) * rProp;
        juce::ColourGradient grad(c, cx, cy, juce::Colours::transparentBlack, cx + r, cy, true);
        g.setGradientFill(grad);
        g.fillRect(bounds);
    };
    glow(0.22f, 0.48f, 0.48f, juce::Colour::fromRGB(0, 170, 255).withAlpha(0.16f));
    glow(0.75f, 0.60f, 0.44f, juce::Colour::fromRGB(102, 0, 221).withAlpha(0.15f));
    glow(0.82f, 0.12f, 0.32f, juce::Colour::fromRGB(68, 0, 187).withAlpha(0.11f));
    glow(0.10f, 0.88f, 0.30f, juce::Colour::fromRGB(0, 85, 204).withAlpha(0.10f));

    // Diagonal metallic streaks
    g.setColour(juce::Colour::fromRGB(0, 80, 160).withAlpha(0.05f));
    for (int i = -getHeight(); i < getWidth() + getHeight(); i += 38)
        g.drawLine((float) i, 0.0f, (float) i + (float) getHeight() * 0.32f, (float) getHeight(), 1.0f);
    g.setColour(juce::Colour::fromRGB(40, 0, 100).withAlpha(0.045f));
    for (int i = -getHeight(); i < getWidth() + getHeight(); i += 52)
        g.drawLine((float) i, 0.0f, (float) i + (float) getHeight() * 0.40f, (float) getHeight(), 1.0f);
    g.setColour(juce::Colour::fromRGB(0, 160, 255).withAlpha(0.025f));
    for (int i = -getHeight(); i < getWidth() + getHeight(); i += 70)
        g.drawLine((float) i, 0.0f, (float) i + (float) getHeight() * 0.27f, (float) getHeight(), 1.0f);

    // Vignette
    {
        float cx = (float) getWidth() * 0.5f, cy = (float) getHeight() * 0.5f;
        float r  = (float) juce::jmax(getWidth(), getHeight()) * 0.62f;
        juce::ColourGradient vignette(juce::Colours::transparentBlack, cx, cy,
                                      juce::Colour::fromRGB(0, 0, 8).withAlpha(0.55f), cx + r, cy, true);
        g.setGradientFill(vignette);
        g.fillRect(bounds);
    }

    // ---- Title bar ----
    juce::Rectangle<int> titleBar(0, 0, getWidth(), 40);
    g.setColour(juce::Colour::fromRGB(4, 8, 18).withAlpha(0.8f));
    g.fillRect(titleBar);
    g.setColour(juce::Colour::fromRGB(24, 40, 64));
    g.fillRect(0, 39, getWidth(), 1);

    g.setColour(juce::Colour::fromRGB(184, 212, 238));
    g.setFont(juce::FontOptions(18.0f, juce::Font::bold));
    g.drawText("RING MOD", titleBar, juce::Justification::centred);

    g.setColour(juce::Colour::fromRGB(40, 64, 96));
    g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    g.drawText("JUCE", titleBar.reduced(16, 0), juce::Justification::centredRight);

    // ---- Subtitle strip ----
    juce::Rectangle<int> subStrip(0, 40, getWidth(), 20);
    g.setColour(juce::Colour::fromRGB(3, 6, 14).withAlpha(0.7f));
    g.fillRect(subStrip);
    g.setColour(juce::Colour::fromRGB(14, 27, 44));
    g.fillRect(0, 59, getWidth(), 1);

    g.setColour(juce::Colour::fromRGB(46, 80, 112));
    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    g.drawText("JUCE DSP | Phase Accumulator", subStrip.withTrimmedLeft(16), juce::Justification::centredLeft);

    // ---- Content columns ----
    auto contentArea = getLocalBounds().withTrimmedTop(60).withTrimmedBottom(34).reduced(24, 20);
    int colWidth = (contentArea.getWidth() - 32) / 3;

    auto leftCol = contentArea.removeFromLeft(colWidth);
    contentArea.removeFromLeft(16);
    auto centerCol = contentArea.removeFromLeft(colWidth);
    contentArea.removeFromLeft(16);
    auto rightCol = contentArea;

    // --- LEFT: Carrier frequency ---
    auto freqBox = leftCol.removeFromTop(40);
    g.setColour(juce::Colour::fromRGB(2, 4, 8));
    g.fillRect(freqBox);
    g.setColour(juce::Colour::fromRGB(26, 56, 80));
    g.drawRect(freqBox, 1);

    g.setColour(juce::Colour::fromRGB(30, 64, 96));
    g.setFont(juce::FontOptions(8.0f, juce::Font::plain));
    g.drawText(juce::String(frameCounter % 1000).paddedLeft('0', 3), freqBox.reduced(10, 5), juce::Justification::topRight);

    float freqHz = audioProcessor.getAPVTS().getRawParameterValue("freq")->load();
    juce::String freqStr = freqHz >= 1000.0f ? juce::String(freqHz / 1000.0f, 2) + " kHz" : juce::String(freqHz, 2) + " Hz";
    g.setColour(juce::Colour::fromRGB(0, 200, 255));
    g.setFont(juce::FontOptions(26.0f, juce::Font::plain));
    g.drawText(freqStr, freqBox, juce::Justification::centred);

    leftCol.removeFromTop(8);
    g.setColour(juce::Colour::fromRGB(32, 64, 80));
    g.setFont(juce::FontOptions(9.0f, juce::Font::plain));
    g.drawText("Range: 20 Hz - 2000 Hz", leftCol.removeFromTop(16), juce::Justification::topLeft);

    leftCol.removeFromTop(10);
    leftCol.removeFromTop(152); // freqSlider knob paints itself here (see resized())

    leftCol.removeFromTop(10);
    g.setColour(juce::Colour::fromRGB(58, 104, 128));
    g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    g.drawText("CARRIER FREQUENCY", leftCol.removeFromTop(18), juce::Justification::centred);

    leftCol.removeFromTop(8);
    auto minMaxRow = leftCol.removeFromTop(14);
    g.setColour(juce::Colour::fromRGB(30, 64, 96));
    g.setFont(juce::FontOptions(8.0f, juce::Font::plain));
    g.drawText("20", minMaxRow, juce::Justification::centredLeft);
    g.drawText("kHz", minMaxRow, juce::Justification::centredRight);

    // --- CENTER: Waveform ---
    g.setColour(juce::Colour::fromRGB(40, 72, 96));
    g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
    g.drawText("CARRIER BUFFER", centerCol.removeFromTop(16), juce::Justification::centred);

    centerCol.removeFromTop(10);
    auto scopeBox = centerCol.removeFromTop(128).withSizeKeepingCentre(168, 128);
    g.setColour(juce::Colour::fromRGB(1, 4, 8));
    g.fillRect(scopeBox);
    g.setColour(juce::Colour::fromRGB(24, 46, 64));
    g.drawRect(scopeBox, 1);

    // Grid
    g.setColour(juce::Colour::fromRGB(12, 30, 42));
    for(int y = 32; y < 128; y += 32) g.drawLine((float)scopeBox.getX(), (float)(scopeBox.getY() + y), (float)scopeBox.getRight(), (float)(scopeBox.getY() + y));
    for(int x = 42; x < 168; x += 42) g.drawLine((float)(scopeBox.getX() + x), (float)scopeBox.getY(), (float)(scopeBox.getX() + x), (float)scopeBox.getBottom());

    g.setColour(juce::Colour::fromRGB(22, 40, 56));
    g.drawLine((float)scopeBox.getX(), (float)(scopeBox.getY() + 64), (float)scopeBox.getRight(), (float)(scopeBox.getY() + 64));

    // Corner ticks
    g.setColour(juce::Colour::fromRGB(30, 64, 96));
    g.drawRect(scopeBox.getX() + 4, scopeBox.getY() + 4, 6, 6, 1);
    g.drawRect(scopeBox.getRight() - 10, scopeBox.getY() + 4, 6, 6, 1);
    g.drawRect(scopeBox.getX() + 4, scopeBox.getBottom() - 10, 6, 6, 1);
    g.drawRect(scopeBox.getRight() - 10, scopeBox.getBottom() - 10, 6, 6, 1);

    // Waveform trace
    juce::Path wavePath;
    float scopeW = (float)scopeBox.getWidth();
    float scopeH = scopeBox.getHeight() * 0.5f;
    float scopeCy = (float)scopeBox.getCentreY();
    float scopeX = (float)scopeBox.getX();

    bool started = false;
    for (int i = 0; i < RingModAudioProcessor::kScopeSize; ++i)
    {
        float x = scopeX + ((float)i / RingModAudioProcessor::kScopeSize) * scopeW;
        float y = scopeCy - displayBuf[i] * scopeH * 0.9f;
        if (!started) { wavePath.startNewSubPath(x, y); started = true; }
        else wavePath.lineTo(x, y);
    }

    g.setColour(juce::Colour::fromRGB(0, 200, 255).withAlpha(0.2f));
    g.strokePath(wavePath, juce::PathStrokeType(4.0f));
    g.setColour(juce::Colour::fromRGB(0, 200, 255));
    g.strokePath(wavePath, juce::PathStrokeType(1.5f));

    centerCol.removeFromTop(10);
    g.setColour(juce::Colour::fromRGB(32, 64, 80));
    g.setFont(juce::FontOptions(10.0f, juce::Font::plain));
    g.drawText("Waveform", centerCol.removeFromTop(14), juce::Justification::centredLeft);

    // dropdown paints itself here (see resized())

    // --- RIGHT: Mix ---
    auto mixHeader = rightCol.removeFromTop(26);
    g.setColour(juce::Colour::fromRGB(46, 112, 80));
    g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    g.drawText("MIX:", mixHeader.removeFromLeft(40), juce::Justification::centredLeft);

    float mixVal = audioProcessor.getAPVTS().getRawParameterValue("mix")->load();
    g.setColour(juce::Colour::fromRGB(0, 255, 136));
    g.setFont(juce::FontOptions(22.0f, juce::Font::plain));
    g.drawText(juce::String(mixVal, 2), mixHeader, juce::Justification::centredRight);

    int mixPct = juce::roundToInt(mixVal * 100.0f);

    rightCol.removeFromTop(6);
    g.setColour(juce::Colour::fromRGB(35, 80, 64));
    g.setFont(juce::FontOptions(9.0f, juce::Font::plain));
    g.drawText("Value: " + juce::String(mixVal, 1) + " (" + juce::String(mixPct) + "% Wet)",
                rightCol.removeFromTop(14), juce::Justification::topLeft);

    rightCol.removeFromTop(10);
    auto mixLabelsRow = rightCol.removeFromTop(12);
    g.setColour(juce::Colour::fromRGB(26, 56, 48));
    g.setFont(juce::FontOptions(7.0f, juce::Font::plain));
    g.drawText("0%", mixLabelsRow, juce::Justification::centredLeft);
    g.drawText("100%", mixLabelsRow, juce::Justification::centredRight);

    rightCol.removeFromTop(4);
    rightCol.removeFromTop(9); // mixBarSlider paints itself here (see resized())

    rightCol.removeFromTop(14);
    rightCol.removeFromTop(116); // mixSlider knob paints itself here (see resized())

    rightCol.removeFromTop(8);
    g.setColour(juce::Colour::fromRGB(46, 96, 80));
    g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    g.drawText("DRY/WET MIX", rightCol.removeFromTop(16), juce::Justification::centred);

    // ---- Footer status bar ----
    juce::Rectangle<int> footer(0, getHeight() - 34, getWidth(), 34);
    g.setColour(juce::Colour::fromRGB(2, 4, 10).withAlpha(0.68f));
    g.fillRect(footer);
    g.setColour(juce::Colour::fromRGB(14, 27, 44));
    g.fillRect(0, footer.getY(), getWidth(), 1);

    auto footerContent = footer.reduced(16, 0);
    g.setColour(juce::Colour::fromRGB(30, 48, 80));
    g.setFont(juce::FontOptions(8.0f, juce::Font::bold));
    g.drawText("PARAMETERS", footerContent.removeFromLeft(80), juce::Justification::centredLeft);

    auto drawPip = [&] (juce::Rectangle<int> area, juce::Colour c, const juce::String& label, const juce::String& value)
    {
        auto dotArea = area.removeFromLeft(10);
        g.setColour(c);
        g.fillEllipse(dotArea.withSizeKeepingCentre(6, 6).toFloat());
        area.removeFromLeft(4);
        g.setFont(juce::FontOptions(8.0f, juce::Font::plain));
        g.setColour(juce::Colour::fromRGB(42, 80, 96));
        g.drawText(label + " " + value, area, juce::Justification::centredLeft);
    };
    drawPip(footerContent.removeFromLeft(90), juce::Colour::fromRGB(0, 200, 255), "freq", juce::String(freqHz, 1));
    drawPip(footerContent.removeFromLeft(90), juce::Colour::fromRGB(0, 255, 136), "mix", juce::String(mixVal, 1));

    static const char* waveformNames[] = { "Sine", "Saw", "Square", "Triangle" };
    int waveformIdx = juce::jlimit(0, 3, (int) std::round(audioProcessor.getAPVTS().getRawParameterValue("waveform")->load()));
    g.setColour(juce::Colour::fromRGB(30, 56, 72));
    g.setFont(juce::FontOptions(8.0f, juce::Font::plain));
    g.drawText(juce::String("Waveform: ") + waveformNames[waveformIdx] + " | Phase Accumulator | carrierBuffer (Ready)",
               footerContent, juce::Justification::centredRight);

    // ---- Scanline overlay (drawn over all content) ----
    g.setColour(juce::Colours::black.withAlpha(0.10f));
    for (int y = 0; y < getHeight(); y += 4)
        g.drawLine(0.0f, (float) y, (float) getWidth(), (float) y, 1.0f);

    // ---- Corner accent brackets (topmost layer) ----
    g.setColour(juce::Colour::fromRGB(0, 200, 255).withAlpha(0.33f));
    g.drawLine(1.0f, 1.0f, 13.0f, 1.0f, 1.0f);
    g.drawLine(1.0f, 1.0f, 1.0f, 13.0f, 1.0f);
    g.drawLine((float) getWidth() - 13.0f, 1.0f, (float) getWidth() - 1.0f, 1.0f, 1.0f);
    g.drawLine((float) getWidth() - 1.0f, 1.0f, (float) getWidth() - 1.0f, 13.0f, 1.0f);
    g.drawLine(1.0f, (float) getHeight() - 1.0f, 13.0f, (float) getHeight() - 1.0f, 1.0f);
    g.drawLine(1.0f, (float) getHeight() - 13.0f, 1.0f, (float) getHeight() - 1.0f, 1.0f);
    g.drawLine((float) getWidth() - 13.0f, (float) getHeight() - 1.0f, (float) getWidth() - 1.0f, (float) getHeight() - 1.0f, 1.0f);
    g.drawLine((float) getWidth() - 1.0f, (float) getHeight() - 13.0f, (float) getWidth() - 1.0f, (float) getHeight() - 1.0f, 1.0f);
}

void RingModAudioProcessorEditor::resized()
{
    auto contentArea = getLocalBounds().withTrimmedTop(60).withTrimmedBottom(34).reduced(24, 20);
    int colWidth = (contentArea.getWidth() - 32) / 3;

    auto leftCol = contentArea.removeFromLeft(colWidth);
    contentArea.removeFromLeft(16);
    auto centerCol = contentArea.removeFromLeft(colWidth);
    contentArea.removeFromLeft(16);
    auto rightCol = contentArea;

    leftCol.removeFromTop(74);
    freqSlider.setBounds(leftCol.removeFromTop(152).withSizeKeepingCentre(152, 152));

    centerCol.removeFromTop(186);
    waveformBox.setBounds(centerCol.removeFromTop(36).withSizeKeepingCentre(168, 36));

    rightCol.removeFromTop(72);
    mixBarSlider.setBounds(rightCol.removeFromTop(9));
    rightCol.removeFromTop(14);
    mixSlider.setBounds(rightCol.removeFromTop(116).withSizeKeepingCentre(116, 116));
}
