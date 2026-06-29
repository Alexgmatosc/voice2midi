#include "PluginProcessor.h"
#include "PluginEditor.h"

PluginEditor::PluginEditor (VoiceToMidiProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
      visualizer (p.getAPVTS())
{
    setLookAndFeel(&customLookAndFeel);

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

    auto setupSlider = [this, &p](juce::Slider& s, juce::Label& l, const juce::String& text, const juce::String& paramId, std::unique_ptr<SliderAttachment>& attach) {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
        addAndMakeVisible(s);
        
        l.setText(text, juce::dontSendNotification);
        l.setJustificationType(juce::Justification::centred);
        l.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.7f));
        l.setFont(14.0f);
        addAndMakeVisible(l);
        
        attach = std::make_unique<SliderAttachment>(p.getAPVTS(), paramId, s);
    };

    auto setupCombo = [this, &p](juce::ComboBox& c, juce::Label& l, const juce::String& text, const juce::String& paramId, std::unique_ptr<ComboBoxAttachment>& attach) {
        addAndMakeVisible(c);
        
        l.setText(text, juce::dontSendNotification);
        l.setJustificationType(juce::Justification::centred);
        l.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.7f));
        l.setFont(14.0f);
        addAndMakeVisible(l);
        
        attach = std::make_unique<ComboBoxAttachment>(p.getAPVTS(), paramId, c);
    };

    setupSlider(inputGainSlider, inputGainLabel, "Gain", "input_gain", inputGainAttachment);
    setupSlider(gateThresholdSlider, gateThresholdLabel, "Gate", "gate_threshold", gateThresholdAttachment);
    setupSlider(minFreqSlider, minFreqLabel, "Min Freq", "min_freq", minFreqAttachment);
    setupSlider(maxFreqSlider, maxFreqLabel, "Max Freq", "max_freq", maxFreqAttachment);
    setupSlider(pitchBendGlideSlider, pitchBendGlideLabel, "Glide", "pitch_bend_glide", pitchBendGlideAttachment);
    setupSlider(intellibendStickinessSlider, intellibendStickinessLabel, "Stickiness", "intellibend_stickiness", intellibendStickinessAttachment);

    setupCombo(pitchBendRangeBox, pitchBendRangeLabel, "PB Range", "pitch_bend_range", pitchBendRangeAttachment);
    setupCombo(scaleRootBox, scaleRootLabel, "Root", "scale_root", scaleRootAttachment);
    setupCombo(scaleTypeBox, scaleTypeLabel, "Scale", "scale_type", scaleTypeAttachment);
    setupCombo(trackingModeBox, trackingModeLabel, "Tracking", "tracking_mode", trackingModeAttachment);
    setupCombo(expressionCCBox, expressionCCLabel, "Express CC", "expression_cc", expressionCCAttachment);
    setupCombo(intellibendModeBox, intellibendModeLabel, "Intellibend", "intellibend_mode", intellibendModeAttachment);

    setSize (650, 520);
    
    startTimerHz(60); // 60 FPS for smooth waveform updates
}

PluginEditor::~PluginEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void PluginEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour(0xff121316)); // Darker modern background

    // Draw panel backgrounds
    auto area = getLocalBounds();
    area.removeFromTop(30);
    area.removeFromTop(150); // visArea
    
    auto panelArea = area.reduced(10);
    int pW = panelArea.getWidth() / 3;
    
    g.setColour(juce::Colour(0xff1A1C20));
    g.fillRoundedRectangle(panelArea.removeFromLeft(pW).reduced(5).toFloat(), 10.0f);
    g.fillRoundedRectangle(panelArea.removeFromLeft(pW).reduced(5).toFloat(), 10.0f);
    g.fillRoundedRectangle(panelArea.reduced(5).toFloat(), 10.0f);
    
    // Panel Titles
    g.setColour(juce::Colours::white.withAlpha(0.6f));
    g.setFont(juce::Font(16.0f, juce::Font::bold));
    
    auto titleArea = getLocalBounds().reduced(10).withTop(185);
    g.drawText("Input Settings", titleArea.removeFromLeft(pW).withHeight(30), juce::Justification::centred);
    g.drawText("Musical Scale", titleArea.removeFromLeft(pW).withHeight(30), juce::Justification::centred);
    g.drawText("Expressiveness", titleArea.withHeight(30), juce::Justification::centred);
}

void PluginEditor::resized()
{
    auto area = getLocalBounds();
    
    // 0. Calibrate Top Bar
    auto calibrationArea = area.removeFromTop(30).reduced(2);
    calibrateButton.setBounds(calibrationArea.removeFromLeft(120));
    calibrationLabel.setBounds(calibrationArea);
    
    // 1. Visualizer and HUD
    auto visArea = area.removeFromTop(150).reduced(10);
    auto topVis = visArea.removeFromTop(30);
    timbreMeter.setBounds(topVis.removeFromRight(30));
    hud.setBounds(topVis);
    visualizer.setBounds(visArea);
    
    // 2. Three Panels
    auto panelArea = area.reduced(10);
    panelArea.removeFromTop(30); // Titles space
    int pW = panelArea.getWidth() / 3;
    
    auto leftPanel = panelArea.removeFromLeft(pW).reduced(10);
    auto midPanel = panelArea.removeFromLeft(pW).reduced(10);
    auto rightPanel = panelArea.reduced(10);
    
    // Helpers
    auto placeSlider = [](juce::Slider& s, juce::Label& l, juce::Rectangle<int>& bounds) {
        auto r = bounds.removeFromLeft(bounds.getWidth() / 2);
        s.setBounds(r.withTrimmedBottom(20));
        l.setBounds(r.withTop(s.getBottom()).withHeight(20));
    };
    
    auto placeCombo = [](juce::ComboBox& c, juce::Label& l, juce::Rectangle<int>& bounds, int divisor) {
        auto row = bounds.removeFromTop(bounds.getHeight() / divisor);
        l.setBounds(row.removeFromTop(20));
        c.setBounds(row.reduced(5, (row.getHeight() - 24) / 2));
    };

    // Left Panel (4 sliders)
    auto row1L = leftPanel.removeFromTop(leftPanel.getHeight() / 2);
    auto row2L = leftPanel;
    placeSlider(inputGainSlider, inputGainLabel, row1L);
    placeSlider(gateThresholdSlider, gateThresholdLabel, row1L);
    placeSlider(minFreqSlider, minFreqLabel, row2L);
    placeSlider(maxFreqSlider, maxFreqLabel, row2L);

    // Mid Panel (4 combos)
    placeCombo(scaleRootBox, scaleRootLabel, midPanel, 4);
    placeCombo(scaleTypeBox, scaleTypeLabel, midPanel, 3);
    placeCombo(trackingModeBox, trackingModeLabel, midPanel, 2);
    placeCombo(expressionCCBox, expressionCCLabel, midPanel, 1);

    // Right Panel (2 combos, 2 sliders)
    auto rightComboArea = rightPanel.removeFromTop(rightPanel.getHeight() / 2);
    placeCombo(intellibendModeBox, intellibendModeLabel, rightComboArea, 2);
    placeCombo(pitchBendRangeBox, pitchBendRangeLabel, rightComboArea, 1);
    
    placeSlider(intellibendStickinessSlider, intellibendStickinessLabel, rightPanel);
    placeSlider(pitchBendGlideSlider, pitchBendGlideLabel, rightPanel);
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
