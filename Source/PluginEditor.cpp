#include "PluginProcessor.h"
#include "PluginEditor.h"

PluginEditor::PluginEditor (VoiceToMidiProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), genericEditor(p)
{
    addAndMakeVisible(genericEditor);
    
    // Setup the monitor label
    addAndMakeVisible(monitorLabel);
    monitorLabel.setJustificationType(juce::Justification::centred);
    monitorLabel.setFont(juce::Font(juce::FontOptions().withHeight(16.0f).withStyle("Bold")));
    monitorLabel.setColour(juce::Label::backgroundColourId, juce::Colours::black.withAlpha(0.4f));
    monitorLabel.setColour(juce::Label::textColourId, juce::Colours::cyan);
    monitorLabel.setText("Audio Monitor - Esperando Señal...", juce::dontSendNotification);

    // Set height to accommodate the generic editor plus the monitor bar
    setSize (400, 360);
    
    startTimerHz(30); // 30 FPS updates
}

PluginEditor::~PluginEditor()
{
    stopTimer();
}

void PluginEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void PluginEditor::resized()
{
    auto area = getLocalBounds();
    
    // Place the monitor label at the top
    monitorLabel.setBounds(area.removeFromTop(60).reduced(10));
    
    // The generic editor takes the remaining space
    genericEditor.setBounds(area);
}

void PluginEditor::timerCallback()
{
    float db = audioProcessor.getCurrentLevelDb();
    float pitch = audioProcessor.getCurrentPitchHz();
    
    juce::String pitchText = (pitch > 0.0f) ? juce::String(pitch, 1) + " Hz" : "No detectado";
    
    juce::String text = juce::String::formatted(
        "Nivel de Entrada: %.1f dB   |   Pitch: %s",
        db,
        pitchText.toRawUTF8()
    );
    
    // Change color based on gate status
    auto gateParam = audioProcessor.getAPVTS().getRawParameterValue("gate_threshold");
    float gateThreshold = (gateParam != nullptr) ? gateParam->load() : -30.0f;
    if (db > gateThreshold)
    {
        monitorLabel.setColour(juce::Label::textColourId, juce::Colours::lightgreen);
    }
    else
    {
        monitorLabel.setColour(juce::Label::textColourId, juce::Colours::cyan);
    }
    
    monitorLabel.setText(text, juce::dontSendNotification);
}
