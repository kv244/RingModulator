#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class FuturisticLookAndFeel : public juce::LookAndFeel_V4
{
public:
    FuturisticLookAndFeel();
    
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float startAngle, float endAngle,
                           juce::Slider& slider) override;
                           
    void drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH,
                       juce::ComboBox& box) override;
                       
    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override;
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
    void timerCallback() override;

    RingModAudioProcessor& audioProcessor;
    std::array<float, RingModAudioProcessor::kScopeSize> displayBuf {};

    FuturisticLookAndFeel customLAF;

    juce::Slider   freqSlider;
    juce::Slider   mixSlider;
    juce::ComboBox waveformBox;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   freqAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   mixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> waveformAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RingModAudioProcessorEditor)
};
