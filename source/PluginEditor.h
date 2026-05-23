#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class GlowKnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    GlowKnobLookAndFeel (juce::Colour trackColour, juce::Colour thumbColour)
        : track (trackColour), thumb (thumbColour) {}

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float startAngle, float endAngle,
                           juce::Slider&) override;
private:
    juce::Colour track, thumb;
};

class RingModAudioProcessorEditor : public juce::AudioProcessorEditor,
                                     private juce::Timer
{
public:
    RingModAudioProcessorEditor (RingModAudioProcessor&);
    ~RingModAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override { repaint(); }

    void drawJuceLogo      (juce::Graphics&);
    void drawDigitalDisplay(juce::Graphics&);
    void drawFreqTickMarks (juce::Graphics&);
    void drawOscilloscope  (juce::Graphics&);
    void drawMixSection    (juce::Graphics&);
    void drawParamsPanel   (juce::Graphics&);

    static float hzToAngle (float hz, float startAngle, float endAngle) noexcept;

    RingModAudioProcessor& audioProcessor;

    GlowKnobLookAndFeel freqLAF { juce::Colour::fromRGB (0, 170, 200), juce::Colour::fromRGB (0, 240, 255) };
    GlowKnobLookAndFeel mixLAF  { juce::Colour::fromRGB (0, 155, 75),  juce::Colour::fromRGB (0, 255, 120) };

    juce::Slider freqSlider;
    juce::Slider mixSlider;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> freqAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RingModAudioProcessorEditor)
};
