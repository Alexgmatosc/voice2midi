#include "PluginProcessor.h"
#include "PluginEditor.h"

PluginEditor::PluginEditor (VoiceToMidiProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), genericEditor(p),
      visualizer (p.getAPVTS())
{
    addAndMakeVisible(genericEditor);
    addAndMakeVisible(visualizer);
    addAndMakeVisible(hud);
    addAndMakeVisible(timbreMeter);
    
    // Increased size to accommodate HUD, Waveform and the new scale/glide parameters
    setSize (450, 530);
    
    startTimerHz(60); // 60 FPS for smooth waveform updates
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
    
    // 1. Place HUD display at the top (80px height) next to TimbreMeter
    auto topArea = area.removeFromTop(80).reduced(10);
    timbreMeter.setBounds(topArea.removeFromRight(40));
    hud.setBounds(topArea.reduced(2));
    
    // 2. Place Waveform visualizer in the middle (120px height)
    visualizer.setBounds(area.removeFromTop(120).reduced(10));
    
    // 3. The generic editor takes the remaining space at the bottom for parameter sliders
    genericEditor.setBounds(area);
}

void PluginEditor::timerCallback()
{
    // 1. Pull downsampled audio from the processor FIFO and push to the visualizer
    auto& fifo = audioProcessor.getVisualFifo();
    int available = fifo.getNumAvailable();
    if (available > 0)
    {
        std::vector<float> tempBuffer(static_cast<size_t>(available));
        int readCount = fifo.read(tempBuffer.data(), available);
        if (readCount > 0)
        {
            visualizer.pushSamples(tempBuffer.data(), readCount);
        }
    }
    
    // 2. Poll notes, pitch, and spectral centroid from the audio processor
    int playingNote = audioProcessor.getCurrentPlayingNote();
    float pitchHz = audioProcessor.getCurrentPitchHz();
    float centroid = audioProcessor.getCurrentCentroid();
    
    // 3. Update HUD and TimbreMeter states
    hud.updateState(playingNote, pitchHz);
    timbreMeter.setCentroidValue(centroid);
}
