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
    
    addAndMakeVisible(calibrateButton);
    calibrateButton.onClick = [this] {
        audioProcessor.getCalibrationManager().startCalibration();
    };
    
    addAndMakeVisible(calibrationLabel);
    calibrationLabel.setJustificationType(juce::Justification::centred);
    calibrationLabel.setColour(juce::Label::textColourId, juce::Colours::yellow);
    
    // Increased size to accommodate HUD, Waveform and the new scale/glide parameters
    setSize (450, 580);
    
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
    
    // 0. Place Calibrate Button and Label at the very top (30px)
    auto calibrationArea = area.removeFromTop(30).reduced(2);
    calibrateButton.setBounds(calibrationArea.removeFromLeft(120));
    calibrationLabel.setBounds(calibrationArea);
    
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
    
    // 4. Update Calibration State
    auto& calMgr = audioProcessor.getCalibrationManager();
    auto state = calMgr.getCurrentState();
    
    if (state == v2m::CalibrationManager::State::Inactive)
    {
        calibrationLabel.setText("", juce::dontSendNotification);
    }
    else if (state == v2m::CalibrationManager::State::MeasuringNoise)
    {
        calibrationLabel.setText("Stay silent (Measuring background noise...)", juce::dontSendNotification);
    }
    else if (state == v2m::CalibrationManager::State::MeasuringVocalRange)
    {
        calibrationLabel.setText("Sing your range (Aaa/Ooo)...", juce::dontSendNotification);
    }
    else if (state == v2m::CalibrationManager::State::Completed)
    {
        calibrationLabel.setText("Calibration Done!", juce::dontSendNotification);
        
        // Grab values from atomic storage and push to APVTS via parameter attachments
        auto& apvts = audioProcessor.getAPVTS();
        if (auto p = apvts.getParameter("gate_threshold"))
            p->setValueNotifyingHost(p->convertTo0to1(calMgr.getGateThresholdDb()));
        
        if (auto p = apvts.getParameter("min_freq"))
            p->setValueNotifyingHost(p->convertTo0to1(calMgr.getMinFreqHz()));
            
        if (auto p = apvts.getParameter("max_freq"))
            p->setValueNotifyingHost(p->convertTo0to1(calMgr.getMaxFreqHz()));
            
        // Reset state so we don't spam updates
        calMgr.markAsInactive();
    }
}
