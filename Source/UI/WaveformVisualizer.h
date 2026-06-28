#pragma once
#include <JuceHeader.h>
#include <vector>

namespace v2m
{

class WaveformVisualizer : public juce::Component
{
public:
    WaveformVisualizer(juce::AudioProcessorValueTreeState& apvts);
    ~WaveformVisualizer() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Call this from the UI timer to push new downsampled samples into the visualizer history
    void pushSamples(const float* samples, int numSamples);

    // Mouse interactions for dragging the gate threshold line
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;

private:
    juce::AudioProcessorValueTreeState& apvts;
    
    // Waveform history buffer (rolling display)
    std::vector<float> waveformHistory;
    int historySize = 300; // Matches pixel width approx
    
    // Gate dragging state
    bool isDraggingGate = false;
    bool isMouseOverGate = false;
    
    // Helper mappings
    float getGateY() const;
    float getGateDb() const { return apvts.getRawParameterValue("gate_threshold")->load(); }
    void updateGateParameter(float newDb);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformVisualizer)
};

} // namespace v2m
