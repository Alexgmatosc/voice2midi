#pragma once
#include <JuceHeader.h>

namespace v2m
{

class HUDDisplay : public juce::Component
{
public:
    HUDDisplay();
    ~HUDDisplay() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Set the current pitch state to be painted
    void updateState(int midiNote, float pitchHz);

private:
    int currentMidiNote = -1;
    float currentPitchHz = -1.0f;
    float centsDeviation = 0.0f;

    juce::String getNoteName(int midiNote) const;
    void calculateCentsDeviation();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HUDDisplay)
};

} // namespace v2m
