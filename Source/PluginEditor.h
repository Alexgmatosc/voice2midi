#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UI/WaveformVisualizer.h"
#include "UI/HUDDisplay.h"
#include "UI/TimbreMeter.h"

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
    juce::GenericAudioProcessorEditor genericEditor;
    
    v2m::WaveformVisualizer visualizer;
    v2m::HUDDisplay hud;
    v2m::TimbreMeter timbreMeter;
    
    juce::TextButton calibrateButton { "Calibrate Voice" };
    juce::Label calibrationLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};
