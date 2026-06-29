#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UI/WaveformVisualizer.h"
#include "UI/HUDDisplay.h"
#include "UI/TimbreMeter.h"
#include "UI/CustomLookAndFeel.h"

class PluginEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    PluginEditor (VoiceToMidiProcessor&);
    ~PluginEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    VoiceToMidiProcessor& audioProcessor;
    
    v2m::CustomLookAndFeel customLookAndFeel;
    
    v2m::WaveformVisualizer visualizer;
    v2m::HUDDisplay hud;
    v2m::TimbreMeter timbreMeter;
    
    juce::TextButton calibrateButton { "Calibrate Voice" };
    juce::Label calibrationLabel;
    
    // Custom Components
    juce::Slider inputGainSlider, gateThresholdSlider, minFreqSlider, maxFreqSlider, pitchBendGlideSlider, intellibendStickinessSlider;
    juce::ComboBox pitchBendRangeBox, scaleRootBox, scaleTypeBox, trackingModeBox, expressionCCBox, intellibendModeBox;
    
    // Labels
    juce::Label inputGainLabel, gateThresholdLabel, minFreqLabel, maxFreqLabel, pitchBendGlideLabel, intellibendStickinessLabel;
    juce::Label pitchBendRangeLabel, scaleRootLabel, scaleTypeLabel, trackingModeLabel, expressionCCLabel, intellibendModeLabel;

    // Attachments
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    
    std::unique_ptr<SliderAttachment> inputGainAttachment, gateThresholdAttachment, minFreqAttachment, maxFreqAttachment, pitchBendGlideAttachment, intellibendStickinessAttachment;
    std::unique_ptr<ComboBoxAttachment> pitchBendRangeAttachment, scaleRootAttachment, scaleTypeAttachment, trackingModeAttachment, expressionCCAttachment, intellibendModeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};
